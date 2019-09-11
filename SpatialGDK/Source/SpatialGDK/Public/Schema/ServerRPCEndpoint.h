// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Schema/Component.h"
#include "Schema/RPCPayload.h"
#include "Schema/RPCRingBuffer.h"
#include "SpatialConstants.h"
#include "SpatialGDKSettings.h"
#include "Utils/SchemaUtils.h"

#include <WorkerSDK/improbable/c_schema.h>
#include <WorkerSDK/improbable/c_worker.h>

namespace SpatialGDK
{

struct ServerRPCEndpoint : Component
{
	static const Worker_ComponentId ComponentId = SpatialConstants::SERVER_RPC_ENDPOINT_COMPONENT_ID;

	ServerRPCEndpoint()
		: ReliableRPCs(GetDefault<USpatialGDKSettings>()->GetRPCRingBufferSize(SCHEMA_ClientReliableRPC), 1)
		, UnreliableRPCs(GetDefault<USpatialGDKSettings>()->GetRPCRingBufferSize(SCHEMA_ClientUnreliableRPC), ReliableRPCs.SchemaFieldStart + ReliableRPCs.RingBufferSize)
		, LastExecutedReliableRPCFieldId(UnreliableRPCs.SchemaFieldStart + UnreliableRPCs.RingBufferSize)
		, LastExecutedUnreliableRPCFieldId(LastExecutedReliableRPCFieldId + 1)
	{

	}

	ServerRPCEndpoint(const Worker_ComponentData& Data)
		: ServerRPCEndpoint()
	{
		Schema_Object* EndpointObject = Schema_GetComponentDataFields(Data.schema_type);
		ReliableRPCs.ReadFromSchema(EndpointObject);
		UnreliableRPCs.ReadFromSchema(EndpointObject);

		LastExecutedReliableRPC = Schema_GetUint64(EndpointObject, LastExecutedReliableRPCFieldId);
		LastExecutedUnreliableRPC = Schema_GetUint64(EndpointObject, LastExecutedUnreliableRPCFieldId);
	}

	void ApplyComponentUpdate(const Worker_ComponentUpdate& Update) override
	{
		Schema_Object* EndpointObject = Schema_GetComponentUpdateFields(Update.schema_type);
		ReliableRPCs.ReadFromSchema(EndpointObject);
		UnreliableRPCs.ReadFromSchema(EndpointObject);

		if (Schema_GetUint64Count(EndpointObject, LastExecutedReliableRPCFieldId) > 0)
		{
			LastExecutedReliableRPC = Schema_GetUint64(EndpointObject, LastExecutedReliableRPCFieldId);
		}
		if (Schema_GetUint64Count(EndpointObject, LastExecutedUnreliableRPCFieldId) > 0)
		{
			LastExecutedUnreliableRPC = Schema_GetUint64(EndpointObject, LastExecutedUnreliableRPCFieldId);
		}
	}

	Worker_ComponentData CreateRPCEndpointData(const RPCsOnEntityCreation* RPCMap)
	{
		Worker_ComponentData Data{};
		Data.component_id = ComponentId;
		Data.schema_type = Schema_CreateComponentData(ComponentId);
		Schema_Object* ComponentObject = Schema_GetComponentDataFields(Data.schema_type);

		if (RPCMap != nullptr)
		{
			if (const TArray<RPCPayload>* ReliableRPCArray = RPCMap->Find(SCHEMA_ClientReliableRPC))
			{
				ReliableRPCs.WriteToSchema(ComponentObject, *ReliableRPCArray);
			}
			if (const TArray<RPCPayload>* UnreliableRPCArray = RPCMap->Find(SCHEMA_ClientUnreliableRPC))
			{
				UnreliableRPCs.WriteToSchema(ComponentObject, *UnreliableRPCArray);
			}
		}

		return Data;
	}

	// Create component update from RPCs and executed counters

	RPCRingBuffer ReliableRPCs;
	RPCRingBuffer UnreliableRPCs;
	uint64 LastExecutedReliableRPC = 0;
	uint64 LastExecutedUnreliableRPC = 0;

	const Schema_FieldId LastExecutedReliableRPCFieldId;
	const Schema_FieldId LastExecutedUnreliableRPCFieldId;
};

} // namespace SpatialGDK
