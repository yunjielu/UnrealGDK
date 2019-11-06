// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "LoadBalancing/GridBasedLBStrategy.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "Utils/SpatialActorUtils.h"

DEFINE_LOG_CATEGORY(LogGridBasedLBStrategy);

UGridBasedLBStrategy::UGridBasedLBStrategy()
	: Super()
	, Rows(10)
	, Cols(10)
	, WorldWidth(10000.f)
	, WorldHeight(10000.f)
{
	WorldWidthMin = -(WorldWidth / 2.f);
	WorldHeightMin = -(WorldHeight / 2.f);
}

void UGridBasedLBStrategy::Init(const USpatialNetDriver* InNetDriver)
{
	Super::Init(InNetDriver);
	VirtualWorkerIdGrid.Reserve(Rows*Cols);
	InitVirtualWorkerIdGrid();
}

void UGridBasedLBStrategy::AddVirtualWorkerIdToGrid(VirtualWorkerId VirtualWorkerId)
{
	VirtualWorkerIds.Add(VirtualWorkerId);
	VirtualWorkerIdGrid.Add(VirtualWorkerId);
}

bool UGridBasedLBStrategy::ShouldRelinquishAuthority(const AActor& Actor) const
{
	if (!IsReady())
	{
		return false;
	}

	const FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));

	int32 CellIndex = WorldPositionToCellIndex(Actor2DLocation);
	if (VirtualWorkerIdGrid.Num() <= CellIndex)
	{
		return false;
	}

	return LocalVirtualWorkerId != VirtualWorkerIdGrid[CellIndex];
}

VirtualWorkerId UGridBasedLBStrategy::WhoShouldHaveAuthority(const AActor& Actor) const
{
	if (!IsReady())
	{
		return SpatialConstants::INVALID_VIRTUAL_WORKER_ID;
	}

	FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));
	int32 CellIndex = WorldPositionToCellIndex(Actor2DLocation);
	if (VirtualWorkerIdGrid.Num() > CellIndex)
	{
		return VirtualWorkerIdGrid[CellIndex];
	}

	return SpatialConstants::INVALID_VIRTUAL_WORKER_ID;
}

int32 UGridBasedLBStrategy::WorldPositionToCellIndex(const FVector2D& Location) const
{
	float ProgressY = (Location.Y - WorldWidthMin) / WorldWidth;
	int32 CellX = ProgressY * Cols;
	FMath::Clamp(CellX, 0, static_cast<int32>(Cols));

	float ProgressX = (Location.X - WorldHeightMin) / WorldHeight;
	int32 CellY = ProgressX * Rows;
	FMath::Clamp(CellY, 0, static_cast<int32>(Rows));

	return CellY * Cols + CellX;
}

void UGridBasedLBStrategy::PrintVirtualWorkerIdGrid() const
{
	FString Buffer;

	const int32 Size = VirtualWorkerIdGrid.Num();
	for (int32 i = 0; i < Size; ++i)
	{
		Buffer.Append(FString::Printf(TEXT("%d, "), VirtualWorkerIdGrid[i]));
		if ((i + 1) % Cols == 0)
		{
			UE_LOG(LogGridBasedLBStrategy, Log, TEXT("%s"), *Buffer);
			Buffer.Empty();
		}
	}
}
