// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Misc/Optional.h"

#include "Schema/RPCPayload.h"

#include <WorkerSDK/improbable/c_schema.h>
#include <WorkerSDK/improbable/c_worker.h>

namespace SpatialGDK
{

struct RPCRingBuffer
{
	RPCRingBuffer(uint32 InRingBufferSize, Schema_FieldId InSchemaFieldStart);

	void ReadFromData(Schema_ComponentData* Data);
	void ReadFromUpdate(Schema_ComponentUpdate* Update);

	inline Schema_FieldId GetLastSentRPCIdFieldId()
	{
		return SchemaFieldStart + RingBufferSize;
	}

	// Passed in constructor, can't change.
	const uint32 RingBufferSize;
	const Schema_FieldId SchemaFieldStart;

	TArray<TOptional<RPCPayload>> RingBuffer;

	uint32 LastSentRPCId;
};

} // namespace SpatialGDK
