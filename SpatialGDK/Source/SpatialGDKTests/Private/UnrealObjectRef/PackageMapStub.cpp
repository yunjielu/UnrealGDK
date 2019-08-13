// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "PackageMapStub.h"

namespace
{
	FNetworkGUID VALID_NETGUID = FNetworkGUID(42);
	FUnrealObjectRef VALID_ENTITY_UNREAL_OBJECT_REF = FUnrealObjectRef(42, 0);
} // anonymous namespace

UObject* UPackageMapStub::GetObjectFromNetGUID(const FNetworkGUID& NetGUID, const bool bIgnoreMustBeMapped)
{
	if (NetGUID == VALID_NETGUID)
	{
		return ExpectedObject;
	}
	return nullptr;
}

FNetworkGUID UPackageMapStub::GetNetGUIDFromObject(const UObject* InObject) const
{
	if (InObject == ExpectedObject)
	{
		return VALID_NETGUID;
	}
	return FNetworkGUID();
}

FNetworkGUID UPackageMapStub::TryResolveObjectAsEntity(UObject* Object)
{
	ExpectedObject = Object;
	ExpectedObjectRef = VALID_ENTITY_UNREAL_OBJECT_REF;

	return VALID_NETGUID;
}

FNetworkGUID UPackageMapStub::ResolveStablyNamedObject(UObject* Object)
{
	ExpectedObject = Object;
	ExpectedObjectRef = FUnrealObjectRef(0, 0, Object->GetFName(), FUnrealObjectRef());

	return VALID_NETGUID;
}

FUnrealObjectRef UPackageMapStub::GetUnrealObjectRefFromNetGUID(const FNetworkGUID& NetGUID) const
{
	if (NetGUID == VALID_NETGUID)
	{
		return ExpectedObjectRef;
	}
	return FUnrealObjectRef::UNRESOLVED_OBJECT_REF;
}

FNetworkGUID UPackageMapStub::GetNetGUIDFromUnrealObjectRef(const FUnrealObjectRef& ObjectRef) const
{
	if (ObjectRef == ExpectedObjectRef)
	{
		return VALID_NETGUID;
	}
	return FNetworkGUID();
}

AActor* UPackageMapStub::GetSingletonByClassRef(const FUnrealObjectRef& SingletonClassRef)
{
	if (SingletonClassRef == ExpectedObjectRef)
	{
		return Cast<AActor>(ExpectedObject);
	}
	return nullptr;
}
