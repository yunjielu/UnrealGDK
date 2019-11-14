// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"

enum class RPCType
{
	ClientReliable,
	ClientUnreliable,
	ServerReliable,
	ServerUnreliable,
	Multicast
};

struct EntityRPCType
{
	EntityRPCType(Worker_EntityId EntityId, RPCType Type)
		: EntityId(EntityId)
		, Type(Type)
	{}

	Worker_EntityId EntityId;
	RPCType Type;

	friend inline uint32 GetTypeHash(EntityRPCType Value)
	{
		return GetTypeHash(static_cast<int64>(Value.EntityId)) + 977u * GetTypeHash(Value.Type);
	}
};

struct EntityComponentId
{
	EntityComponentId(Worker_EntityId EntityId, Worker_ComponentId ComponentId)
		: EntityId(EntityId)
		, ComponentId(ComponentId)
	{}

	Worker_EntityId EntityId;
	Worker_ComponentId ComponentId;

	friend inline uint32 GetTypeHash(EntityComponentId Value)
	{
		return GetTypeHash(static_cast<int64>(Value.EntityId)) + 977u * GetTypeHash(static_cast<uint32>(Value.ComponentId));
	}
};

struct RingBufferDescriptor
{
	inline Schema_FieldId GetRingBufferElementFieldId(uint64 RPCId)
	{
		return SchemaFieldStart + ((RPCId - 1) % RingBufferSize);
	}

	inline Schema_FieldId GetLastSentRPCIdFieldId()
	{
		return SchemaFieldStart + RingBufferSize;
	}

	inline int32 GetCapacity(uint64 LastAckedRPCId, uint64 LastSentRPCId)
	{
		return LastAckedRPCId + RingBufferSize - LastSentRPCId;
	}

	uint32 RingBufferSize;
	Schema_FieldId SchemaFieldStart;
};

class SpatialRPCService
{
public:
	using ExtractRPCCallbackType = TFunction<bool(Worker_EntityId, RPCType, const RPCPayload&)>;

	SpatialRPCService(ExtractRPCCallbackType ExtractRPCCallback, const USpatialStaticComponentView* View);

	void PushRPC(Worker_EntityId EntityId, RPCType Type, RPCPayload Payload);

	TArray<Worker_ComponentUpdate> GetRPCsAndAcksToSend();
	TArray<Worker_ComponentData> GetRPCComponentsOnEntityCreation(Worker_EntityId EntityId);

	// Will also store acked IDs locally.
	// Calls ExtractRPCCallback for each RPC it extracts from a given component. If the callback returns false,
	// stops retrieving RPCs.
	void ExtractRPCsForEntity(Worker_EntityId EntityId, Worker_ComponentId ComponentId);

	/*{
		switch (ComponentId)
		{
		case CLIENT_ENDPOINT:
			ExtractForType(ClientReliable);
			ExtractForType(ClientUnreliable);

		}
	}*/

private:
	void ExtractRPCsForType(RPCType Type);

private:
	ExtractRPCCallbackType ExtractRPCCallback;
	const USpatialStaticComponentView* View;

	// This is local, not written into schema.
	TMap<Worker_EntityId_Key, uint64> LastSeenMulticastRPCIds;

	// Stored here for things we have authority over.
	TMap<EntityRPCType, uint64> LastAckedRPCIds;
	TMap<EntityRPCType, uint64> LastSentRPCIds;

	TMap<EntityComponentId, Schema_ComponentData*> PendingRPCsOnEntityCreation;

	TMap<EntityComponentId, Schema_ComponentUpdate*> PendingComponentUpdatesToSend;
};



//GetAck(RPCType)
//{
//	switch (RPCType)
//	{
//	case ClientReliable:
//	case ClientUnreliable:
//		Ack = View->GetComponent<ClientEndpoint>(Entity)->GetAck(RPCType);
//	case ServerReliable:
//	case ServerUnreliable:
//		Ack = View->GetComponent<ServerEndpoint>(Entity)->GetAck(RPCType);
//	case Multicast:
//		No;
//	}
//}
//
//GetBuffer(RPCType)
//{
//	switch (RPCType)
//	{
//	case ClientReliable:
//	case ClientUnreliable:
//		Buffer = View->GetComponent<ServerEndpoint>(Entity)->GetBuffer(RPCType);
//	case ServerReliable:
//	case ServerUnreliable:
//		Buffer = View->GetComponent<ClientEndpoint>(Entity)->GetBuffer(RPCType);
//	case Multicast:
//		GetBuffer;
//	}
//}
