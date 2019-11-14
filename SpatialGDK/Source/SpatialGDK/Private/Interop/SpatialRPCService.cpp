// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "Interop/SpatialRPCService.h"

Worker_ComponentId RPCTypeToBufferComponentId(RPCType Type)
{
	switch (Type)
	{
	case RPCType::ClientReliable:
	case RPCType::ClientUnreliable:
		return SpatialConstants::SERVER_RPC_ENDPOINT_RING_BUFFER_COMPONENT_ID;
	case RPCType::ServerReliable:
	case RPCType::ServerUnreliable:
		return SpatialConstants::CLIENT_RPC_ENDPOINT_RING_BUFFER_COMPONENT_ID;
	case RPCType::Multicast:
		return SpatialConstants::NETMULTICAST_RPCS_RING_BUFFER_COMPONENT_ID;
	default:
		return SpatialConstants::INVALID_COMPONENT_ID;
	}
}

Worker_ComponentId RPCTypeToAckComponentId(RPCType)
{
	switch (Type)
	{
	case RPCType::ClientReliable:
	case RPCType::ClientUnreliable:
		return SpatialConstants::CLIENT_RPC_ENDPOINT_RING_BUFFER_COMPONENT_ID;
	case RPCType::ServerReliable:
	case RPCType::ServerUnreliable:
		return SpatialConstants::SERVER_RPC_ENDPOINT_RING_BUFFER_COMPONENT_ID;
	default:
		return SpatialConstants::INVALID_COMPONENT_ID;
	}
}

SpatialRPCService::SpatialRPCService(ExtractRPCCallbackType ExtractRPCCallback, const USpatialStaticComponentView* View)
	: View(View)
	, ExtractRPCCallback(ExtractRPCCallback)
{
}

void SpatialRPCService::PushRPC(Worker_EntityId EntityId, RPCType Type, RPCPayload Payload)
{
	Worker_ComponentId BufferComponentId = RPCTypeToBufferComponentId(Type);
	EntityComponentId EntityComponent = EntityComponentId(EntityId, BufferComponentId);
	EntityRPCType EntityType = EntityRPCType(EntityId, Type);;

	Schema_Object* Object;
	uint64 LastAckedRPCId;
	if (View->HasComponent(EntityId, BufferComponentId))
	{
		Schema_ComponentUpdate** ComponentUpdatePtr = PendingComponentUpdatesToSend.Find(EntityComponent);
		if (ComponentUpdatePtr == nullptr)
		{
			ComponentUpdatePtr = &PendingComponentUpdatesToSend.Add(EntityComponent, Schema_CreateComponentUpdate());
		}
		Object = Schema_GetComponentUpdateFields(*ComponentUpdatePtr);

		if (Type == RPCType::Multicast)
		{
			// Assume all multicast RPCs are auto-acked.
			LastAckedRPCId = LastSentRPCIds[EntityType];
		}
		else
		{
			LastAckedRPCId = LastAckedRPCIds[EntityType];
		}
	}
	else
	{
		// If the entity isn't in the view, we assume this RPC was called before
		// CreateEntityRequest, so we put it into a component data object.
		Schema_ComponentData** ComponentDataPtr = PendingRPCsOnEntityCreation.Find(EntityComponent);
		if (ComponentDataPtr == nullptr)
		{
			ComponentDataPtr = &PendingRPCsOnEntityCreation.Add(EntityComponent, Schema_CreateComponentData());
		}
		Object = Schema_GetComponentDataFields(*ComponentUpdatePtr);

		LastAckedRPCId = 0;
	}

	RingBufferDescriptor Descriptor = GetRingBufferDescriptor(Type);

	// We shouldn't have authority over the component that has the acks.
	Worker_ComponentId AckComponentId = RPCTypeToAckComponentId(Type);
	if (Type != RPCType::Multicast && View->HasAuthority(EntityId, AckComponentId))
	{
		UE_LOG(LogSpatialRPCService, Error, TEXT("SpatialRPCService::PushRPC: Has authority on ack component when sending RPC. Entity: %lld, component: %d"), EntityId, AckComponentId);
		return;
	}

	uint64 NewRPCId = LastSentRPCIds[EntityType] + 1;


	LastSentRPCIds[EntityType] = NewRPCId;
}

TArray<Worker_ComponentUpdate> SpatialRPCService::GetRPCsAndAcksToSend()
{

}

TArray<Worker_ComponentData> SpatialRPCService::GetRPCComponentsOnEntityCreation(Worker_EntityId EntityId)
{

}

void SpatialRPCService::ExtractRPCsForEntity(Worker_EntityId EntityId, Worker_ComponentId ComponentId)
{

}

