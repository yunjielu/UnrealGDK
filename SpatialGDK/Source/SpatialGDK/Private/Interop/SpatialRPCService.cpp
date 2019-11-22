// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "Interop/SpatialRPCService.h"

DEFINE_LOG_CATEGORY(LogSpatialRPCService);

/* Don't need these if we have this mapping in RingBufferDescriptor.
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

bool ShouldQueueOverflowed(RPCType Type)
{
	switch (Type)
	{
	case RPCType::ClientReliable:
	case RPCType::ServerReliable:
		return true;
	default:
		return false;
	}
}
*/

RingBufferDescriptor GetRingBufferDescriptor(RPCType Type)
{
	// Client endpoint looks like:
	//   [ Server Reliable RPC Ring Buffer + Last Sent ID, Server Unreliable RPC Ring Buffer + Last Sent ID, Client Reliable Ack, Client Unreliable Ack ]
	// Server endpoint:
	//   [ Client Reliable RPC Ring Buffer + Last Sent ID, Client Unreliable RPC Ring Buffer + Last Sent ID, Server Reliable Ack, Server Unreliable Ack ]
	// Multicast endpoint:
	//   [ Multicast RPC Ring Buffer + Last Sent ID ]
	//
	// I want the descriptors to be defined in this form, but have the opposite mapping: from RPC Type to components and field IDs, e.g.:
	//
	// Client Unreliable RPCs: {
	//   Ring Buffer Component: SERVER_ENDPOINT,
	//   Ring Buffer Field ID start: Client Reliable Last Sent ID Field ID + 1,
	//   Ring Buffer size: get from settings,
	//   Last Sent ID Field ID: Field ID start + Ring Buffer size,
	//   Ack Component: CLIENT_ENDPOINT,
	//   Ack Field ID: Client Reliable Ack Field ID + 1,
	//   Should queue overflowed: false
	// }

	return while you can;
}

SpatialRPCService::SpatialRPCService(ExtractRPCCallbackType ExtractRPCCallback, const USpatialStaticComponentView* View)
	: View(View)
	, ExtractRPCCallback(ExtractRPCCallback)
{
}

void SpatialRPCService::PushRPC(Worker_EntityId EntityId, RPCType Type, RPCPayload Payload)
{
	RingBufferDescriptor Descriptor = GetRingBufferDescriptor(Type);

	EntityComponentId EntityComponent = EntityComponentId(EntityId, Descriptor.RingBufferComponentId);
	EntityRPCType EntityType = EntityRPCType(EntityId, Type);

	if (Descriptor.bShouldQueueOverflowed && OverflowedRPCs.Contains(EntityComponent))
	{
		// Log and add to overflowed.
		AddOverflowedRPC(EntityComponent, MoveTemp(Payload));
	}

	Schema_Object* EndpointObject;
	uint64 LastAckedRPCId;
	if (View->HasComponent(EntityId, Descriptor.RingBufferComponentId))
	{
		Schema_ComponentUpdate** ComponentUpdatePtr = PendingComponentUpdatesToSend.Find(EntityComponent);
		if (ComponentUpdatePtr == nullptr)
		{
			ComponentUpdatePtr = &PendingComponentUpdatesToSend.Add(EntityComponent, Schema_CreateComponentUpdate());
		}
		EndpointObject = Schema_GetComponentUpdateFields(*ComponentUpdatePtr);

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
		EndpointObject = Schema_GetComponentDataFields(*ComponentUpdatePtr);

		LastAckedRPCId = 0;
	}

	// We shouldn't have authority over the component that has the acks.
	if (Type != RPCType::Multicast && View->HasAuthority(EntityId, Descriptor.AckComponentId))
	{
		UE_LOG(LogSpatialRPCService, Error, TEXT("SpatialRPCService::PushRPC: Has authority on ack component when sending RPC. Entity: %lld, component: %d"), EntityId, Descriptor.AckComponentId);
		return;
	}

	uint64 NewRPCId = LastSentRPCIds[EntityType] + 1;

	// Check capacity.
	if (Descriptor.HasCapacity(LastAckedRPCId, NewRPCId))
	{
		Schema_Object* RPCObject = Schema_AddObject(EndpointObject, Descriptor.GetRingBufferElementFieldId(NewRPCId));
		RPCPayload::WriteToSchemaObject(RPCObject, Payload.Offset, Payload.Index, Payload.PayloadData.GetData(), Payload.PayloadData.Num());

		Schema_ClearField(EndpointObject, Descriptor.GetLastSentRPCIdFieldId());
		Schema_AddUint64(EndpointObject, Descriptor.GetLastSentRPCIdFieldId(), NewRPCId);

		LastSentRPCIds[EntityType] = NewRPCId;
	}
	else
	{
		//UE_LOG(LogSpatialRPCService, Warning, TEXT("SpatialRPCService::PushRPC: Queuing RPC for object: %s %s"), *TargetObject->GetPathName(), *Function->GetName());

		// Overflowed
		if (Descriptor.bShouldQueueOverflowed)
		{
			AddOverflowedRPC(EntityComponent, MoveTemp(Payload));
		}
	}
}

TArray<SpatialRPCService::UpdateToSend> SpatialRPCService::GetRPCsAndAcksToSend()
{
	TArray<SpatialRPCService::UpdateToSend> UpdatesToSend;

	for (auto& It : PendingComponentUpdatesToSend)
	{
		SpatialRPCService::UpdateToSend& UpdateToSend = UpdatesToSend.AddZeroed_GetRef();
		UpdateToSend.EntityId = It.Key.EntityId;
		UpdateToSend.Update.component_id = It.Key.ComponentId;
		UpdateToSend.Update.schema_type = It.Value;
	}

	PendingComponentUpdatesToSend.Empty();

	return UpdatesToSend;
}

TArray<Worker_ComponentData> SpatialRPCService::GetRPCComponentsOnEntityCreation(Worker_EntityId EntityId)
{
	static Worker_ComponentId EndpointComponentIds[] = {
		SpatialConstants::CLIENT_RPC_ENDPOINT_RING_BUFFER_COMPONENT_ID,
		SpatialConstants::SERVER_RPC_ENDPOINT_RING_BUFFER_COMPONENT_ID,
		SpatialConstants::NETMULTICAST_RPCS_RING_BUFFER_COMPONENT_ID
	};

	TArray<Worker_ComponentData> Components;

	for (Worker_ComponentId EndpointComponentId : EndpointComponentIds)
	{
		EntityComponentId EntityComponent = EntityComponentId(EntityId, EntityComponentId);

		Worker_ComponentData& Component = Components.AddZeroed_GetRef();
		Component.component_id = EndpointComponentId;
		if (Schema_ComponentData** ComponentData = PendingRPCsOnEntityCreation.Find(EntityComponent))
		{
			Component.schema_type = *ComponentData;
			PendingRPCsOnEntityCreation.Remove(EntityComponent);
		}
		else
		{
			Component.schema_type = Schema_CreateComponentData();
		}
	}

	return Components;
}

void SpatialRPCService::ExtractRPCsForEntity(Worker_EntityId EntityId, Worker_ComponentId ComponentId)
{

}

void SpatialRPCService::AddOverflowedRPC(EntityComponentId EntityComponent, RPCPayload Payload)
{
	OverflowedRPCs.FindOrAdd(EntityComponent).Add(Payload);
}

