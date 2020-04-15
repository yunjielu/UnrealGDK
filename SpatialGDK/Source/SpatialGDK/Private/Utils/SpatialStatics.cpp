// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "Utils/SpatialStatics.h"

#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "EngineClasses/SpatialPackageMapClient.h"
#include "GeneralProjectSettings.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SpatialConstants.h"
#include "EngineClasses/SpatialGameInstance.h"
#include "SpatialGDKSettings.h"
#include "Utils/InspectionColors.h"
#include "Utils/SpatialLayerManager.h"

DEFINE_LOG_CATEGORY(LogSpatial);

bool USpatialStatics::IsSpatialNetworkingEnabled()
{
    return GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking();
}

SpatialLayerManager* USpatialStatics::GetLayerManager(const UObject* WorldContext)
{
	if (const UWorld* World = WorldContext->GetWorld())
	{
		if (const USpatialGameInstance* SpatialGameInstance = Cast<USpatialGameInstance>(World->GetGameInstance()))
		{
			check(SpatialGameInstance->LayerManager.IsValid());
			return SpatialGameInstance->LayerManager.Get();
		}
	}
	return nullptr;
}

FName USpatialStatics::GetCurrentWorkerType(const UObject* WorldContext)
{
	if (const UWorld* World = WorldContext->GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSpatialWorkerType();
		}
	}

	return NAME_None;
}

bool USpatialStatics::GetWorkerFlag(const UObject* WorldContext, const FString& InFlagName, FString& OutFlagValue)
{
	if (const UWorld* World = WorldContext->GetWorld())
	{
		if (const USpatialNetDriver* SpatialNetDriver = Cast<USpatialNetDriver>(World->GetNetDriver()))
		{
			if (const USpatialWorkerFlags* SpatialWorkerFlags = SpatialNetDriver->SpatialWorkerFlags) 
			{
				return SpatialWorkerFlags->GetWorkerFlag(InFlagName, OutFlagValue);
			}
		}
	}

	return false;
}

TArray<FDistanceFrequencyPair> USpatialStatics::GetNCDDistanceRatios()
{
	return GetDefault<USpatialGDKSettings>()->InterestRangeFrequencyPairs;
}

float USpatialStatics::GetFullFrequencyNetCullDistanceRatio()
{
	return GetDefault<USpatialGDKSettings>()->FullFrequencyNetCullDistanceRatio;
}

FColor USpatialStatics::GetInspectorColorForWorkerName(const FString& WorkerName)
{
	return SpatialGDK::GetColorForWorkerName(WorkerName);
}

bool USpatialStatics::IsSpatialOffloadingEnabled()
{
    return IsSpatialNetworkingEnabled() && GetDefault<USpatialGDKSettings>()->bEnableMultiWorker;
}

bool USpatialStatics::IsLayerOwnerForActor(const AActor* Actor)
{
	if (Actor == nullptr)
	{
		return false;
	}

	const AActor* EffectiveActor = Actor;
	while (EffectiveActor->bUseNetOwnerActorGroup && EffectiveActor->GetOwner() != nullptr)
	{
		EffectiveActor = EffectiveActor->GetOwner();
	}

	return IsLayerOwnerForClass(EffectiveActor, EffectiveActor->GetClass());
}

bool USpatialStatics::IsLayerOwnerForClass(const UObject* WorldContextObject, const TSubclassOf<AActor> ActorClass)
{
	if (SpatialLayerManager* LayerManager = GetLayerManager(WorldContextObject))
	{
		const FName ClassWorkerType = LayerManager->GetWorkerTypeForClass(ActorClass);
		const FName CurrentWorkerType = GetCurrentWorkerType(WorldContextObject);
		return ClassWorkerType == CurrentWorkerType;
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		return World->GetNetMode() != NM_Client;
	}

	return false;
}

bool USpatialStatics::IsLayerOwner(const UObject* WorldContextObject, const FName Layer)
{
	if (SpatialLayerManager* LayerManager = GetLayerManager(WorldContextObject))
	{
		const FName LayerWorkerType = LayerManager->GetWorkerTypeForLayer(Layer);
		const FName CurrentWorkerType = GetCurrentWorkerType(WorldContextObject);
		return LayerWorkerType == CurrentWorkerType;
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		return World->GetNetMode() != NM_Client;
	}

	return false;
}

FName USpatialStatics::GetLayerForActor(const AActor* Actor)
{
	if (SpatialLayerManager* LayerManager = GetLayerManager(Actor))
	{
		const AActor* EffectiveActor = Actor;
		while (EffectiveActor->bUseNetOwnerActorGroup && EffectiveActor->GetOwner() != nullptr)
		{
			EffectiveActor = EffectiveActor->GetOwner();
		}

		return LayerManager->GetLayerForClass(EffectiveActor->GetClass());
	}

	return SpatialConstants::DefaultLayer;
}

FName USpatialStatics::GetLayerForClass(const UObject* WorldContextObject, const TSubclassOf<AActor> ActorClass)
{
	if (SpatialLayerManager* LayerManager = GetLayerManager(WorldContextObject))
	{
		return LayerManager->GetLayerForClass(ActorClass);
	}

	return SpatialConstants::DefaultLayer;
}

void USpatialStatics::PrintStringSpatial(UObject* WorldContextObject, const FString& InString /*= FString(TEXT("Hello"))*/, bool bPrintToScreen /*= true*/, FLinearColor TextColor /*= FLinearColor(0.0, 0.66, 1.0)*/, float Duration /*= 2.f*/)
{
	// This will be logged in the SpatialOutput so we don't want to double log this, therefore bPrintToLog is false.
	UKismetSystemLibrary::PrintString(WorldContextObject, InString, bPrintToScreen, false /*bPrintToLog*/, TextColor, Duration);

	// By logging to LogSpatial we will print to the spatial os runtime.
	UE_LOG(LogSpatial, Log, TEXT("%s"), *InString);
}

void USpatialStatics::PrintTextSpatial(UObject* WorldContextObject, const FText InText /*= INVTEXT("Hello")*/, bool bPrintToScreen /*= true*/, FLinearColor TextColor /*= FLinearColor(0.0, 0.66, 1.0)*/, float Duration /*= 2.f*/)
{
	PrintStringSpatial(WorldContextObject, InText.ToString(), bPrintToScreen, TextColor, Duration);
}

int64 USpatialStatics::GetActorEntityId(const AActor* Actor)
{
	check(Actor);
	if (const USpatialNetDriver* SpatialNetDriver = Cast<USpatialNetDriver>(Actor->GetNetDriver()))
	{
		return static_cast<int64>(SpatialNetDriver->PackageMap->GetEntityIdFromObject(Actor));
	}
	return 0;
}

FString USpatialStatics::EntityIdToString(int64 EntityId)
{
	if (EntityId <= SpatialConstants::INVALID_ENTITY_ID)
	{
		return FString("Invalid");
	}

	return FString::Printf(TEXT("%lld"), EntityId);
}

FString USpatialStatics::GetActorEntityIdAsString(const AActor* Actor)
{
	return EntityIdToString(GetActorEntityId(Actor));
}
