// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "LoadBalancing/GridBasedLBStrategy.h"
#include "VoronoiGridBasedLBStrategy.generated.h"

USTRUCT()
struct FPointInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Voronoi Load Balancing")
	float X;

	UPROPERTY(EditDefaultsOnly, Category = "Voronoi Load Balancing")
	float Y;
};

UCLASS(Blueprintable)
class SPATIALGDK_API UVoronoiGridBasedLBStrategy : public UGridBasedLBStrategy
{
	GENERATED_BODY()

public:
	UVoronoiGridBasedLBStrategy();

/* UAbstractLBStrategy Interface */
	virtual void Init(const class USpatialNetDriver* InNetDriver) override;
/* End UAbstractLBStrategy Interface */

protected:

	/* Points configuration. */
	UPROPERTY(EditDefaultsOnly, Category = "Voronoi Load Balancing")
	TArray<FPointInfo> Points;

/* UGridBasedLBStrategy Interface */
	virtual void InitVirtualWorkerIdGrid() override;
/* End UGridBasedLBStrategy Interface */

private:

	TMap<VirtualWorkerId, FVector2D> VirtualWorkerPositions;
	VirtualWorkerId FindBestVirtualWorker(const FVector2D& Position) const;
};
