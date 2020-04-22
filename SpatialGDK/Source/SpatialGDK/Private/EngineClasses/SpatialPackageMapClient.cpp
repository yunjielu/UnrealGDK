// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "EngineClasses/SpatialPackageMapClient.h"

#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

#include "EngineClasses/SpatialActorChannel.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "EngineClasses/SpatialNetBitReader.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Interop/SpatialReceiver.h"
#include "Interop/SpatialSender.h"
#include "Schema/UnrealObjectRef.h"
#include "SpatialConstants.h"
#include "Utils/EntityPool.h"
#include "Utils/SchemaOption.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY(LogSpatialPackageMap);

void USpatialPackageMapClient::Init(USpatialNetDriver* NetDriver, FTimerManager* TimerManager)
{
	bIsServer = NetDriver->IsServer();
	// Entity Pools should never exist on clients
	if (bIsServer)
	{
		EntityPool = NewObject<UEntityPool>();
		EntityPool->Init(NetDriver, TimerManager);
	}
}

void GetSubobjects(UObject* ParentObject, TArray<UObject*>& InSubobjects)
{
	InSubobjects.Empty();
	ForEachObjectWithOuter(ParentObject, [&InSubobjects](UObject* Object)
	{
		// Objects can only be allocated NetGUIDs if this is true.
		if (Object->IsSupportedForNetworking() && !Object->IsPendingKill() && !Object->IsEditorOnly())
		{
			// Walk up the outer chain and ensure that no object is PendingKill. This is required because although
			// EInternalObjectFlags::PendingKill prevents objects that are PendingKill themselves from getting added
			// to the list, it'll still add children of PendingKill objects. This then causes an assertion within 
			// FNetGUIDCache::RegisterNetGUID_Server where it again iterates up the object's owner chain, assigning
			// ids and ensuring that no object is set to PendingKill in the process.
			UObject* Outer = Object->GetOuter();
			while (Outer != nullptr)
			{
				if (Outer->IsPendingKill())
				{
					return;
				}
				Outer = Outer->GetOuter();
			}
			InSubobjects.Add(Object);
		}
	}, true, RF_NoFlags, EInternalObjectFlags::PendingKill);

	InSubobjects.StableSort([](UObject& A, UObject& B)
	{
		return A.GetName() < B.GetName();
	});
}

Worker_EntityId USpatialPackageMapClient::AllocateEntityIdAndResolveActor(AActor* Actor)
{
	check(Actor);
	checkf(bIsServer, TEXT("Tried to allocate an Entity ID on the client, this shouldn't happen."));

	if (!IsEntityPoolReady())
	{
		UE_LOG(LogSpatialPackageMap, Error, TEXT("EntityPool must be ready when resolving an Actor: %s"), *Actor->GetName());
		return SpatialConstants::INVALID_ENTITY_ID;
	}

	Worker_EntityId EntityId = EntityPool->GetNextEntityId();
	if (EntityId == SpatialConstants::INVALID_ENTITY_ID)
	{
		UE_LOG(LogSpatialPackageMap, Error, TEXT("Unable to retrieve an Entity ID for Actor: %s"), *Actor->GetName());
		return EntityId;
	}

	// Register Actor with package map since we know what the entity id is.
	if (!ResolveEntityActor(Actor, EntityId))
	{
		UE_LOG(LogSpatialPackageMap, Error, TEXT("Unable to resolve an Entity for Actor: %s"), *Actor->GetName());
		return SpatialConstants::INVALID_ENTITY_ID;
	}

	return EntityId;
}

FNetworkGUID USpatialPackageMapClient::TryResolveObjectAsEntity(UObject* Value)
{
	FNetworkGUID NetGUID;

	if (!bIsServer)
	{
		return NetGUID;
	}

	AActor* Actor = Value->IsA<AActor>() ? Cast<AActor>(Value) : Value->GetTypedOuter<AActor>();
	if (Actor == nullptr)
	{
		return NetGUID;
	}

	if (!Actor->GetIsReplicated())
	{
		return NetGUID;
	}

	// Resolve as an entity if it is an unregistered actor
	if (Actor->Role == ROLE_Authority && GetEntityIdFromObject(Actor) == SpatialConstants::INVALID_ENTITY_ID)
	{
		Worker_EntityId EntityId = AllocateEntityIdAndResolveActor(Actor);
		if (EntityId != SpatialConstants::INVALID_ENTITY_ID)
		{
			// Mark this entity ID as pending creation (checked in USpatialActorChannel::SetChannelActor).
			PendingCreationEntityIds.Add(EntityId);
		}

		NetGUID = GetNetGUIDFromObject(Value);
	}

	return NetGUID;
}

