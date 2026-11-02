/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "ProductHistorySubsystem.h"
#include "Engine/World.h"

bool UProductHistorySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only worlds that actually run: game and PIE, not editor preview or asset worlds
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

AFurnitureProduct* UProductHistorySubsystem::SpawnProduct(
	const FProductDefinition& Definition, const FTransform& Transform, const FGuid& Guid)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFurnitureProduct* Product = World->SpawnActor<AFurnitureProduct>(AFurnitureProduct::StaticClass(), Transform, SpawnParameters);
	if (!Product)
	{
		return nullptr;
	}

	Product->ProductGuid = Guid;
	Product->InitializeFromDefinition(Definition);
	Products.Add(Guid, Product);
	return Product;
}

bool UProductHistorySubsystem::DestroyProduct(const FGuid& Guid)
{
	const TWeakObjectPtr<AFurnitureProduct>* Found = Products.Find(Guid);
	AFurnitureProduct* Product = Found ? Found->Get() : nullptr;
	if (!IsValid(Product))
	{
		return false;
	}

	Products.Remove(Guid);
	Product->Destroy();
	return true;
}

AFurnitureProduct* UProductHistorySubsystem::PlaceProduct(const FProductDefinition& Definition, const FTransform& Transform)
{
	const FGuid Guid = FGuid::NewGuid();
	AFurnitureProduct* Product = SpawnProduct(Definition, Transform, Guid);
	if (!Product)
	{
		OnEditRejected.Broadcast(FText::FromString(TEXT("The product could not be created.")));
		return nullptr;
	}

	FProductEdit Edit;
	Edit.Kind = EProductEditKind::Place;
	Edit.ProductGuid = Guid;
	Edit.Definition = Definition;
	Edit.Transform = Transform;

	// A new edit clears redo
	UndoStack.Add(Edit);
	RedoStack.Reset();

	OnEditApplied.Broadcast(Edit, false);
	OnHistoryChanged.Broadcast();
	return Product;
}

bool UProductHistorySubsystem::RemoveProduct(AFurnitureProduct* Product)
{
	if (!IsValid(Product) || !Products.Contains(Product->ProductGuid))
	{
		OnEditRejected.Broadcast(FText::FromString(TEXT("There is no product to remove.")));
		return false;
	}
	if (!Product->Definition.Capabilities.bRemovable)
	{
		OnEditRejected.Broadcast(FText::FromString(TEXT("This product cannot be removed.")));
		return false;
	}

	FProductEdit Edit;
	Edit.Kind = EProductEditKind::Remove;
	Edit.ProductGuid = Product->ProductGuid;
	Edit.Definition = Product->Definition;
	Edit.Transform = Product->GetActorTransform();

	DestroyProduct(Edit.ProductGuid);

	UndoStack.Add(Edit);
	RedoStack.Reset();

	OnEditApplied.Broadcast(Edit, false);
	OnHistoryChanged.Broadcast();
	return true;
}

bool UProductHistorySubsystem::Undo()
{
	if (UndoStack.Num() == 0)
	{
		return false;
	}

	const FProductEdit Edit = UndoStack.Last();
	bool bApplied = false;
	if (Edit.Kind == EProductEditKind::Place)
	{
		bApplied = DestroyProduct(Edit.ProductGuid);
	}
	else
	{
		bApplied = SpawnProduct(Edit.Definition, Edit.Transform, Edit.ProductGuid) != nullptr;
	}

	if (!bApplied)
	{
		// Leave both stacks untouched rather than applying the edit partially
		OnEditRejected.Broadcast(FText::FromString(TEXT("Undo is not possible: the product was changed elsewhere.")));
		return false;
	}

	UndoStack.Pop();
	RedoStack.Add(Edit);
	OnEditApplied.Broadcast(Edit, true);
	OnHistoryChanged.Broadcast();
	return true;
}

bool UProductHistorySubsystem::Redo()
{
	if (RedoStack.Num() == 0)
	{
		return false;
	}

	const FProductEdit Edit = RedoStack.Last();
	bool bApplied = false;
	if (Edit.Kind == EProductEditKind::Place)
	{
		bApplied = SpawnProduct(Edit.Definition, Edit.Transform, Edit.ProductGuid) != nullptr;
	}
	else
	{
		bApplied = DestroyProduct(Edit.ProductGuid);
	}

	if (!bApplied)
	{
		OnEditRejected.Broadcast(FText::FromString(TEXT("Redo is not possible: the product was changed elsewhere.")));
		return false;
	}

	RedoStack.Pop();
	UndoStack.Add(Edit);
	OnEditApplied.Broadcast(Edit, false);
	OnHistoryChanged.Broadcast();
	return true;
}

TArray<AFurnitureProduct*> UProductHistorySubsystem::GetProducts() const
{
	TArray<AFurnitureProduct*> Result;
	for (const TPair<FGuid, TWeakObjectPtr<AFurnitureProduct>>& Pair : Products)
	{
		if (AFurnitureProduct* Product = Pair.Value.Get())
		{
			Result.Add(Product);
		}
	}
	return Result;
}
