// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "Schema/RPCPayload.h"
#include "SpatialCommonTypes.h"
#include "SpatialConstants.h"

#include <WorkerSDK/improbable/c_worker.h>

#include "SpatialRingBufferManager.generated.h"

class USpatialNetDriver;
class USpatialSender;
class USpatialStaticComponentView;

UCLASS()
class USpatialRingBufferManager : public UObject
{
	GENERATED_BODY()

public:
	void Init(USpatialNetDriver* InNetDriver);

private:
	UPROPERTY()
	USpatialNetDriver* NetDriver;
	UPROPERTY()
	USpatialSender* Sender;
	UPROPERTY()
	USpatialStaticComponentView* StaticComponentView;

public:
	// If we're a client, this will be server RPCs; if we're a server, this will be client and multicast RPCs.
	TMap<Worker_EntityId_Key, TMap<ESchemaComponentType, TArray<SpatialGDK::RPCPayload>>> RPCsToSendMap;
	// If we're a client, this will be client and multicast RPCs; if we're a server, this will be server RPCs.
	TMap<Worker_EntityId_Key, TSet<ESchemaComponentType>> LastHandledRPCMap;
};
