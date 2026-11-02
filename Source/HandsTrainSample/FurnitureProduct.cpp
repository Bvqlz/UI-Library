/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "FurnitureProduct.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const FName ColorParamName = FName(TEXT("Color"));

	FProductDefinition MakeDefinition(const TCHAR* Id, const TCHAR* Name, EFurnitureType Type, const FVector& SizeCm)
	{
		FProductDefinition Definition;
		Definition.Id = FName(Id);
		Definition.DisplayName = FText::FromString(Name);
		Definition.Type = Type;
		Definition.SizeCm = SizeCm;
		return Definition;
	}

	FLinearColor GetBaseColor(EFurnitureType Type)
	{
		switch (Type)
		{
			case EFurnitureType::Chair:
				return FLinearColor(0.55f, 0.32f, 0.14f);
			case EFurnitureType::Table:
				return FLinearColor(0.45f, 0.28f, 0.14f);
			case EFurnitureType::Desk:
				return FLinearColor(0.35f, 0.35f, 0.4f);
			case EFurnitureType::Sofa:
				return FLinearColor(0.2f, 0.3f, 0.5f);
			case EFurnitureType::Bookshelf:
				return FLinearColor(0.3f, 0.2f, 0.1f);
		}
		return FLinearColor::White;
	}
}

TArray<FProductDefinition> UProductCatalogAsset::MakeDefaultProducts()
{
	return {
		MakeDefinition(TEXT("chair"), TEXT("Chair"), EFurnitureType::Chair, FVector(45.0f, 45.0f, 90.0f)),
		MakeDefinition(TEXT("table"), TEXT("Table"), EFurnitureType::Table, FVector(60.0f, 120.0f, 75.0f)),
		MakeDefinition(TEXT("desk"), TEXT("Desk"), EFurnitureType::Desk, FVector(60.0f, 120.0f, 75.0f)),
		MakeDefinition(TEXT("sofa"), TEXT("Sofa"), EFurnitureType::Sofa, FVector(85.0f, 190.0f, 80.0f)),
		MakeDefinition(TEXT("bookshelf"), TEXT("Bookshelf"), EFurnitureType::Bookshelf, FVector(30.0f, 80.0f, 180.0f)),
	};
}

AFurnitureProduct::AFurnitureProduct()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootSceneComponent;
}

void AFurnitureProduct::BeginPlay()
{
	Super::BeginPlay();

	// Products placed in the editor carry a definition; spawned ones are initialized after spawning
	if (Parts.Num() == 0 && !Definition.Id.IsNone())
	{
		RebuildParts();
	}
}

void AFurnitureProduct::InitializeFromDefinition(const FProductDefinition& InDefinition)
{
	Definition = InDefinition;
	RebuildParts();
}

UStaticMeshComponent* AFurnitureProduct::AddBox(const FVector& CenterCm, const FVector& SizeCm)
{
	static UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	static UMaterialInterface* ShapeMaterial =
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetStaticMesh(CubeMesh);
	Part->SetMaterial(0, ShapeMaterial);
	Part->SetupAttachment(RootComponent);
	Part->SetRelativeLocation(CenterCm);
	Part->SetRelativeScale3D(SizeCm * 0.01f);
	Part->SetCastShadow(false);
	// Visible to ray traces so products can be selected; never blocks the pawn
	Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Part->SetCollisionResponseToAllChannels(ECR_Ignore);
	Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Part->RegisterComponent();

	Parts.Add(Part);
	PartMaterials.Add(Part->CreateDynamicMaterialInstance(0));
	return Part;
}