bool USpatialPackageMapClient::IsEntityIdPendingCreation(Worker_EntityId EntityId) const
{
	return PendingCreationEntityIds.Contains(EntityId);
}

void USpatialPackageMapClient::RemovePendingCreationEntityId(Worker_EntityId EntityId)
{
	PendingCreationEntityIds.Remove(EntityId);
}

bool USpatialPackageMapClient::ResolveEntityActor(AActor* Actor, Worker_EntityId EntityId)
{
	FSpatialNetGUIDCache* SpatialGuidCache = static_cast<FSpatialNetGUIDCache*>(GuidCache.Get());
	FNetworkGUID NetGUID = SpatialGuidCache->GetNetGUIDFromEntityId(EntityId);

	// check we haven't already assigned a NetGUID to this object
	if (!NetGUID.IsValid())
	{
		NetGUID = SpatialGuidCache->AssignNewEntityActorNetGUID(Actor, EntityId);
	}

	if (GetEntityIdFromObject(Actor) != EntityId)
	{
		UE_LOG(LogSpatialPackageMap, Error, TEXT("ResolveEntityActor failed for Actor: %s with NetGUID: %s and passed entity ID: %lld"), *Actor->GetName(), *NetGUID.ToString(), EntityId);
		return false;
	}

	return NetGUID.IsValid();
}

void USpatialPackageMapClient::ResolveSubobject(UObject* Object, const FUnrealObjectRef& ObjectRef)
{
	FSpatialNetGUIDCache* SpatialGuidCache = static_cast<FSpatialNetGUIDCache*>(GuidCache.Get());
	FNetworkGUID NetGUID = SpatialGuidCache->GetNetGUIDFromUnrealObjectRef(ObjectRef);

	if (!NetGUID.IsValid())
	{
		SpatialGuidCache->AssignNewSubobjectNetGUID(Object, ObjectRef);
	}
}

void USpatialPackageMapClient::RemoveEntityActor(Worker_EntityId EntityId)
{
	FSpatialNetGUIDCache* SpatialGuidCache = static_cast<FSpatialNetGUIDCache*>(GuidCache.Get());

	if (SpatialGuidCache->GetNetGUIDFromEntityId(EntityId).IsValid())
	{
		SpatialGuidCache->RemoveEntityNetGUID(EntityId);
	}
}

void USpatialPackageMapClient::RemoveSubobject(const FUnrealObjectRef& ObjectRef)
{
	FSpatialNetGUIDCache* SpatialGuidCache = static_cast<FSpatialNetGUIDCache*>(GuidCache.Get());

	if (SpatialGuidCache->GetNetGUIDFromUnrealObjectRef(ObjectRef).IsValid())
	{
		SpatialGuidCache->RemoveSubobjectNetGUID(ObjectRef);
	}
}

void USpatialPackageMapClient::UnregisterActorObjectRefOnly(const FUnrealObjectRef& ObjectRef)
{
	FSpatialNetGUIDCache* SpatialGuidCache = static_cast<FSpatialNetGUIDCache*>(GuidCache.Get());
	SpatialGuidCache->UnregisterActorObjectRefOnly(ObjectRef);
}

FNetworkGUID USpatialPackageMapClient::ResolveStablyNamedObject(UObject* Object)
{
	FSpatialNetGUIDCache* SpatialGuidCache = static_cast<FSpatialNetGUIDCache*>(GuidCache.Get());
	return SpatialGuidCache->AssignNewStablyNamedObjectNetGUID(Object);
}

FUnrealObjectRef USpatialPackageMapClient::GetUnrealObjectRefFromNetGUID(const FNetworkGUID & NetGUID) const
{
	FSpatialNetGUIDCache* SpatialGuidCache = static_cast<FSpatialNetGUIDCache*>(GuidCache.Get());
	return SpatialGuidCache->GetUnrealObjectRefFromNetGUID(NetGUID);
}

