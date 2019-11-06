// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "LoadBalancing/GridBasedLBStrategy.h"
#include "StandardGridBasedLBStrategy.generated.h"

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

UCLASS(Blueprintable)
class SPATIALGDK_API UStandardGridBasedLBStrategy : public UGridBasedLBStrategy
{
	GENERATED_BODY()

public:
	UStandardGridBasedLBStrategy();

/* UAbstractLBStrategy Interface */
	virtual void Init(const class USpatialNetDriver* InNetDriver) override;
/* End UAbstractLBStrategy Interface */

protected:

/* UGridBasedLBStrategy Interface */
	virtual void InitVirtualWorkerIdGrid() override;
/* End UGridBasedLBStrategy Interface */
};