void AFurnitureProduct::RebuildParts()
{
	for (UStaticMeshComponent* Part : Parts)
	{
		if (Part)
		{
			Part->DestroyComponent();
		}
	}
	Parts.Reset();
	PartMaterials.Reset();

	// Actor origin is the center of the footprint on the floor; X runs front to back, Y across, Z up.
	// The front of the product faces +X, so a placement yaw of 180 faces a viewer standing at -X.
	const float D = Definition.SizeCm.X;
	const float W = Definition.SizeCm.Y;
	const float H = Definition.SizeCm.Z;

	switch (Definition.Type)
	{
		case EFurnitureType::Chair:
		{
			const float Leg = 4.0f;
			const float SeatZ = H * 0.5f;
			const float SeatT = 4.0f;
			AddBox(FVector(0, 0, SeatZ), FVector(D, W, SeatT));
			AddBox(FVector(-D * 0.5f + 2.0f, 0, SeatZ + (H - SeatZ) * 0.5f), FVector(4.0f, W, H - SeatZ));
			for (const float X : {-D * 0.5f + Leg * 0.5f, D * 0.5f - Leg * 0.5f})
			{
				for (const float Y : {-W * 0.5f + Leg * 0.5f, W * 0.5f - Leg * 0.5f})
				{
					AddBox(FVector(X, Y, (SeatZ - SeatT * 0.5f) * 0.5f), FVector(Leg, Leg, SeatZ - SeatT * 0.5f));
				}
			}
			break;
		}
		case EFurnitureType::Table:
		{
			const float Leg = 6.0f;
			const float TopT = 4.0f;
			AddBox(FVector(0, 0, H - TopT * 0.5f), FVector(D, W, TopT));
			for (const float X : {-D * 0.5f + Leg * 0.5f + 2.0f, D * 0.5f - Leg * 0.5f - 2.0f})
			{
				for (const float Y : {-W * 0.5f + Leg * 0.5f + 2.0f, W * 0.5f - Leg * 0.5f - 2.0f})
				{
					AddBox(FVector(X, Y, (H - TopT) * 0.5f), FVector(Leg, Leg, H - TopT));
				}
			}
			break;
		}
		case EFurnitureType::Desk:
		{
			const float TopT = 4.0f;
			const float PanelT = 3.0f;
			AddBox(FVector(0, 0, H - TopT * 0.5f), FVector(D, W, TopT));
			AddBox(FVector(0, -W * 0.5f + PanelT * 0.5f, (H - TopT) * 0.5f), FVector(D - 2.0f, PanelT, H - TopT));
			AddBox(FVector(0, W * 0.5f - PanelT * 0.5f, (H - TopT) * 0.5f), FVector(D - 2.0f, PanelT, H - TopT));
			AddBox(FVector(-D * 0.5f + PanelT * 0.5f, 0, (H - TopT) * 0.5f + 5.0f), FVector(PanelT, W - 2.0f * PanelT, (H - TopT) - 10.0f));
			break;
		}
		case EFurnitureType::Sofa:
		{
			const float BaseH = H * 0.5f;
			const float ArmW = 14.0f;
			AddBox(FVector(0, 0, BaseH * 0.5f), FVector(D, W, BaseH));
			AddBox(FVector(-D * 0.5f + 9.0f, 0, BaseH + (H - BaseH) * 0.5f), FVector(18.0f, W, H - BaseH));
			AddBox(FVector(0, -W * 0.5f + ArmW * 0.5f, BaseH + 8.0f), FVector(D, ArmW, 16.0f));
			AddBox(FVector(0, W * 0.5f - ArmW * 0.5f, BaseH + 8.0f), FVector(D, ArmW, 16.0f));
			break;
		}
		case EFurnitureType::Bookshelf:
		{
			const float SideT = 3.0f;
			AddBox(FVector(0, -W * 0.5f + SideT * 0.5f, H * 0.5f), FVector(D, SideT, H));
			AddBox(FVector(0, W * 0.5f - SideT * 0.5f, H * 0.5f), FVector(D, SideT, H));
			AddBox(FVector(-D * 0.5f + 1.0f, 0, H * 0.5f), FVector(2.0f, W - 2.0f * SideT, H));
			for (int32 Shelf = 0; Shelf < 6; ++Shelf)
			{
				const float Z = Shelf * (H - SideT) / 5.0f + SideT * 0.5f;
				AddBox(FVector(0, 0, Z), FVector(D, W - 2.0f * SideT, SideT));
			}
			break;
		}
	}

	ApplyColor();
}

void AFurnitureProduct::SetGhost(bool bIsGhost)
{
	bGhost = bIsGhost;
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetCollisionEnabled(bGhost ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	}
	SetVisualState(bGhost ? EProductVisualState::GhostValid : EProductVisualState::Normal);
}

void AFurnitureProduct::SetVisualState(EProductVisualState NewState)
{
	VisualState = NewState;
	ApplyColor();
}

void AFurnitureProduct::ApplyColor()
{
	FLinearColor Color = GetBaseColor(Definition.Type);
	switch (VisualState)
	{
		case EProductVisualState::Hovered:
			Color = FLinearColor(0.9f, 0.8f, 0.2f);
			break;
		case EProductVisualState::Selected:
			Color = FLinearColor(0.2f, 0.7f, 0.9f);
			break;
		case EProductVisualState::GhostValid:
			Color = FLinearColor(0.1f, 0.9f, 0.3f);
			break;
		case EProductVisualState::GhostInvalid:
			Color = FLinearColor(0.9f, 0.1f, 0.1f);
			break;
		default:
			break;
	}

	for (UMaterialInstanceDynamic* Material : PartMaterials)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(ColorParamName, Color);
		}
	}
}

FVector2D AFurnitureProduct::GetFootprintHalfExtent() const
{
	// Axis-aligned bounds of the footprint rectangle rotated by the actor's yaw
	const float Yaw = FMath::DegreesToRadians(GetActorRotation().Yaw);
	const float C = FMath::Abs(FMath::Cos(Yaw));
	const float S = FMath::Abs(FMath::Sin(Yaw));
	const float HalfD = Definition.SizeCm.X * 0.5f;
	const float HalfW = Definition.SizeCm.Y * 0.5f;
	return FVector2D(HalfD * C + HalfW * S, HalfD * S + HalfW * C);
}