FNetworkGUID USpatialPackageMapClient::GetNetGUIDFromUnrealObjectRef(const FUnrealObjectRef & ObjectRef) const
{
	FSpatialNetGUIDCache* SpatialGuidCache = static_cast<FSpatialNetGUIDCache*>(GuidCache.Get());
	return SpatialGuidCache->GetNetGUIDFromUnrealObjectRef(ObjectRef);
}

FNetworkGUID USpatialPackageMapClient::GetNetGUIDFromEntityId(const Worker_EntityId& EntityId) const
{
	FSpatialNetGUIDCache* SpatialGuidCache = static_cast<FSpatialNetGUIDCache*>(GuidCache.Get());
	FUnrealObjectRef ObjectRef(EntityId, 0);
	return GetNetGUIDFromUnrealObjectRef(ObjectRef);
}

TWeakObjectPtr<UObject> USpatialPackageMapClient::GetObjectFromUnrealObjectRef(const FUnrealObjectRef& ObjectRef)
{
	FNetworkGUID NetGUID = GetNetGUIDFromUnrealObjectRef(ObjectRef);
	if (NetGUID.IsValid() && !NetGUID.IsDefault())
	{
		return GetObjectFromNetGUID(NetGUID, true);
	}

	return nullptr;
}

TWeakObjectPtr<UObject> USpatialPackageMapClient::GetObjectFromEntityId(const Worker_EntityId& EntityId)
{
	return GetObjectFromUnrealObjectRef(FUnrealObjectRef(EntityId, 0));
}

FUnrealObjectRef USpatialPackageMapClient::GetUnrealObjectRefFromObject(const UObject* Object)
{
	if (Object == nullptr)
	{
		return FUnrealObjectRef::NULL_OBJECT_REF;
	}

	FNetworkGUID NetGUID = GetNetGUIDFromObject(Object);

	return GetUnrealObjectRefFromNetGUID(NetGUID);
}

Worker_EntityId USpatialPackageMapClient::GetEntityIdFromObject(const UObject* Object)
{
	if (Object == nullptr)
	{
		return SpatialConstants::INVALID_ENTITY_ID;
	}

	FNetworkGUID NetGUID = GetNetGUIDFromObject(Object);
	return GetUnrealObjectRefFromNetGUID(NetGUID).Entity;
}

bool USpatialPackageMapClient::CanClientLoadObject(UObject* Object)
{
	FNetworkGUID NetGUID = GetNetGUIDFromObject(Object);
	return GuidCache->CanClientLoadObject(Object, NetGUID);
}

bool USpatialPackageMapClient::IsEntityPoolReady() const
{
	return (EntityPool != nullptr) && (EntityPool->IsReady());
}

bool USpatialPackageMapClient::SerializeObject(FArchive& Ar, UClass* InClass, UObject*& Obj, FNetworkGUID *OutNetGUID)
{
	// Super::SerializeObject is not called here on purpose
	if (Ar.IsSaving())
	{
		Ar << Obj;
		return true;
	}

	FSpatialNetBitReader& Reader = static_cast<FSpatialNetBitReader&>(Ar);
	bool bUnresolved = false;
	Obj = Reader.ReadObject(bUnresolved);

	return !bUnresolved;
}

const FClassInfo* USpatialPackageMapClient::TryResolveNewDynamicSubobjectAndGetClassInfo(UObject* Object)
{
	AActor* Actor = Object ? Object->GetTypedOuter<AActor>() : nullptr;
	Worker_EntityId EntityId = GetEntityIdFromObject(Actor);

	if (EntityId != SpatialConstants::INVALID_ENTITY_ID)
	{
		FUnrealObjectRef Ref = GetUnrealObjectRefFromObject(Object);
		if (Ref.IsValid())
		{
			UE_LOG(LogSpatialPackageMap, Error, TEXT("Trying to resolve a dynamic subobject twice! Object %s, Actor %s, EntityId %d."), *GetNameSafe(Object), *GetNameSafe(Actor), EntityId);
			return nullptr;
		}

		const FClassInfo* Info = Cast<USpatialNetDriver>(GuidCache->Driver)->ClassInfoManager->GetClassInfoForNewSubobject(Object, EntityId, this);

		// If we don't get the info, an error is logged in the above function, that we have exceeded the maximum number of dynamic subobjects on the entity
		if (Info != nullptr)
		{
			ResolveSubobject(Object, FUnrealObjectRef(EntityId, Info->SchemaComponents[SCHEMA_Data]));
		}

		return Info;
	}

	UE_LOG(LogSpatialPackageMap, Error, TEXT("While trying to resolve a new dynamic subobject %s, the parent actor %s was not resolved."), *GetNameSafe(Object), *GetNameSafe(Actor));

	return nullptr;
}

