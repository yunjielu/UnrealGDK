// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "Schema/RPCRingBuffer.h"

namespace SpatialGDK
{

RPCRingBuffer::RPCRingBuffer(uint32 InRingBufferSize, Schema_FieldId InSchemaFieldStart)
	: RingBufferSize(InRingBufferSize)
	, SchemaFieldStart(InSchemaFieldStart)
{
	RingBuffer.SetNum(RingBufferSize);
}

void RPCRingBuffer::ReadFromSchema(Schema_Object* Fields)
{
	for (uint32 i = 0; i < RingBufferSize; i++)
	{
		Schema_FieldId FieldId = SchemaFieldStart + i;
		if (Schema_GetObjectCount(Fields, FieldId) > 0)
		{
			RingBuffer[i].Emplace(Schema_GetObject(Fields, FieldId));
		}
	}

	if (Schema_GetUint64Count(Fields, GetLastSentRPCIdFieldId()) > 0)
	{
		LastSentRPCId = Schema_GetUint64(Fields, GetLastSentRPCIdFieldId());
	}
}

void RPCRingBuffer::WriteToSchema(Schema_Object* Fields, const TArray<RPCPayload>& RPCs)
{
	if (RPCs.Num() > 0)
	{
		// Write at most RingBufferSize RPCs.
		for (int i = FMath::Max<int>(0, RPCs.Num() - RingBufferSize); i < RPCs.Num(); i++)
		{
			Schema_FieldId FieldId = SchemaFieldStart + ((LastSentRPCId + i) % RingBufferSize);
			Schema_Object* RPCObject = Schema_AddObject(Fields, FieldId);
			RPCs[i].WriteToSchemaObject(RPCObject);
		}

		LastSentRPCId += RPCs.Num();
		Schema_AddUint64(Fields, GetLastSentRPCIdFieldId(), LastSentRPCId);
	}
}

} // namespace SpatialGDK
