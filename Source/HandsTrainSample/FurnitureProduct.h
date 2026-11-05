/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "FurnitureProduct.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** Furniture shapes the sample catalog can build from primitive boxes. */
UENUM(BlueprintType)
enum class EFurnitureType : uint8
{
	Chair,
	Table,
	Desk,
	Sofa,
	Bookshelf
};

/** What the host says a product allows. Tools use these instead of hard-coded product names. */
USTRUCT(BlueprintType)
struct FProductCapabilities
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bMovable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRotatable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRemovable = true;

	/** The Scale / Dimensions tool may change the width (Y). Height and depth stay fixed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bResizable = true;

	/** Allowed width range for the Scale / Dimensions tool (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bResizable"))
	float MinWidthCm = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bResizable"))
	float MaxWidthCm = 300.0f;
};

/** One catalog entry. */
USTRUCT(BlueprintType)
struct FProductDefinition
{
	GENERATED_BODY()

	/** Stable id, used by history to rebuild a removed product. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFurnitureType Type = EFurnitureType::Chair;

	/** Footprint and height in cm: X is front-to-back depth, Y is width, Z is height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SizeCm = FVector(50.0f, 50.0f, 90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FProductCapabilities Capabilities;
};

/**
 * The product catalog, supplied by the host as a data asset. If none is assigned, the menu
 * uses MakeDefaultProducts().
 */
UCLASS(BlueprintType)
class HANDSTRAINSAMPLE_API UProductCatalogAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catalog")
	TArray<FProductDefinition> Products;

	/** The five sample furniture products. */
	static TArray<FProductDefinition> MakeDefaultProducts();
};

/** How a product is currently drawn. */
UENUM(BlueprintType)
enum class EProductVisualState : uint8
{
	Normal,
	Hovered,
	Selected,
	GhostValid,
	GhostInvalid
};

/** A piece of furniture built from boxes. Placement and removal go through UProductHistorySubsystem. */
UCLASS()
class HANDSTRAINSAMPLE_API AFurnitureProduct : public AActor
{
	GENERATED_BODY()

public:
	AFurnitureProduct();

	/** Stable identity that survives remove and undo. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product")
	FGuid ProductGuid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product")
	FProductDefinition Definition;

	/** Set the definition and (re)build the parts. Call before FinishSpawning when spawning deferred. */
	void InitializeFromDefinition(const FProductDefinition& InDefinition);

	/** A ghost has no collision and is drawn in the valid or invalid color. */
	void SetGhost(bool bIsGhost);

	UFUNCTION(BlueprintCallable, Category = "Product")
	void SetVisualState(EProductVisualState NewState);

	UFUNCTION(BlueprintCallable, Category = "Product")
	EProductVisualState GetVisualState() const { return VisualState; }

	/** Footprint half-extents in world X and Y (cm), accounting for the actor's yaw. */
	FVector2D GetFootprintHalfExtent() const;

protected:
	virtual void BeginPlay() override;

private:
	void RebuildParts();
	UStaticMeshComponent* AddBox(const FVector& CenterCm, const FVector& SizeCm);
	void ApplyColor();

	UPROPERTY()
	USceneComponent* RootSceneComponent;

	UPROPERTY()
	TArray<UStaticMeshComponent*> Parts;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> PartMaterials;

	EProductVisualState VisualState = EProductVisualState::Normal;
	bool bGhost = false;
};
