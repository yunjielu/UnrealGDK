// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "LoadBalancing/AbstractLBStrategy.h"
#include "GridBasedLBStrategy.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGridBasedLBStrategy, Log, All);

/**
 * A load balancing strategy that divides the world into a grid.
 * Divides the load between Rows * Cols number of workers, each handling a
 * square of the world (WorldWidth / Cols)cm by (WorldHeight / Rows)cm
 *
 * Given a Point, for each Cell:
 * Point is inside Cell iff Min(Cell) <= Point < Max(Cell)
 *
 * Intended Usage: Create a data-only blueprint subclass and change
 * the Cols, Rows, WorldWidth, WorldHeight.
 */
UCLASS(abstract)
class SPATIALGDK_API UGridBasedLBStrategy : public UAbstractLBStrategy
{
	GENERATED_BODY()

public:
	UGridBasedLBStrategy();

/* UAbstractLBStrategy Interface */
	virtual void Init(const class USpatialNetDriver* InNetDriver) override;

	virtual TSet<VirtualWorkerId> GetVirtualWorkerIds() const { return VirtualWorkerIds; };

	virtual bool ShouldRelinquishAuthority(const AActor& Actor) const override;
	virtual VirtualWorkerId WhoShouldHaveAuthority(const AActor& Actor) const override;
/* End UAbstractLBStrategy Interface */

	void PrintVirtualWorkerIdGrid() const;

protected:
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"), category="LoadBalancing")
	uint32 Rows;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"), category = "LoadBalancing")
	uint32 Cols;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"), category = "LoadBalancing")
	float WorldWidth;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"), category = "LoadBalancing")
	float WorldHeight;

	float WorldWidthMin;
	float WorldHeightMin;

	virtual void InitVirtualWorkerIdGrid() PURE_VIRTUAL(UGridBasedLBStrategy::InitVirtualWorkerIdsGrid, ;)

	void AddVirtualWorkerIdToGrid(VirtualWorkerId VirtualWorkerId);

	int32 WorldPositionToCellIndex(const FVector2D& Position) const;

private:

	TSet<VirtualWorkerId> VirtualWorkerIds;
	TArray<VirtualWorkerId> VirtualWorkerIdGrid;

};