FSpatialNetGUIDCache::FSpatialNetGUIDCache(USpatialNetDriver* InDriver)
	: FNetGUIDCache(InDriver)
{
}

FNetworkGUID FSpatialNetGUIDCache::AssignNewEntityActorNetGUID(AActor* Actor, Worker_EntityId EntityId)
{
	check(EntityId > 0);

	USpatialNetDriver* SpatialNetDriver = Cast<USpatialNetDriver>(Driver);
	USpatialReceiver* Receiver = SpatialNetDriver->Receiver;

	FNetworkGUID NetGUID;
	FUnrealObjectRef EntityObjectRef(EntityId, 0);

	// Valid if Actor is stably named. Used for stably named subobject assignment further below
	FUnrealObjectRef StablyNamedRef;

	if (Actor->IsNameStableForNetworking())
	{
		// Startup Actors have two valid UnrealObjectRefs: the entity id and the path.
		// AssignNewStablyNamedObjectNetGUID will register the path ref.
		NetGUID = AssignNewStablyNamedObjectNetGUID(Actor);

		// We register the entity id ref here.
		UnrealObjectRefToNetGUID.Emplace(EntityObjectRef, NetGUID);

		// Once we have an entity id, we should always be using it to refer to entities.
		// Since the path ref may have been registered previously, we first try to remove it
		// and then register the entity id ref.
		StablyNamedRef = NetGUIDToUnrealObjectRef[NetGUID];
		NetGUIDToUnrealObjectRef.Emplace(NetGUID, EntityObjectRef);
	}
	else
	{
		NetGUID = GetOrAssignNetGUID_SpatialGDK(Actor);
		RegisterObjectRef(NetGUID, EntityObjectRef);
	}

	UE_LOG(LogSpatialPackageMap, Verbose, TEXT("Registered new object ref for actor: %s. NetGUID: %s, entity ID: %lld"),
		*Actor->GetName(), *NetGUID.ToString(), EntityId);

	const FClassInfo& Info = SpatialNetDriver->ClassInfoManager->GetOrCreateClassInfoByClass(Actor->GetClass());
	const SubobjectToOffsetMap& SubobjectToOffset = SpatialGDK::CreateOffsetMapFromActor(Actor, Info);

	for (auto& Pair : SubobjectToOffset)
	{
		UObject* Subobject = Pair.Key;
		uint32 Offset = Pair.Value;

		// AssignNewStablyNamedObjectNetGUID is not used due to using the wrong ObjectRef as the outer of the subobject.
		// So it is ok to use RegisterObjectRef in both cases since no prior bookkeeping was done (unlike Actors)
		FNetworkGUID SubobjectNetGUID = GetOrAssignNetGUID_SpatialGDK(Subobject);
		FUnrealObjectRef EntityIdSubobjectRef(EntityId, Offset);

		if (Subobject->IsNameStableForNetworking())
		{
			if (Subobject->GetFName().ToString().Equals(TEXT("PersistentLevel")) && !Subobject->IsA<ULevel>())
			{
				UE_LOG(LogSpatialPackageMap, Fatal, TEXT("Found object called PersistentLevel which isn't a Level! This is not allowed when using the GDK"));
			}

			// Using StablyNamedRef for the outer since referencing ObjectRef in the map
			// will have the EntityId
			FUnrealObjectRef StablyNamedSubobjectRef(0, 0, Subobject->GetFName().ToString(), StablyNamedRef);

			// This is the only extra object ref that has to be registered for the subobject.
			UnrealObjectRefToNetGUID.Emplace(StablyNamedSubobjectRef, SubobjectNetGUID);

			// As the subobject may have be referred to previously in replication flow, it would
			// have it's stable name registered as it's UnrealObjectRef inside NetGUIDToUnrealObjectRef.
			// Update the map to point to the entity id version.
			NetGUIDToUnrealObjectRef.Emplace(SubobjectNetGUID, EntityIdSubobjectRef);
		}

		RegisterObjectRef(SubobjectNetGUID, EntityIdSubobjectRef);

		UE_LOG(LogSpatialPackageMap, Verbose, TEXT("Registered new object ref for subobject %s inside actor %s. NetGUID: %s, object ref: %s"),
			*Subobject->GetName(), *Actor->GetName(), *SubobjectNetGUID.ToString(), *EntityIdSubobjectRef.ToString());
	}

	return NetGUID;
}

