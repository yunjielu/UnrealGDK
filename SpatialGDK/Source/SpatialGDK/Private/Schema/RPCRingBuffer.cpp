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

void RPCRingBuffer::ReadFromData(Schema_ComponentData* Data)
{
	Schema_Object* Fields = Schema_GetComponentDataFields(Data);

	for (uint32 i = 0; i < RingBufferSize; i++)
	{
		Schema_FieldId FieldId = SchemaFieldStart + i;
		if (Schema_GetObjectCount(Fields, FieldId) > 0)
		{
			RingBuffer[i].Emplace(Schema_GetObject(Fields, FieldId));
		}
		else
		{
			RingBuffer[i].Reset();
		}
	}

	LastSentRPCId = Schema_GetUint32(Fields, GetLastSentRPCIdFieldId());
}

void RPCRingBuffer::ReadFromUpdate(Schema_ComponentUpdate* Update)
{
	Schema_Object* Fields = Schema_GetComponentUpdateFields(Update);

	for (uint32 i = 0; i < RingBufferSize; i++)
	{
		Schema_FieldId FieldId = SchemaFieldStart + i;
		if (Schema_GetObjectCount(Fields, FieldId) > 0)
		{
			RingBuffer[i].Emplace(Schema_GetObject(Fields, FieldId));
		}
	}

	TArray<Schema_FieldId> ClearedFields;
	ClearedFields.SetNumUninitialized(Schema_GetComponentUpdateClearedFieldCount(Update));
	Schema_GetComponentUpdateClearedFieldList(Update, ClearedFields.GetData());

	for (Schema_FieldId ClearedId : ClearedFields)
	{
		if (ClearedId >= SchemaFieldStart && ClearedId < SchemaFieldStart + RingBufferSize)
		{
			RingBuffer[ClearedId - SchemaFieldStart].Reset();
		}
	}

	if (Schema_GetUint32Count(Fields, GetLastSentRPCIdFieldId()) > 0)
	{
		LastSentRPCId = Schema_GetUint32(Fields, GetLastSentRPCIdFieldId());
	}
}

} // namespace SpatialGDK
