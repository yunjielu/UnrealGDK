// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "LoadBalancing/VoronoiGridBasedLBStrategy.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "Utils/SpatialActorUtils.h"
#include "GenericPlatform/GenericPlatformMath.h"

DEFINE_LOG_CATEGORY(LogVoronoiGridBasedLBStrategy);

UVoronoiGridBasedLBStrategy::UVoronoiGridBasedLBStrategy()
	: Super()
	, Rows(10)
	, Cols(10)
	, WorldWidth(10000.f)
	, WorldHeight(10000.f)
{
}

void UVoronoiGridBasedLBStrategy::Init(const USpatialNetDriver* InNetDriver)
{
	Super::Init(InNetDriver);

	const static TArray<FVector2D> Points = { FVector2D(-5000.f, -5000.f), FVector2D(5000.f, 5000.f), FVector2D(5000.f, -5000.f) , FVector2D(0.f, 0.f) };

	const size_t Size = Points.Num();
	for (uint32 i = 0; i < Size; ++i)
	{
		VirtualWorkerId VirtualWorkerId = i + 1;
		const FVector2D& Position = Points[i];
		VirtualWorkerIds.Add(VirtualWorkerId);
		VirtualWorkerPositions.Add(VirtualWorkerId, Position);
	}

	WorldWidthMin = -(WorldWidth / 2.f);
	WorldHeightMin = -(WorldHeight / 2.f);

	const float ColumnWidth = WorldWidth / Cols;
	const float RowHeight = WorldHeight / Rows;

	const float HalfColumnWidth = ColumnWidth / 2.f;
	const float HalfRowHeight = RowHeight / 2.f;

	float Y = WorldWidthMin;
	float X = WorldHeightMin;

	for (uint32 Col = 0; Col < Cols; ++Col)
	{
		for (uint32 Row = 0; Row < Rows; ++Row)
		{
			FVector2D CellPosition = FVector2D(X + HalfColumnWidth, Y + HalfRowHeight);
			VirtualWorkerId VirtualWorkerId = FindBestVirtualWorker(CellPosition);
			VirtualWorkerIdsGrid.Add(VirtualWorkerId);
			Y += RowHeight;
		}
		Y = WorldHeightMin;
		X += ColumnWidth;
	}

	PrintVirtualWorkerIdsGrid();
}

VirtualWorkerId UVoronoiGridBasedLBStrategy::FindBestVirtualWorker(const FVector2D& Location) const
{
	VirtualWorkerId BestVirtualWorkerId;
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

TSet<VirtualWorkerId> UVoronoiGridBasedLBStrategy::GetVirtualWorkerIds() const
{
	return VirtualWorkerIds;
}

bool UVoronoiGridBasedLBStrategy::ShouldRelinquishAuthority(const AActor& Actor) const
{
	if (!IsReady())
	{
		return false;
	}

	const FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));

	int32 CellIndex = PositionToCellIndex(Actor2DLocation);
	return LocalVirtualWorkerId != VirtualWorkerIdsGrid[CellIndex];
}

VirtualWorkerId UVoronoiGridBasedLBStrategy::WhoShouldHaveAuthority(const AActor& Actor) const
{
	if (!IsReady())
	{
		return SpatialConstants::INVALID_VIRTUAL_WORKER_ID;
	}

	FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));
	int32 CellIndex = PositionToCellIndex(Actor2DLocation);
	return VirtualWorkerIdsGrid[CellIndex];
}

int32 UVoronoiGridBasedLBStrategy::PositionToCellIndex(const FVector2D& Location) const
{
	float ProgressY = (Location.Y - WorldWidthMin) / WorldWidth;
	int32 CellX = ProgressY * Cols;
	FMath::Clamp(CellX, 0, static_cast<int32>(Cols));

	float ProgressX = (Location.X - WorldHeightMin) / WorldHeight;
	int32 CellY = ProgressX * Rows;
	FMath::Clamp(CellY, 0, static_cast<int32>(Rows));

	return CellY * Cols + CellX;
}

void UVoronoiGridBasedLBStrategy::PrintVirtualWorkerIdsGrid() const
{
	const size_t Size = VirtualWorkerIdsGrid.Num();

	UE_LOG(LogVoronoiGridBasedLBStrategy, Log, TEXT("World Virtual Worker Distribution"));

	FString Buffer = TEXT("");
	for (int i = 0; i < Size; ++i)
	{
		Buffer.Append(FString::Printf(TEXT("%d, "), VirtualWorkerIdsGrid[i]));
		if ((i + 1) % Cols == 0)
		{
			UE_LOG(LogVoronoiGridBasedLBStrategy, Log, TEXT("%s"), *Buffer);
			Buffer.Empty();
		}
	}
}