void FSpatialNetGUIDCache::AssignNewSubobjectNetGUID(UObject* Subobject, const FUnrealObjectRef& SubobjectRef)
{
	FNetworkGUID SubobjectNetGUID = GetOrAssignNetGUID_SpatialGDK(Subobject);
	RegisterObjectRef(SubobjectNetGUID, SubobjectRef);
}

// Recursively assign netguids to the outer chain of a UObject. Then associate them with their Spatial representation (FUnrealObjectRef)
// This is required in order to be able to refer to a non-replicated stably named UObject.
// Dynamically spawned actors and references to their subobjects do not go through this codepath.
FNetworkGUID FSpatialNetGUIDCache::AssignNewStablyNamedObjectNetGUID(UObject* Object)
{
	FNetworkGUID NetGUID = GetOrAssignNetGUID_SpatialGDK(Object);
	FUnrealObjectRef ExistingObjRef = GetUnrealObjectRefFromNetGUID(NetGUID);
	if (ExistingObjRef != FUnrealObjectRef::UNRESOLVED_OBJECT_REF)
	{
		return NetGUID;
	}

	FNetworkGUID OuterGUID;
	UObject* OuterObject = Object->GetOuter();

	if (OuterObject)
	{
		OuterGUID = AssignNewStablyNamedObjectNetGUID(OuterObject);
	}


	if (Object->GetFName().ToString().Equals(TEXT("PersistentLevel")) && !Object->IsA<ULevel>())
	{
		UE_LOG(LogSpatialPackageMap, Fatal, TEXT("Found object called PersistentLevel which isn't a Level! This is not allowed when using the GDK"));
	}

	bool bNoLoadOnClient = false;
	if (IsNetGUIDAuthority())
	{
		// If the server is replicating references to things inside levels, it needs to indicate
		// that the client should not load these. Once the level is streamed in, the client will
		// resolve the references.
		bNoLoadOnClient = !CanClientLoadObject(Object, NetGUID);
	}
	FUnrealObjectRef StablyNamedObjRef(0, 0, Object->GetFName().ToString(), (OuterGUID.IsValid() && !OuterGUID.IsDefault()) ? GetUnrealObjectRefFromNetGUID(OuterGUID) : FUnrealObjectRef(), bNoLoadOnClient);
	RegisterObjectRef(NetGUID, StablyNamedObjRef);

	return NetGUID;
}

