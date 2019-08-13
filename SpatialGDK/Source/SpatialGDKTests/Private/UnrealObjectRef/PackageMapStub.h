// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "EngineClasses/SpatialPackageMapClient.h"

#include "PackageMapStub.generated.h"

UCLASS()
class UPackageMapStub : public USpatialPackageMapClient
{
	GENERATED_BODY()

public:
	virtual UObject* GetObjectFromNetGUID(const FNetworkGUID& NetGUID, const bool bIgnoreMustBeMapped) override;
	virtual FNetworkGUID GetNetGUIDFromObject(const UObject* InObject) const override;

	virtual FNetworkGUID TryResolveObjectAsEntity(UObject* Object) override;

	virtual FNetworkGUID ResolveStablyNamedObject(UObject* Object) override;

	virtual FUnrealObjectRef GetUnrealObjectRefFromNetGUID(const FNetworkGUID& NetGUID) const override;
	virtual FNetworkGUID GetNetGUIDFromUnrealObjectRef(const FUnrealObjectRef& ObjectRef) const override;

	virtual AActor* GetSingletonByClassRef(const FUnrealObjectRef& SingletonClassRef) override;

	UObject* ExpectedObject = nullptr;
	FUnrealObjectRef ExpectedObjectRef = FUnrealObjectRef::NULL_OBJECT_REF;
};
