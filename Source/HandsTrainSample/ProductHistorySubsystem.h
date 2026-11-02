/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "FurnitureProduct.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProductHistorySubsystem.generated.h"

class AFurnitureProduct;

/** What kind of completed edit a history entry reverses. */
UENUM(BlueprintType)
enum class EProductEditKind : uint8
{
	Place,
	Remove
};

/** One completed, reversible product edit. */
USTRUCT(BlueprintType)
struct FProductEdit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EProductEditKind Kind = EProductEditKind::Place;

	UPROPERTY(BlueprintReadOnly)
	FGuid ProductGuid;

	/** Enough state to rebuild the product when the edit is undone (remove) or redone (place). */
	UPROPERTY(BlueprintReadOnly)
	FProductDefinition Definition;

	UPROPERTY(BlueprintReadOnly)
	FTransform Transform;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FProductHistoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FProductEditApplied, const FProductEdit&, Edit, bool, bWasUndo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProductEditRejected, const FText&, Reason);

/**
 * History service: creates and removes products and keeps the stack of completed edits.
 * One Undo reverses one completed edit, a new edit clears redo, and selection or tool changes
 * never enter the history.
 */
UCLASS()
class HANDSTRAINSAMPLE_API UProductHistorySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** Create a product and record it as one edit. Returns the new product, or null. */
	UFUNCTION(BlueprintCallable, Category = "Product History")
	AFurnitureProduct* PlaceProduct(const FProductDefinition& Definition, const FTransform& Transform);

	/** Remove a product and record it as one edit. */
	UFUNCTION(BlueprintCallable, Category = "Product History")
	bool RemoveProduct(AFurnitureProduct* Product);

	UFUNCTION(BlueprintCallable, Category = "Product History")
	bool Undo();

	UFUNCTION(BlueprintCallable, Category = "Product History")
	bool Redo();

	UFUNCTION(BlueprintPure, Category = "Product History")
	bool CanUndo() const { return UndoStack.Num() > 0; }

	UFUNCTION(BlueprintPure, Category = "Product History")
	bool CanRedo() const { return RedoStack.Num() > 0; }

	/** Products that currently exist in the world (history-managed ones). */
	UFUNCTION(BlueprintCallable, Category = "Product History")
	TArray<AFurnitureProduct*> GetProducts() const;

	/** Fired whenever the undo/redo availability may have changed. */
	UPROPERTY(BlueprintAssignable, Category = "Product History")
	FProductHistoryChanged OnHistoryChanged;

	/** Host change event: a product was created, removed, restored or re-removed. */
	UPROPERTY(BlueprintAssignable, Category = "Product History")
	FProductEditApplied OnEditApplied;

	/** An edit could not be applied (for example the target vanished). Nothing was partially applied. */
	UPROPERTY(BlueprintAssignable, Category = "Product History")
	FProductEditRejected OnEditRejected;

private:
	AFurnitureProduct* SpawnProduct(const FProductDefinition& Definition, const FTransform& Transform, const FGuid& Guid);
	bool DestroyProduct(const FGuid& Guid);

	TArray<FProductEdit> UndoStack;
	TArray<FProductEdit> RedoStack;
	TMap<FGuid, TWeakObjectPtr<AFurnitureProduct>> Products;
};