void FSpatialNetGUIDCache::RemoveEntityNetGUID(Worker_EntityId EntityId)
{
	// Remove actor subobjects.
	USpatialNetDriver* SpatialNetDriver = Cast<USpatialNetDriver>(Driver);

	SpatialGDK::UnrealMetadata* UnrealMetadata = SpatialNetDriver->StaticComponentView->GetComponentData<SpatialGDK::UnrealMetadata>(EntityId);

	// If UnrealMetadata is nullptr (can happen if the editor is closing down) just return.
	if (UnrealMetadata == nullptr)
	{
		return;
	}

	// Due to UnrealMetadata::GetNativeEntityClass using LoadObject, if we are shutting down and garbage collecting,
	// calling LoadObject will crash the editor. In this case, just return since everything will be cleaned up anyways.
	if (IsInGameThread() && IsGarbageCollecting())
	{
		return;
	}

	SpatialGDK::TSchemaOption<FUnrealObjectRef>& StablyNamedRefOption = UnrealMetadata->StablyNamedRef;

	if (UnrealMetadata->NativeClass.IsStale())
	{
		UE_LOG(LogSpatialPackageMap, Warning, TEXT("Attempting to remove stale object from package map - %s"), *UnrealMetadata->ClassPath);
	}
	else
	{
		const FClassInfo& Info = SpatialNetDriver->ClassInfoManager->GetOrCreateClassInfoByClass(UnrealMetadata->GetNativeEntityClass());

		for (auto& SubobjectInfoPair : Info.SubobjectInfo)
		{
			FUnrealObjectRef SubobjectRef(EntityId, SubobjectInfoPair.Key);
			if (FNetworkGUID* SubobjectNetGUID = UnrealObjectRefToNetGUID.Find(SubobjectRef))
			{
				NetGUIDToUnrealObjectRef.Remove(*SubobjectNetGUID);
				UnrealObjectRefToNetGUID.Remove(SubobjectRef);

				if (StablyNamedRefOption.IsSet())
				{
					UnrealObjectRefToNetGUID.Remove(FUnrealObjectRef(0, 0, SubobjectInfoPair.Value->SubobjectName.ToString(), StablyNamedRefOption.GetValue()));
				}
			}
		}
	}

	// Remove dynamically attached subobjects
	if (USpatialActorChannel* Channel = SpatialNetDriver->GetActorChannelByEntityId(EntityId))
	{
		for (UObject* DynamicSubobject : Channel->CreateSubObjects)
		{
			if (FNetworkGUID* SubobjectNetGUID = NetGUIDLookup.Find(DynamicSubobject))
			{
				if (FUnrealObjectRef* SubobjectRef = NetGUIDToUnrealObjectRef.Find(*SubobjectNetGUID))
				{
					UnrealObjectRefToNetGUID.Remove(*SubobjectRef);
					NetGUIDToUnrealObjectRef.Remove(*SubobjectNetGUID);
				}
			}
		}
	}

	// Remove actor.
	FNetworkGUID EntityNetGUID = GetNetGUIDFromEntityId(EntityId);
	// TODO: Figure out why NetGUIDToUnrealObjectRef might not have this GUID. UNR-989
	if (FUnrealObjectRef* ActorRef = NetGUIDToUnrealObjectRef.Find(EntityNetGUID))
	{
		UnrealObjectRefToNetGUID.Remove(*ActorRef);
	}
	NetGUIDToUnrealObjectRef.Remove(EntityNetGUID);
	if (StablyNamedRefOption.IsSet())
	{
		UnrealObjectRefToNetGUID.Remove(StablyNamedRefOption.GetValue());
	}
}

void FSpatialNetGUIDCache::RemoveSubobjectNetGUID(const FUnrealObjectRef& SubobjectRef)
{
	if (!UnrealObjectRefToNetGUID.Contains(SubobjectRef))
	{
		return;
	}

	USpatialNetDriver* SpatialNetDriver = Cast<USpatialNetDriver>(Driver);
	SpatialGDK::UnrealMetadata* UnrealMetadata = SpatialNetDriver->StaticComponentView->GetComponentData<SpatialGDK::UnrealMetadata>(SubobjectRef.Entity);

	// If UnrealMetadata is nullptr (can happen if the editor is closing down) just return.
	if (UnrealMetadata == nullptr)
	{
		return;
	}

	// Due to UnrealMetadata::GetNativeEntityClass using LoadObject, if we are shutting down and garbage collecting,
	// calling LoadObject will crash the editor. In this case, just return since everything will be cleaned up anyways.
	if (IsInGameThread() && IsGarbageCollecting())
	{
		return;
	}

	if (UnrealMetadata->NativeClass.IsStale())
	{
		UE_LOG(LogSpatialPackageMap, Warning, TEXT("Attempting to remove stale subobject from package map - %s"), *UnrealMetadata->ClassPath);
	}
	else
	{
		const FClassInfo& Info = SpatialNetDriver->ClassInfoManager->GetOrCreateClassInfoByClass(UnrealMetadata->GetNativeEntityClass());

		// Part of the CDO
		if (const TSharedRef<const FClassInfo>* SubobjectInfoPtr = Info.SubobjectInfo.Find(SubobjectRef.Offset))
		{
			SpatialGDK::TSchemaOption<FUnrealObjectRef>& StablyNamedRefOption = UnrealMetadata->StablyNamedRef;

			if (StablyNamedRefOption.IsSet())
			{
				UnrealObjectRefToNetGUID.Remove(FUnrealObjectRef(0, 0, SubobjectInfoPtr->Get().SubobjectName.ToString(), StablyNamedRefOption.GetValue()));
			}
		}
	}
	FNetworkGUID SubobjectNetGUID = UnrealObjectRefToNetGUID[SubobjectRef];
	NetGUIDToUnrealObjectRef.Remove(SubobjectNetGUID);
	UnrealObjectRefToNetGUID.Remove(SubobjectRef);
}

