// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "CoreMinimal.h"

#include "TestDefinitions.h"

#include "Schema/UnrealObjectRef.h"

#define UNREALOBJECTREF_TEST(TestName) \
	TEST(FUnrealObjectRef, TestName)

UNREALOBJECTREF_TEST(null_object_ref_maps_to_nullptr_and_vice_versa)
{
	UPackageMapStub* PackageMap = NewObject<UPackageMapStub>();

	bool bUnresolved = false;
	UObject* Object = FUnrealObjectRef::ToObjectPtr(FUnrealObjectRef::NULL_OBJECT_REF, PackageMap, bUnresolved);

	TestNull("Result of calling ToObjectPtr with NULL_OBJECT_REF", Object);
	TestFalse("NULL_OBJECT_REF is unresolved", bUnresolved);

	FUnrealObjectRef ObjectRef = FUnrealObjectRef::FromObjectPtr(nullptr, PackageMap);

	TestEqual("Result of calling FromObjectPtr with nullptr", ObjectRef, FUnrealObjectRef::NULL_OBJECT_REF);

	return true;
}

UNREALOBJECTREF_TEST(entity_offset_object_ref_maps_to_object)
{
	UPackageMapStub* PackageMap = NewObject<UPackageMapStub>();


}
