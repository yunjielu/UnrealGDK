// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "LoadBalancing/AbstractLBStrategy.h"
#include "VoronoiGridBasedLBStrategy.generated.h"

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

DECLARE_LOG_CATEGORY_EXTERN(LogVoronoiGridBasedLBStrategy, Log, All);

UCLASS(Blueprintable)
class SPATIALGDK_API UVoronoiGridBasedLBStrategy : public UAbstractLBStrategy
{
	GENERATED_BODY()

public:
	UVoronoiGridBasedLBStrategy();

/* UAbstractLBStrategy Interface */
	virtual void Init(const class USpatialNetDriver* InNetDriver) override;

	virtual TSet<VirtualWorkerId> GetVirtualWorkerIds() const;

	virtual bool ShouldRelinquishAuthority(const AActor& Actor) const override;
	virtual VirtualWorkerId WhoShouldHaveAuthority(const AActor& Actor) const override;
/* End UAbstractLBStrategy Interface */

protected:
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"), Category = "Load Balancing")
	uint32 Rows;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"), Category = "Load Balancing")
	uint32 Cols;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"), Category = "Load Balancing")
	float WorldWidth;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"), Category = "Load Balancing")
	float WorldHeight;

private:

	float WorldWidthMin;
	float WorldHeightMin;

	TSet<VirtualWorkerId> VirtualWorkerIds;
	TArray<VirtualWorkerId> VirtualWorkerIdsGrid;
	TMap<VirtualWorkerId, FVector2D> VirtualWorkerPositions;

	int32 PositionToCellIndex(const FVector2D& Position) const;
	VirtualWorkerId FindBestVirtualWorker(const FVector2D& Position) const;

	void PrintVirtualWorkerIdsGrid() const;
};