FNetworkGUID FSpatialNetGUIDCache::GetNetGUIDFromUnrealObjectRef(const FUnrealObjectRef& ObjectRef)
{
	return GetNetGUIDFromUnrealObjectRefInternal(ObjectRef);
}

FNetworkGUID FSpatialNetGUIDCache::GetNetGUIDFromUnrealObjectRefInternal(const FUnrealObjectRef& ObjectRef)
{
	FNetworkGUID* CachedGUID = UnrealObjectRefToNetGUID.Find(ObjectRef);
	FNetworkGUID NetGUID = CachedGUID ? *CachedGUID : FNetworkGUID{};
	if (!NetGUID.IsValid() && ObjectRef.Path.IsSet())
	{
		FNetworkGUID OuterGUID;

		// Recursively resolve the outers for this object in order to ensure that the package can be loaded
		if (ObjectRef.Outer.IsSet())
		{
			OuterGUID = GetNetGUIDFromUnrealObjectRef(ObjectRef.Outer.GetValue());
		}

		// Once all outer packages have been resolved, assign a new NetGUID for this object
		NetGUID = RegisterNetGUIDFromPathForStaticObject(ObjectRef.Path.GetValue(), OuterGUID, ObjectRef.bNoLoadOnClient);
		RegisterObjectRef(NetGUID, ObjectRef);
	}
	return NetGUID;
}

void FSpatialNetGUIDCache::NetworkRemapObjectRefPaths(FUnrealObjectRef& ObjectRef, bool bReading) const
{
	// If we have paths, network-sanitize all of them (e.g. removing PIE prefix).
	if (!ObjectRef.Path.IsSet())
	{
		return;
	}

	FUnrealObjectRef* Iterator = &ObjectRef;
	while (true)
	{
		if (Iterator->Path.IsSet())
		{
			FString TempPath(*Iterator->Path);
			GEngine->NetworkRemapPath(Driver, TempPath, bReading);
			Iterator->Path = TempPath;
		}
		if (!Iterator->Outer.IsSet())
		{
			break;
		}
		Iterator = &Iterator->Outer.GetValue();
	}
}

void FSpatialNetGUIDCache::UnregisterActorObjectRefOnly(const FUnrealObjectRef& ObjectRef)
{
	FNetworkGUID& NetGUID = UnrealObjectRefToNetGUID.FindChecked(ObjectRef);
	// Remove ObjectRef first so the reference above isn't destroyed
	NetGUIDToUnrealObjectRef.Remove(NetGUID);
	UnrealObjectRefToNetGUID.Remove(ObjectRef);
}

FUnrealObjectRef FSpatialNetGUIDCache::GetUnrealObjectRefFromNetGUID(const FNetworkGUID& NetGUID) const
{
	const FUnrealObjectRef* ObjRef = NetGUIDToUnrealObjectRef.Find(NetGUID);
	return ObjRef ? (FUnrealObjectRef)*ObjRef : FUnrealObjectRef::UNRESOLVED_OBJECT_REF;
}

FNetworkGUID FSpatialNetGUIDCache::GetNetGUIDFromEntityId(Worker_EntityId EntityId) const
{
	FUnrealObjectRef ObjRef(EntityId, 0);
	const FNetworkGUID* NetGUID = UnrealObjectRefToNetGUID.Find(ObjRef);
	return (NetGUID == nullptr) ? FNetworkGUID(0) : *NetGUID;
}

FNetworkGUID FSpatialNetGUIDCache::RegisterNetGUIDFromPathForStaticObject(const FString& PathName, const FNetworkGUID& OuterGUID, bool bNoLoadOnClient)
{
	// Put the PIE prefix back (if applicable) so that the correct object can be found.
	FString TempPath = PathName;
	GEngine->NetworkRemapPath(Driver, TempPath, true);

	// This function should only be called for stably named object references, not dynamic ones.
	FNetGuidCacheObject CacheObject;
	CacheObject.PathName = FName(*TempPath);
	CacheObject.OuterGUID = OuterGUID;
	CacheObject.bNoLoad = bNoLoadOnClient;		// server decides whether the client should load objects (e.g. don't load levels)
	CacheObject.bIgnoreWhenMissing = bNoLoadOnClient;
	FNetworkGUID NetGUID = GenerateNewNetGUID(1);
	RegisterNetGUID_Internal(NetGUID, CacheObject);
	return NetGUID;
}

