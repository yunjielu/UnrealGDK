// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "LoadBalancing/StandardGridBasedLBStrategy.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "Utils/SpatialActorUtils.h"
#include "GenericPlatform/GenericPlatformMath.h"

UStandardGridBasedLBStrategy::UStandardGridBasedLBStrategy()
	: Super()
{
}

void UStandardGridBasedLBStrategy::Init(const USpatialNetDriver* InNetDriver)
{
	Super::Init(InNetDriver);
}

void UStandardGridBasedLBStrategy::InitVirtualWorkerIdGrid()
{
	for (uint32 i = 1; i <= Rows * Cols; i++)
	{
		AddVirtualWorkerIdToGrid(i);
	}
}
