// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "CoreMinimal.h"

#include "TestDefinitions.h"

#include "Schema/UnrealObjectRef.h"

#define UNREALOBJECTREF_TEST(TestName) \
	TEST(FUnrealObjectRef, TestName)

UNREALOBJECTREF_TEST(GIVEN_a_null_object_ref_WHEN_converted_to_object_ptr_THEN_it_maps_to_nullptr)
{
	UPackageMapStub* PackageMap = NewObject<UPackageMapStub>();

	bool bUnresolved = false;
	UObject* Object = FUnrealObjectRef::ToObjectPtr(FUnrealObjectRef::NULL_OBJECT_REF, PackageMap, bUnresolved);

	TestEqual("NULL_OBJECT_REF should map to nullptr", Object, nullptr);
	TestEqual("NULL_OBJECT_REF should not be unresolved", bUnresolved, false);

	return true;
}

UNREALOBJECTREF_TEST(GIVEN_a_nullptr_WHEN_converted_to_unreal_object_ref_THEN_it_maps_to_null_object_ref)
{
	UPackageMapStub* PackageMap = NewObject<UPackageMapStub>();

	FUnrealObjectRef ObjectRef = FUnrealObjectRef::FromObjectPtr(nullptr, PackageMap);

	TestEqual("nullptr should map to NULL_OBJECT_REF", ObjectRef, FUnrealObjectRef::NULL_OBJECT_REF);

	return true;
}

UNREALOBJECTREF_TEST(GIVEN_an_entity_object_ref_WHEN_converted_to_object_ptr_THEN_it_maps_to_valid_pointer)
{
	UPackageMapStub* PackageMap = NewObject<UPackageMapStub>();

	AActor* DummyActor = NewObject<AActor>();


}