FNetworkGUID FSpatialNetGUIDCache::GenerateNewNetGUID(const int32 IsStatic)
{
	// Here we have to borrow from FNetGuidCache::AssignNewNetGUID_Server to avoid a source change.
#define COMPOSE_NET_GUID(Index, IsStatic)	(((Index) << 1) | (IsStatic) )
#define ALLOC_NEW_NET_GUID(IsStatic)		(COMPOSE_NET_GUID(++UniqueNetIDs[IsStatic], IsStatic))

	// Generate new NetGUID and assign it
	FNetworkGUID NetGUID = FNetworkGUID(ALLOC_NEW_NET_GUID(IsStatic));
	return NetGUID;
}

FNetworkGUID FSpatialNetGUIDCache::GetOrAssignNetGUID_SpatialGDK(UObject* Object)
{
	FNetworkGUID NetGUID = GetOrAssignNetGUID(Object);

	// One major difference between how Unreal does NetGUIDs vs us is, we don't attempt to make them consistent across workers and client.
	// The function above might have returned without assigning new GUID, because we are the client.
	// Let's directly call the client function in that case.
	if (Object != nullptr && NetGUID == FNetworkGUID::GetDefault() && !IsNetGUIDAuthority())
	{
		NetGUID = GenerateNewNetGUID(IsDynamicObject(Object) ? 0 : 1);

		FNetGuidCacheObject CacheObject;
		CacheObject.Object = MakeWeakObjectPtr(const_cast<UObject*>(Object));
		CacheObject.PathName = Object->GetFName();
		CacheObject.OuterGUID = GetOrAssignNetGUID_SpatialGDK(Object->GetOuter());
		CacheObject.bIgnoreWhenMissing = true;
		RegisterNetGUID_Internal(NetGUID, CacheObject);

		UE_LOG(LogSpatialPackageMap, Verbose, TEXT("%s: NetGUID for object %s was not found in the cache. Generated new NetGUID %s."),
			*Cast<USpatialNetDriver>(Driver)->Connection->GetWorkerId(),
			*Object->GetPathName(),
			*NetGUID.ToString());
	}

	check((NetGUID.IsValid() && !NetGUID.IsDefault()) || Object == nullptr);
	return NetGUID;
}

void FSpatialNetGUIDCache::RegisterObjectRef(FNetworkGUID NetGUID, const FUnrealObjectRef& ObjectRef)
{
	// Registered ObjectRefs should never have PIE.
	FUnrealObjectRef RemappedObjectRef = ObjectRef;
	NetworkRemapObjectRefPaths(RemappedObjectRef, false /*bIsReading*/);

	checkfSlow(!NetGUIDToUnrealObjectRef.Contains(NetGUID) || (NetGUIDToUnrealObjectRef.Contains(NetGUID) && NetGUIDToUnrealObjectRef.FindChecked(NetGUID) == RemappedObjectRef),
		TEXT("NetGUID to UnrealObjectRef mismatch - NetGUID: %s ObjRef in map: %s ObjRef expected: %s"), *NetGUID.ToString(),
		*NetGUIDToUnrealObjectRef.FindChecked(NetGUID).ToString(), *RemappedObjectRef.ToString());
	checkfSlow(!UnrealObjectRefToNetGUID.Contains(RemappedObjectRef) || (UnrealObjectRefToNetGUID.Contains(RemappedObjectRef) && UnrealObjectRefToNetGUID.FindChecked(RemappedObjectRef) == NetGUID),
		TEXT("UnrealObjectRef to NetGUID mismatch - UnrealObjectRef: %s NetGUID in map: %s NetGUID expected: %s"), *NetGUID.ToString(),
		*UnrealObjectRefToNetGUID.FindChecked(RemappedObjectRef).ToString(), *RemappedObjectRef.ToString());
	NetGUIDToUnrealObjectRef.Emplace(NetGUID, RemappedObjectRef);
	UnrealObjectRefToNetGUID.Emplace(RemappedObjectRef, NetGUID);
}
