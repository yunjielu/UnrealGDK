// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "LoadBalancing/VoronoiGridBasedLBStrategy.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "Utils/SpatialActorUtils.h"
#include "GenericPlatform/GenericPlatformMath.h"

UVoronoiGridBasedLBStrategy::UVoronoiGridBasedLBStrategy()
	: Super()
{
}

void UVoronoiGridBasedLBStrategy::Init(const USpatialNetDriver* InNetDriver)
{
	Super::Init(InNetDriver);
}

void UVoronoiGridBasedLBStrategy::InitVirtualWorkerIdGrid()
{
	const static TArray<FVector2D> Points = { FVector2D(-5000.f, -5000.f), FVector2D(5000.f, 5000.f), FVector2D(5000.f, -5000.f) , FVector2D(0.f, 0.f) };

	const int32 Size = Points.Num();
	for (int32 i = 0; i < Size; ++i)
	{
		const FVector2D& Position = Points[i];
		VirtualWorkerPositions.Add(i + 1, Position);
	}

	const float ColumnWidth = WorldWidth / Cols;
	const float RowHeight = WorldHeight / Rows;

	const float StartCellY = WorldHeightMin + RowHeight / 2.f;
	float CellY = StartCellY;
	float CellX = WorldWidthMin + ColumnWidth / 2.f;

	for (uint32 Col = 0; Col < Cols; ++Col)
	{
		for (uint32 Row = 0; Row < Rows; ++Row)
		{
			FVector2D CellPosition = FVector2D(CellY, CellX);
			VirtualWorkerId VirtualWorkerId = FindBestVirtualWorker(CellPosition);
			AddVirtualWorkerIdToGrid(VirtualWorkerId);
			CellY += RowHeight;
		}
		CellY = StartCellY;
		CellX += ColumnWidth;
	}
}

VirtualWorkerId UVoronoiGridBasedLBStrategy::FindBestVirtualWorker(const FVector2D& Location) const
{
	VirtualWorkerId BestVirtualWorkerId = SpatialConstants::INVALID_VIRTUAL_WORKER_ID;
	float BestSquareDistance = -1;

	for (const auto& Pair : VirtualWorkerPositions)
	{
		FVector2D CellPosition = Pair.Value;
		float SquareDistance = pow(Location.X - CellPosition.X, 2) + pow(Location.Y - CellPosition.Y, 2);
		if (BestSquareDistance == -1 ||
			SquareDistance < BestSquareDistance)
		{
			BestVirtualWorkerId = Pair.Key;
			BestSquareDistance = SquareDistance;
		}
	}

	return BestVirtualWorkerId;
}
