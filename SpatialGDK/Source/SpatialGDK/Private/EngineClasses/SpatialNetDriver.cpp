// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "EngineClasses/SpatialNetDriver.h"

#include "Engine/ActorChannel.h"
#include "Engine/ChildConnection.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/NetworkObjectList.h"
#include "EngineGlobals.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameNetworkManager.h"
#include "Misc/MessageDialog.h"
#include "Net/DataReplication.h"
#include "Net/RepLayout.h"
#include "SocketSubsystem.h"
#include "UObject/UObjectIterator.h"

#include "EngineClasses/SpatialActorChannel.h"
#include "EngineClasses/SpatialGameInstance.h"
#include "EngineClasses/SpatialNetConnection.h"
#include "EngineClasses/SpatialPackageMapClient.h"
#include "EngineClasses/SpatialPendingNetGame.h"
#include "EngineClasses/SpatialWorldSettings.h"
#include "Interop/Connection/SpatialConnectionManager.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Interop/GlobalStateManager.h"
#include "Interop/SpatialClassInfoManager.h"
#include "Interop/SpatialPlayerSpawner.h"
#include "Interop/SpatialReceiver.h"
#include "Interop/SpatialSender.h"
#include "Interop/SpatialWorkerFlags.h"
#include "LoadBalancing/AbstractLBStrategy.h"
#include "LoadBalancing/GridBasedLBStrategy.h"
#include "LoadBalancing/OwnershipLockingPolicy.h"
#include "SpatialConstants.h"
#include "SpatialGDKSettings.h"
#include "Schema/Singleton.h"
#include "Utils/ComponentFactory.h"
#include "Utils/EntityPool.h"
#include "Utils/ErrorCodeRemapping.h"
#include "Utils/InterestFactory.h"
#include "Utils/OpUtils.h"
#include "Utils/SpatialActorGroupManager.h"
#include "Utils/SpatialDebugger.h"
#include "Utils/SpatialMetrics.h"
#include "Utils/SpatialMetricsDisplay.h"
#include "Utils/SpatialStatics.h"

#if WITH_EDITOR
#include "Settings/LevelEditorPlaySettings.h"
#include "SpatialGDKServicesModule.h"
#endif

using SpatialGDK::ComponentFactory;
using SpatialGDK::FindFirstOpOfType;
using SpatialGDK::FindFirstOpOfTypeForComponent;
using SpatialGDK::InterestFactory;
using SpatialGDK::RPCPayload;

DEFINE_LOG_CATEGORY(LogSpatialOSNetDriver);

DECLARE_CYCLE_STAT(TEXT("ServerReplicateActors"), STAT_SpatialServerReplicateActors, STATGROUP_SpatialNet);
DECLARE_CYCLE_STAT(TEXT("ProcessPrioritizedActors"), STAT_SpatialProcessPrioritizedActors, STATGROUP_SpatialNet);
DECLARE_CYCLE_STAT(TEXT("PrioritizeActors"), STAT_SpatialPrioritizeActors, STATGROUP_SpatialNet);
DECLARE_CYCLE_STAT(TEXT("ProcessOps"), STAT_SpatialProcessOps, STATGROUP_SpatialNet);
DECLARE_CYCLE_STAT(TEXT("UpdateAuthority"), STAT_SpatialUpdateAuthority, STATGROUP_SpatialNet);
DEFINE_STAT(STAT_SpatialConsiderList);
DEFINE_STAT(STAT_SpatialActorsRelevant);
DEFINE_STAT(STAT_SpatialActorsChanged);

USpatialNetDriver::USpatialNetDriver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LoadBalanceStrategy(nullptr)
	, LoadBalanceEnforcer(nullptr)
	, bAuthoritativeDestruction(true)
	, bConnectAsClient(false)
	, bPersistSpatialConnection(true)
	, bWaitingToSpawn(false)
	, bIsReadyToStart(false)
	, bMapLoaded(false)
	, SessionId(0)
	, NextRPCIndex(0)
	, TimeWhenPositionLastUpdated(0.f)
{
#if ENGINE_MINOR_VERSION >= 23
	// Due to changes in 4.23, we now use an outdated flow in ComponentReader::ApplySchemaObject
	// Native Unreal now iterates over all commands on clients, and no longer has access to a BaseHandleToCmdIndex
	// in the RepLayout, the below change forces its creation on clients, but this is a workaround
	// TODO: UNR-2375
	bMaySendProperties = true;
#endif
}

bool USpatialNetDriver::InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error)
{
	if (!Super::InitBase(bInitAsClient, InNotify, URL, bReuseAddressAndPort, Error))
	{
		return false;
	}

	bConnectAsClient = bInitAsClient;

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USpatialNetDriver::OnMapLoaded);

	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &USpatialNetDriver::OnLevelAddedToWorld);

	if (GetWorld() != nullptr)
	{
		GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &USpatialNetDriver::OnActorSpawned));
	}

	// Make absolutely sure that the actor channel that we are using is our Spatial actor channel
	// Copied from what the Engine does with UActorChannel
	FChannelDefinition SpatialChannelDefinition{};
	SpatialChannelDefinition.ChannelName = NAME_Actor;
	SpatialChannelDefinition.ClassName = FName(*USpatialActorChannel::StaticClass()->GetPathName());
	SpatialChannelDefinition.ChannelClass = USpatialActorChannel::StaticClass();
	SpatialChannelDefinition.bServerOpen = true;

	ChannelDefinitions[CHTYPE_Actor] = SpatialChannelDefinition;
	ChannelDefinitionMap[NAME_Actor] = SpatialChannelDefinition;

	// If no sessionId exists in the URL options, SessionId member will be set to 0.
	SessionId = FCString::Atoi(URL.GetOption(*SpatialConstants::SpatialSessionIdURLOption, TEXT("0")));

	// We do this here straight away to trigger LoadMap.
	if (bInitAsClient)
	{
		// If the URL has not specified to keep the clients connection then we should create a new one.
		bPersistSpatialConnection = URL.HasOption(*SpatialConstants::ClientsStayConnectedURLOption);
	}
	else
	{
		// Servers should never disconnect from a deployment.
		bPersistSpatialConnection = true;
	}

	ActorGroupManager = GetGameInstance()->ActorGroupManager.Get();

	// Initialize ClassInfoManager here because it needs to load SchemaDatabase.
	// We shouldn't do that in CreateAndInitializeCoreClasses because it is called
	// from OnConnectionToSpatialOSSucceeded callback which could be executed with the async
	// loading thread suspended (e.g. when resuming rendering thread), in which
	// case we'll crash upon trying to load SchemaDatabase.
	ClassInfoManager = NewObject<USpatialClassInfoManager>();

	// If it fails to load, don't attempt to connect to spatial.
	if (!ClassInfoManager->TryInit(this, ActorGroupManager))
	{
		Error = TEXT("Failed to load Spatial SchemaDatabase! Make sure that schema has been generated for your project");
		return false;
	}

#if WITH_EDITOR
	PlayInEditorID = GPlayInEditorID;

	// If we're launching in PIE then ensure there is a deployment running before connecting.
	if (FSpatialGDKServicesModule* GDKServices = FModuleManager::GetModulePtr<FSpatialGDKServicesModule>("SpatialGDKServices"))
	{
		FLocalDeploymentManager* LocalDeploymentManager = GDKServices->GetLocalDeploymentManager();

		// Wait for a running local deployment before connecting. If the deployment has already started then just connect.
		if (LocalDeploymentManager->ShouldWaitForDeployment())
		{
			UE_LOG(LogSpatialOSNetDriver, Display, TEXT("Waiting for local SpatialOS deployment to start before connecting..."));
			SpatialDeploymentStartHandle = LocalDeploymentManager->OnDeploymentStart.AddLambda([WeakThis = TWeakObjectPtr<USpatialNetDriver>(this), URL]
				{
					if (!WeakThis.IsValid())
					{
						return;
					}
					UE_LOG(LogSpatialOSNetDriver, Display, TEXT("Local deployment started, connecting with URL: %s"), *URL.ToString());

					WeakThis.Get()->InitiateConnectionToSpatialOS(URL);
					if (FSpatialGDKServicesModule* GDKServices = FModuleManager::GetModulePtr<FSpatialGDKServicesModule>("SpatialGDKServices"))
					{
						GDKServices->GetLocalDeploymentManager()->OnDeploymentStart.Remove(WeakThis.Get()->SpatialDeploymentStartHandle);
					}
				});

			return true;
		}
	}

	TombstonedEntities.Reserve(EDITOR_TOMBSTONED_ENTITY_TRACKING_RESERVATION_COUNT);
#endif

	InitiateConnectionToSpatialOS(URL);

	return true;
}

USpatialGameInstance* USpatialNetDriver::GetGameInstance() const
{
	USpatialGameInstance* GameInstance = nullptr;

	// A client does not have a world at this point, so we use the WorldContext
	// to get a reference to the GameInstance
	if (bConnectAsClient)
	{
		const FWorldContext& WorldContext = GEngine->GetWorldContextFromPendingNetGameNetDriverChecked(this);
		GameInstance = Cast<USpatialGameInstance>(WorldContext.OwningGameInstance);
	}
	else
	{
		GameInstance = Cast<USpatialGameInstance>(GetWorld()->GetGameInstance());
	}

	return GameInstance;
}

void USpatialNetDriver::InitiateConnectionToSpatialOS(const FURL& URL)
{
	USpatialGameInstance* GameInstance = GetGameInstance();

	if (GameInstance == nullptr)
	{
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("A SpatialGameInstance is required. Make sure your game's GameInstance inherits from SpatialGameInstance"));
		return;
	}

	if (bConnectAsClient)
	{
		bPersistSpatialConnection = URL.HasOption(*SpatialConstants::ClientsStayConnectedURLOption);
	}

	if (!bPersistSpatialConnection)
	{
		GameInstance->DestroySpatialConnectionManager();
		GameInstance->CreateNewSpatialConnectionManager();
	}
	else
	{
		UE_LOG(LogSpatialOSNetDriver, Log, TEXT("Getting existing connection, not creating a new one"));
	}

	ConnectionManager = GameInstance->GetSpatialConnectionManager();
	ConnectionManager->OnConnectedCallback.BindUObject(this, &USpatialNetDriver::OnConnectionToSpatialOSSucceeded);
	ConnectionManager->OnFailedToConnectCallback.BindUObject(this, &USpatialNetDriver::OnConnectionToSpatialOSFailed);

	// If this is the first connection try using the command line arguments to setup the config objects.
	// If arguments can not be found we will use the regular flow of loading from the input URL.

	FString SpatialWorkerType = GetGameInstance()->GetSpatialWorkerType().ToString();

	if (!GameInstance->GetFirstConnectionToSpatialOSAttempted())
	{
		GameInstance->SetFirstConnectionToSpatialOSAttempted();
		if (GetDefault<USpatialGDKSettings>()->GetPreventClientCloudDeploymentAutoConnect(bConnectAsClient))
		{
			// If first time connecting but the bGetPreventClientCloudDeploymentAutoConnect flag is set then use input URL to setup connection config.
			ConnectionManager->SetupConnectionConfigFromURL(URL, SpatialWorkerType);
		}
		// Otherwise, try using command line arguments to setup connection config.
		else if (!ConnectionManager->TrySetupConnectionConfigFromCommandLine(SpatialWorkerType))
		{
			// If the command line arguments can not be used, use the input URL to setup connection config.
			ConnectionManager->SetupConnectionConfigFromURL(URL, SpatialWorkerType);
		}
	}
	else if (URL.Host == SpatialConstants::RECONNECT_USING_COMMANDLINE_ARGUMENTS)
	{
		if (!ConnectionManager->TrySetupConnectionConfigFromCommandLine(SpatialWorkerType))
		{
			ConnectionManager->SetConnectionType(ESpatialConnectionType::Receptionist);
			ConnectionManager->ReceptionistConfig.LoadDefaults();
			ConnectionManager->ReceptionistConfig.WorkerType = SpatialWorkerType;
		}
	}
	else
	{
		ConnectionManager->SetupConnectionConfigFromURL(URL, SpatialWorkerType);
	}

#if WITH_EDITOR
	ConnectionManager->Connect(bConnectAsClient, PlayInEditorID);
#else
	ConnectionManager->Connect(bConnectAsClient, 0);
#endif
}

void USpatialNetDriver::OnConnectionToSpatialOSSucceeded()
{
	Connection = ConnectionManager->GetWorkerConnection();
	check(Connection);

	// If we're the server, we will spawn the special Spatial connection that will route all updates to SpatialOS.
	// There may be more than one of these connections in the future for different replication conditions.
	if (!bConnectAsClient)
	{
		CreateServerSpatialOSNetConnection();
	}

	CreateAndInitializeCoreClasses();

	// Query the GSM to figure out what map to load
	if (bConnectAsClient)
	{
		QueryGSMToLoadMap();
	}
	else
	{
		Sender->CreateServerWorkerEntity();
	}

	USpatialGameInstance* GameInstance = GetGameInstance();
	check(GameInstance != nullptr);
	GameInstance->HandleOnConnected();
}

void USpatialNetDriver::OnConnectionToSpatialOSFailed(uint8_t ConnectionStatusCode, const FString& ErrorMessage)
{
	if (USpatialGameInstance* GameInstance = GetGameInstance())
	{
		if (GEngine != nullptr && GameInstance->GetWorld() != nullptr)
		{
			GEngine->BroadcastNetworkFailure(GameInstance->GetWorld(), this, ENetworkFailure::FromDisconnectOpStatusCode(ConnectionStatusCode), *ErrorMessage);
		}

		GameInstance->HandleOnConnectionFailed(ErrorMessage);
	}
}

void USpatialNetDriver::InitializeSpatialOutputDevice()
{
	int32 PIEIndex = -1; // -1 is Unreal's default index when not using PIE
#if WITH_EDITOR
	if (!bConnectAsClient)
	{
		PIEIndex = GEngine->GetWorldContextFromWorldChecked(GetWorld()).PIEInstance;
	}
	else
	{
		PIEIndex = GEngine->GetWorldContextFromPendingNetGameNetDriverChecked(this).PIEInstance;
	}
#endif //WITH_EDITOR

	FName LoggerName = FName(TEXT("Unreal"));

	if (const USpatialGameInstance * GameInstance = GetGameInstance())
	{
		LoggerName = GameInstance->GetSpatialWorkerType();
	}

	SpatialOutputDevice = MakeUnique<FSpatialOutputDevice>(Connection, LoggerName, PIEIndex);
}

void USpatialNetDriver::CreateAndInitializeCoreClasses()
{
	InitializeSpatialOutputDevice();

	Dispatcher = MakeUnique<SpatialDispatcher>();
	Sender = NewObject<USpatialSender>();
	Receiver = NewObject<USpatialReceiver>();

	// TODO: UNR-2452
	// Ideally the GlobalStateManager and StaticComponentView would be created as part of USpatialWorkerConnection::Init
	// however, this causes a crash upon the second instance of running PIE due to a destroyed USpatialNetDriver still being reference.
	// Why the destroyed USpatialNetDriver is referenced is unknown.
	USpatialGameInstance* GameInstance = GetGameInstance();
	check(GameInstance != nullptr);

	GlobalStateManager = GameInstance->GetGlobalStateManager();
	check(GlobalStateManager != nullptr);

	StaticComponentView = GameInstance->GetStaticComponentView();
	check(StaticComponentView != nullptr);

	PlayerSpawner = NewObject<USpatialPlayerSpawner>();
	SnapshotManager = MakeUnique<SpatialSnapshotManager>();
	SpatialMetrics = NewObject<USpatialMetrics>();
	SpatialWorkerFlags = NewObject<USpatialWorkerFlags>();

	const USpatialGDKSettings* SpatialSettings = GetDefault<USpatialGDKSettings>();
#if !UE_BUILD_SHIPPING
	// If metrics display is enabled, spawn a singleton Actor to replicate the information to each client.
	if (IsServer())
	{
		if (SpatialSettings->bEnableMetricsDisplay)
		{
			SpatialMetricsDisplay = GetWorld()->SpawnActor<ASpatialMetricsDisplay>();
		}

		if (SpatialSettings->SpatialDebugger != nullptr)
		{
			SpatialDebugger = GetWorld()->SpawnActor<ASpatialDebugger>(SpatialSettings->SpatialDebugger);
		}
	}
#endif

	if (SpatialSettings->bEnableUnrealLoadBalancer)
	{
		CreateAndInitializeLoadBalancingClasses();
	}

	if (SpatialSettings->UseRPCRingBuffer())
	{
		RPCService = MakeUnique<SpatialGDK::SpatialRPCService>(ExtractRPCDelegate::CreateUObject(Receiver, &USpatialReceiver::OnExtractIncomingRPC), StaticComponentView);
	}

	Dispatcher->Init(Receiver, StaticComponentView, SpatialMetrics, SpatialWorkerFlags);
	Sender->Init(this, &TimerManager, RPCService.Get());
	Receiver->Init(this, &TimerManager, RPCService.Get());
	GlobalStateManager->Init(this);
	SnapshotManager->Init(Connection, GlobalStateManager, Receiver);
	PlayerSpawner->Init(this, &TimerManager);
	PlayerSpawner->OnPlayerSpawnFailed.BindUObject(GameInstance, &USpatialGameInstance::HandleOnPlayerSpawnFailed);
	SpatialMetrics->Init(Connection, NetServerMaxTickRate, IsServer());
	SpatialMetrics->ControllerRefProvider.BindUObject(this, &USpatialNetDriver::GetCurrentPlayerControllerRef);

	// PackageMap value has been set earlier in USpatialNetConnection::InitBase
	// Making sure the value is the same
	USpatialPackageMapClient* NewPackageMap = Cast<USpatialPackageMapClient>(GetSpatialOSNetConnection()->PackageMap);
	check(NewPackageMap == PackageMap);

	PackageMap->Init(this, &TimerManager);

	// The interest factory depends on the package map, so is created last.
	InterestFactory = MakeUnique<SpatialGDK::InterestFactory>(ClassInfoManager, PackageMap);
}

void USpatialNetDriver::CreateAndInitializeLoadBalancingClasses()
{
	const ASpatialWorldSettings* WorldSettings = GetWorld() ? Cast<ASpatialWorldSettings>(GetWorld()->GetWorldSettings()) : nullptr;
	if (IsServer())
	{
		if (WorldSettings == nullptr || WorldSettings->LoadBalanceStrategy == nullptr)
		{
			if (WorldSettings == nullptr)
			{
				UE_LOG(LogSpatialOSNetDriver, Error, TEXT("If EnableUnrealLoadBalancer is set, WorldSettings should inherit from SpatialWorldSettings to get the load balancing strategy. Using a 1x1 grid."));
			}
			else
			{
				UE_LOG(LogSpatialOSNetDriver, Error, TEXT("If EnableUnrealLoadBalancer is set, there must be a LoadBalancing strategy set. Using a 1x1 grid."));
			}
			LoadBalanceStrategy = NewObject<UGridBasedLBStrategy>(this);
		}
		else
		{
			LoadBalanceStrategy = NewObject<UAbstractLBStrategy>(this, WorldSettings->LoadBalanceStrategy);
		}
		LoadBalanceStrategy->Init();
	}

	VirtualWorkerTranslator = MakeUnique<SpatialVirtualWorkerTranslator>(LoadBalanceStrategy, Connection->GetWorkerId());

	if (IsServer())
	{
		LoadBalanceEnforcer = MakeUnique<SpatialLoadBalanceEnforcer>(Connection->GetWorkerId(), StaticComponentView, VirtualWorkerTranslator.Get());

		if (WorldSettings == nullptr || WorldSettings->LockingPolicy == nullptr)
		{
			UE_LOG(LogSpatialOSNetDriver, Error, TEXT("If EnableUnrealLoadBalancer is set, there must be a Locking Policy set. Using default policy."));
			LockingPolicy = NewObject<UOwnershipLockingPolicy>(this);
		}
		else
		{
			LockingPolicy = NewObject<UAbstractLockingPolicy>(this, WorldSettings->LockingPolicy);
		}
		LockingPolicy->Init(AcquireLockDelegate, ReleaseLockDelegate);
	}
}


void USpatialNetDriver::CreateServerSpatialOSNetConnection()
{
	check(!bConnectAsClient);

	USpatialNetConnection* NetConnection = NewObject<USpatialNetConnection>(GetTransientPackage(), NetConnectionClass);
	check(NetConnection);

	ISocketSubsystem* SocketSubsystem = GetSocketSubsystem();
	// This is just a fake address so that Unreal doesn't ensure-crash on disconnecting from SpatialOS
	// See UNetDriver::RemoveClientConnection for more details, but basically there is a TMap which uses internet addresses as the key and an unitialised
	// internet address for a connection causes the TMap.Find to fail
	TSharedRef<FInternetAddr> FromAddr = SocketSubsystem->CreateInternetAddr();
	bool bIsAddressValid = false;
	FromAddr->SetIp(*SpatialConstants::LOCAL_HOST, bIsAddressValid);

	check(bIsAddressValid);

	// Each connection stores a URL with various optional settings (host, port, map, netspeed...)
	// We currently don't make use of any of these as some are meaningless in a SpatialOS world, and some are less of a priority.
	// So for now we just give the connection a dummy url, might change in the future.
	FURL DummyURL;

	NetConnection->InitRemoteConnection(this, nullptr, DummyURL, *FromAddr, USOCK_Open);
	Notify->NotifyAcceptedConnection(NetConnection);
	NetConnection->bReliableSpatialConnection = true;
	AddClientConnection(NetConnection);
	//Since this is not a "real" client connection, we immediately pretend that it is fully logged on.
	NetConnection->SetClientLoginState(EClientLoginState::Welcomed);

	// Bind the ProcessServerTravel delegate to the spatial variant. This ensures that if ServerTravel is called and Spatial networking is enabled, we can travel properly.
	GetWorld()->SpatialProcessServerTravelDelegate.BindStatic(SpatialProcessServerTravel);
}

bool USpatialNetDriver::ClientCanSendPlayerSpawnRequests()
{
	return GlobalStateManager->GetAcceptingPlayers() && SessionId == GlobalStateManager->GetSessionId();
}

void USpatialNetDriver::OnGSMQuerySuccess()
{
	// If the deployment is now accepting players and we are waiting to spawn. Spawn.
	if (bWaitingToSpawn && ClientCanSendPlayerSpawnRequests())
	{
		uint32 ServerHash = GlobalStateManager->GetSchemaHash();
		if (ClassInfoManager->SchemaDatabase->SchemaDescriptorHash != ServerHash) // Are we running with the same schema hash as the server?
		{
			UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Your client's schema does not match your deployment's schema. Client hash: '%u' Server hash: '%u'"), ClassInfoManager->SchemaDatabase->SchemaDescriptorHash, ServerHash);
		}

		UWorld* CurrentWorld = GetWorld();
		const FString& DeploymentMapURL = GlobalStateManager->GetDeploymentMapURL();
		if (CurrentWorld == nullptr || CurrentWorld->RemovePIEPrefix(DeploymentMapURL) != CurrentWorld->RemovePIEPrefix(CurrentWorld->URL.Map))
		{
			// Load the correct map based on the GSM URL
			UE_LOG(LogSpatial, Log, TEXT("Welcomed by SpatialOS (Level: %s)"), *DeploymentMapURL);

			// Extract map name and options
			FWorldContext& WorldContext = GEngine->GetWorldContextFromPendingNetGameNetDriverChecked(this);
			FURL LastURL = WorldContext.PendingNetGame->URL;

			FURL RedirectURL = FURL(&LastURL, *DeploymentMapURL, (ETravelType)WorldContext.TravelType);
			RedirectURL.Host = LastURL.Host;
			RedirectURL.Port = LastURL.Port;
			RedirectURL.Portal = LastURL.Portal;

			// Usually the LastURL options are added to the RedirectURL in the FURL constructor.
			// However this is not the case when TravelType = TRAVEL_Absolute so we must do it explicitly here.
			if (WorldContext.TravelType == ETravelType::TRAVEL_Absolute)
			{
				RedirectURL.Op.Append(LastURL.Op);
			}

			RedirectURL.AddOption(*SpatialConstants::ClientsStayConnectedURLOption);

			WorldContext.PendingNetGame->bSuccessfullyConnected = true;
			WorldContext.PendingNetGame->bSentJoinRequest = false;
			WorldContext.PendingNetGame->URL = RedirectURL;

			// Ensure the singleton map is reset as it will contain bad data from the old map
			GlobalStateManager->RemoveAllSingletons();
		}
		else
		{
			MakePlayerSpawnRequest();
		}
	}
}

void USpatialNetDriver::RetryQueryGSM()
{
	float RetryTimerDelay = SpatialConstants::ENTITY_QUERY_RETRY_WAIT_SECONDS;

	// In PIE we want to retry the entity query as soon as possible.
#if WITH_EDITOR
	RetryTimerDelay = 0.1f;
#endif

	UE_LOG(LogSpatialOSNetDriver, Log, TEXT("Retrying query for GSM in %f seconds"), RetryTimerDelay);
	FTimerHandle RetryTimer;
	TimerManager.SetTimer(RetryTimer, [WeakThis = TWeakObjectPtr<USpatialNetDriver>(this)]()
	{
		if (WeakThis.IsValid())
		{
			if (UGlobalStateManager* GSM = WeakThis.Get()->GlobalStateManager)
			{
				UGlobalStateManager::QueryDelegate QueryDelegate;
				QueryDelegate.BindUObject(WeakThis.Get(), &USpatialNetDriver::GSMQueryDelegateFunction);
				GSM->QueryGSM(QueryDelegate);
			}
		}
	}, RetryTimerDelay, false);
}

void USpatialNetDriver::GSMQueryDelegateFunction(const Worker_EntityQueryResponseOp& Op)
{
	bool bNewAcceptingPlayers = false;
	int32 QuerySessionId = 0;
	bool bQueryResponseSuccess = GlobalStateManager->GetAcceptingPlayersAndSessionIdFromQueryResponse(Op, bNewAcceptingPlayers, QuerySessionId);

	if (!bQueryResponseSuccess)
	{
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Failed to extract AcceptingPlayers and SessionId from GSM query response."));
		RetryQueryGSM();
		return;
	}
	else if (!bNewAcceptingPlayers)
	{
		UE_LOG(LogSpatialOSNetDriver, Log, TEXT("GlobalStateManager not accepting players. Usually caused by servers not registering themselves with the deployment yet. Did you launch the correct number of servers?"));
		RetryQueryGSM();
		return;
	}
	else if (QuerySessionId != SessionId)
	{
		UE_LOG(LogSpatialOSNetDriver, Log, TEXT("GlobalStateManager session id mismatch - got (%d) expected (%d)."), QuerySessionId, SessionId);
		RetryQueryGSM();
		return;
	}

	OnGSMQuerySuccess();
}

void USpatialNetDriver::QueryGSMToLoadMap()
{
	check(bConnectAsClient);

	// Register our interest in spawning.
	bWaitingToSpawn = true;

	UGlobalStateManager::QueryDelegate QueryDelegate;
	QueryDelegate.BindUObject(this, &USpatialNetDriver::GSMQueryDelegateFunction);

	// Begin querying the state of the GSM so we know the state of AcceptingPlayers and SessionId.
	GlobalStateManager->QueryGSM(QueryDelegate);
}

void USpatialNetDriver::OnActorSpawned(AActor* Actor)
{
	if (!Actor->GetIsReplicated() ||
		Actor->GetLocalRole() != ROLE_Authority ||
		Actor->GetClass()->HasAnySpatialClassFlags(SPATIALCLASS_Singleton) ||
		!Actor->GetClass()->HasAnySpatialClassFlags(SPATIALCLASS_SpatialType) ||
		USpatialStatics::IsActorGroupOwnerForActor(Actor))
	{
		// We only want to delete actors which are replicated and we somehow gain local authority over, while not the actor group owner.
		return;
	}

	const FString WorkerType = GetGameInstance()->GetSpatialWorkerType().ToString();
	UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Worker %s spawned replicated actor %s (owner: %s) but is not actor group owner for actor group %s. The actor will be destroyed in 0.01s"),
		*WorkerType, *GetNameSafe(Actor), *GetNameSafe(Actor->GetOwner()), *USpatialStatics::GetActorGroupForActor(Actor).ToString());
	// We tear off, because otherwise SetLifeSpan fails, we SetLifeSpan because we are just about to spawn the Actor and Unreal would complain if we destroyed it.
	Actor->TearOff();
	Actor->SetLifeSpan(0.01f);
}

void USpatialNetDriver::OnMapLoaded(UWorld* LoadedWorld)
{
	if (LoadedWorld == nullptr)
	{
		return;
	}

	if (LoadedWorld->GetNetDriver() != this)
	{
		// In PIE, if we have more than 2 clients, then OnMapLoaded is going to be triggered once each client loads the world.
		// As the delegate is a global variable, it triggers all 3 USpatialNetDriver::OnMapLoaded callbacks. As a result, we should
		// make sure that the net driver of this world is in fact us.
		return;
	}

	if (IsServer())
	{
		if (GlobalStateManager != nullptr &&
			!GlobalStateManager->GetCanBeginPlay() &&
			StaticComponentView->HasAuthority(GlobalStateManager->GlobalStateManagerEntityId, SpatialConstants::STARTUP_ACTOR_MANAGER_COMPONENT_ID))
		{
			// ServerTravel - Increment the session id, so users don't rejoin the old game.
			GlobalStateManager->TriggerBeginPlay();
			GlobalStateManager->SetDeploymentState();
			GlobalStateManager->SetAcceptingPlayers(true);
			GlobalStateManager->IncrementSessionID();
		}
	}
	else
	{
		if (ClientCanSendPlayerSpawnRequests())
		{
			MakePlayerSpawnRequest();
		}
		else
		{
			UE_LOG(LogSpatial, Warning, TEXT("Client map finished loading but could not send player spawn request. Will requery the GSM for the correct map to load."));
			QueryGSMToLoadMap();
		}
	}

	bMapLoaded = true;
}

void USpatialNetDriver::MakePlayerSpawnRequest()
{
	if (bWaitingToSpawn)
	{
		PlayerSpawner->SendPlayerSpawnRequest();
		bWaitingToSpawn = false;
		bPersistSpatialConnection = false;
	}
}

void USpatialNetDriver::OnLevelAddedToWorld(ULevel* LoadedLevel, UWorld* OwningWorld)
{
	UE_LOG(LogSpatialOSNetDriver, Log, TEXT("OnLevelAddedToWorld: Level (%s) OwningWorld (%s) World (%s)"),
		*GetNameSafe(LoadedLevel), *GetNameSafe(OwningWorld), *GetNameSafe(World));

	if (OwningWorld != World
		|| !IsServer()
		|| GlobalStateManager == nullptr
		|| USpatialStatics::IsSpatialOffloadingEnabled())
	{
		// If the world isn't our owning world, we are a client, or we loaded the levels
		// before connecting to Spatial, or we are running with offloading, we return early.
		return;
	}

	const bool bLoadBalancingEnabled = GetDefault<USpatialGDKSettings>()->bEnableUnrealLoadBalancer;
	const bool bHaveGSMAuthority = StaticComponentView->HasAuthority(SpatialConstants::INITIAL_GLOBAL_STATE_MANAGER_ENTITY_ID, SpatialConstants::STARTUP_ACTOR_MANAGER_COMPONENT_ID);

	if (!bLoadBalancingEnabled && !bHaveGSMAuthority)
	{
		// If load balancing is disabled and this worker is not GSM authoritative then exit early.
		return;
	}

	if (bLoadBalancingEnabled && !LoadBalanceStrategy->IsReady())
	{
		// Load balancer isn't ready, this should only occur when servers are loading composition levels on startup, before connecting to spatial.
		return;
	}

	for (auto Actor : LoadedLevel->Actors)
	{
		// If load balancing is disabled, we must be the GSM-authoritative worker, so set Role_Authority
		// otherwise, load balancing is enabled, so check the lb strategy.
		if (Actor->GetIsReplicated())
		{
			const bool bRoleAuthoritative = !bLoadBalancingEnabled || LoadBalanceStrategy->ShouldHaveAuthority(*Actor);
			Actor->Role = bRoleAuthoritative ? ROLE_Authority : ROLE_SimulatedProxy;
			Actor->RemoteRole = bRoleAuthoritative ? ROLE_SimulatedProxy : ROLE_Authority;
		}
	}
}

// NOTE: This method is a clone of the ProcessServerTravel located in GameModeBase with modifications to support Spatial.
// Will be called via a delegate that has been set in the UWorld instead of the original in GameModeBase.
void USpatialNetDriver::SpatialProcessServerTravel(const FString& URL, bool bAbsolute, AGameModeBase* GameMode)
{
#if WITH_SERVER_CODE

	UWorld* World = GameMode->GetWorld();
	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(World->GetNetDriver());

	if (!NetDriver->StaticComponentView->HasAuthority(SpatialConstants::INITIAL_GLOBAL_STATE_MANAGER_ENTITY_ID, SpatialConstants::DEPLOYMENT_MAP_COMPONENT_ID))
	{
		// TODO: UNR-678 Send a command to the GSM to initiate server travel on the correct server.
		UE_LOG(LogGameMode, Warning, TEXT("Trying to server travel on a server which is not authoritative over the GSM."));
		return;
	}

	NetDriver->GlobalStateManager->ResetGSM();

	GameMode->StartToLeaveMap();

	// Force an old style load screen if the server has been up for a long time so that TimeSeconds doesn't overflow and break everything
	bool bSeamless = (GameMode->bUseSeamlessTravel && World->TimeSeconds < 172800.0f); // 172800 seconds == 48 hours

	FString NextMap;
	if (URL.ToUpper().Contains(TEXT("?RESTART")))
	{
		NextMap = UWorld::RemovePIEPrefix(GameMode->GetOutermost()->GetName());
	}
	else
	{
		int32 OptionStart = URL.Find(TEXT("?"));
		if (OptionStart == INDEX_NONE)
		{
			NextMap = URL;
		}
		else
		{
			NextMap = URL.Left(OptionStart);
		}
	}

	FGuid NextMapGuid = UEngine::GetPackageGuid(FName(*NextMap), World->IsPlayInEditor());

	FString NewURL = URL;

	if (!NewURL.Contains(SpatialConstants::SpatialSessionIdURLOption))
	{
		int32 NextSessionId = NetDriver->GlobalStateManager->GetSessionId() + 1;
		NewURL.Append(FString::Printf(TEXT("?spatialSessionId=%d"), NextSessionId));
	}

	// Notify clients we're switching level and give them time to receive.
	FString URLMod = NewURL;
	APlayerController* LocalPlayer = GameMode->ProcessClientTravel(URLMod, NextMapGuid, bSeamless, bAbsolute);

	// We can't have the NextURL set this early when using SpatialProcessServerTravel so empty the string here.
	// The reason for this is, on the next WorldTick the current World and NetDriver will be unloaded.
	// During the deployment wipe we are waiting for an entity query response of all entities in the deployment.
	// If the NetDriver has been unloaded in that time, the delegate to delete all these entities will be lost and server travel will fail.
	World->NextURL.Empty();

	ENetMode NetMode = GameMode->GetNetMode();

	// FinishServerTravel - Allows Unreal to finish it's normal server travel.
	PostWorldWipeDelegate FinishServerTravel;
	FinishServerTravel.BindLambda([World, NetDriver, NewURL, NetMode, bSeamless, bAbsolute]
	{
		UE_LOG(LogGameMode, Log, TEXT("SpatialServerTravel - Finishing Server Travel : %s"), *NewURL);
		check(World);
		World->NextURL = NewURL;

		if (bSeamless)
		{
			World->SeamlessTravel(World->NextURL, bAbsolute);
			World->NextURL = TEXT("");
		}
		// Switch immediately if not networking.
		else if (NetMode != NM_DedicatedServer && NetMode != NM_ListenServer)
		{
			World->NextSwitchCountdown = 0.0f;
		}
	});

	UE_LOG(LogGameMode, Log, TEXT("SpatialServerTravel - Wiping the world"), *NewURL);
	NetDriver->WipeWorld(FinishServerTravel);
#endif // WITH_SERVER_CODE
}

void USpatialNetDriver::BeginDestroy()
{
	Super::BeginDestroy();

	if (Connection != nullptr)
	{
		// Cleanup our corresponding worker entity if it exists.
		if (WorkerEntityId != SpatialConstants::INVALID_ENTITY_ID)
		{
			Connection->SendDeleteEntityRequest(WorkerEntityId);
		}

		// Destroy the connection to disconnect from SpatialOS if we aren't meant to persist it.
		if (!bPersistSpatialConnection)
		{
			if (UWorld* LocalWorld = GetWorld())
			{
				Cast<USpatialGameInstance>(LocalWorld->GetGameInstance())->DestroySpatialConnectionManager();
			}
			Connection = nullptr;
		}
	}

#if WITH_EDITOR
	// Ensure our OnDeploymentStart delegate is removed when the net driver is shut down.
	if (FSpatialGDKServicesModule* GDKServices = FModuleManager::GetModulePtr<FSpatialGDKServicesModule>("SpatialGDKServices"))
	{
		GDKServices->GetLocalDeploymentManager()->OnDeploymentStart.Remove(SpatialDeploymentStartHandle);
	}
#endif

	ActorGroupManager = nullptr;
}

void USpatialNetDriver::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// GuidCache will be allocated as an FNetGUIDCache above. To avoid an engine code change, we re-do it with the Spatial equivalent.
		GuidCache = MakeShareable(new FSpatialNetGUIDCache(this));
	}
}

bool USpatialNetDriver::IsLevelInitializedForActor(const AActor* InActor, const UNetConnection* InConnection) const
{
	//In our case, the connection is not specific to a client. Thus, it's not relevant whether the level is initialized.
	return true;
}

void USpatialNetDriver::NotifyActorDestroyed(AActor* ThisActor, bool IsSeamlessTravel /*= false*/)
{
	// Intentionally does not call Super::NotifyActorDestroyed, but most of the functionality is copied here
	// The native UNetDriver would normally store destruction info here for "StartupActors" - replicated actors
	// placed in the level, but we handle this flow differently in the GDK

	// In single process PIE sessions this can be called on the server with actors from a client when the client unloads a level.
	// Such actors will not have a valid entity ID.
	// As only clients unload a level, if an actor has an entity ID and authority then it can not be such a spurious entity.

	// Remove the actor from the property tracker map
	RepChangedPropertyTrackerMap.Remove(ThisActor);

	const bool bIsServer = ServerConnection == nullptr;
	if (bIsServer)
	{
		// Check if this is a dormant entity, and if so retire the entity
		if (PackageMap != nullptr && World != nullptr)
		{
			const Worker_EntityId EntityId = PackageMap->GetEntityIdFromObject(ThisActor);

			// It is safe to check that we aren't destroying a singleton actor on a server if there is a valid entity ID and this is not a client.
			if (ThisActor->GetClass()->HasAnySpatialClassFlags(SPATIALCLASS_Singleton) && EntityId != SpatialConstants::INVALID_ENTITY_ID)
			{
				UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Removed a singleton actor on a server. This should never happen. "
					"Actor: %s."), *ThisActor->GetName());
			}

			// If the actor is an initially dormant startup actor that has not been replicated.
			if (EntityId == SpatialConstants::INVALID_ENTITY_ID && ThisActor->IsNetStartupActor() && ThisActor->GetIsReplicated() && ThisActor->HasAuthority())
			{
				UE_LOG(LogSpatialOSNetDriver, Log, TEXT("Creating a tombstone entity for initially dormant statup actor. "
					"Actor: %s."), *ThisActor->GetName());
				Sender->CreateTombstoneEntity(ThisActor);
			}
			else if (IsDormantEntity(EntityId) && ThisActor->HasAuthority())
			{
				// Deliberately don't unregister the dormant entity, but let it get cleaned up in the entity remove op process
				if (!StaticComponentView->HasAuthority(EntityId, SpatialGDK::Position::ComponentId))
				{
					UE_LOG(LogSpatialOSNetDriver, Warning, TEXT("Retiring dormant entity that we don't have spatial authority over [%lld][%s]"), EntityId, *ThisActor->GetName());
				}
				Sender->RetireEntity(EntityId);
			}
		}

		for (int32 i = ClientConnections.Num() - 1; i >= 0; i--)
		{
			UNetConnection* ClientConnection = ClientConnections[i];
			if (ThisActor->bNetTemporary)
			{
				ClientConnection->SentTemporaries.Remove(ThisActor);
			}

			if (UActorChannel* Channel = ClientConnection->ActorChannelMap().FindRef(ThisActor))
			{
				check(Channel->OpenedLocally);
				Channel->bClearRecentActorRefs = false;
				// TODO: UNR-952 - Add code here for cleaning up actor channels from our maps.
				Channel->Close(EChannelCloseReason::Destroyed);
			}

			// Remove it from any dormancy lists
			ClientConnection->DormantReplicatorMap.Remove(ThisActor);
		}
	}

	// Remove this actor from the network object list
	GetNetworkObjectList().Remove(ThisActor);

	// Remove from renamed list if destroyed
	RenamedStartupActors.Remove(ThisActor->GetFName());
}

void USpatialNetDriver::Shutdown()
{
	if (!IsServer())
	{
		// Notify the server that we're disconnecting so it can clean up our actors.
		if (USpatialNetConnection* SpatialNetConnection = Cast<USpatialNetConnection>(ServerConnection))
		{
			SpatialNetConnection->ClientNotifyClientHasQuit();
		}
	}

	SpatialOutputDevice = nullptr;

	Super::Shutdown();

	// This is done after Super::Shutdown so the NetDriver is given an opportunity to shutdown all open channels, and those
	// startup actors that were tombstoned, will be cleaned up also.
#if WITH_EDITOR
	const bool bDeleteDynamicEntities = GetDefault<ULevelEditorPlaySettings>()->GetDeleteDynamicEntities();

	if (bDeleteDynamicEntities && IsServer())
	{
		for (const Worker_EntityId EntityId : DormantEntities)
		{
			if (StaticComponentView->HasAuthority(EntityId, SpatialGDK::Position::ComponentId))
			{
				Connection->SendDeleteEntityRequest(EntityId);
			}
		}

		for (const Worker_EntityId EntityId : TombstonedEntities)
		{
			if (StaticComponentView->HasAuthority(EntityId, SpatialGDK::Position::ComponentId))
			{
				Connection->SendDeleteEntityRequest(EntityId);
			}
		}
	}
#endif //WITH_EDITOR
}

void USpatialNetDriver::NotifyActorFullyDormantForConnection(AActor* Actor, UNetConnection* NetConnection)
{
	// Similar to NetDriver::NotifyActorFullyDormantForConnection, however we only care about a single connection
	const int NumConnections = 1;
	GetNetworkObjectList().MarkDormant(Actor, NetConnection, NumConnections, this);

	if (UReplicationDriver* RepDriver = GetReplicationDriver())
	{
		RepDriver->NotifyActorFullyDormantForConnection(Actor, NetConnection);
	}

	// Intentionally don't call Super::NotifyActorFullyDormantForConnection
}

void USpatialNetDriver::OnOwnerUpdated(AActor* Actor, AActor* OldOwner)
{
	if (!IsServer())
	{
		return;
	}

	if (LockingPolicy != nullptr)
	{
		LockingPolicy->OnOwnerUpdated(Actor, OldOwner);
	}

	// If PackageMap doesn't exist, we haven't connected yet, which means
	// we don't need to update the interest at this point
	if (PackageMap == nullptr)
	{
		return;
	}

	Worker_EntityId EntityId = PackageMap->GetEntityIdFromObject(Actor);
	if (EntityId == SpatialConstants::INVALID_ENTITY_ID)
	{
		return;
	}

	USpatialActorChannel* Channel = GetActorChannelByEntityId(EntityId);
	if (Channel == nullptr)
	{
		return;
	}

	Channel->MarkInterestDirty();

	Channel->ServerProcessOwnershipChange();
}

//SpatialGDK: Functions in the ifdef block below are modified versions of the UNetDriver:: implementations.
#if WITH_SERVER_CODE

// Returns true if this actor should replicate to *any* of the passed in connections
static FORCEINLINE_DEBUGGABLE bool IsActorRelevantToConnection(const AActor* Actor, UActorChannel* ActorChannel, const TArray<FNetViewer>& ConnectionViewers)
{
	// An actor without a channel yet will need to be replicated at least
	// once to have a channel and entity created for it
	if (ActorChannel == nullptr)
	{
		return true;
	}

	for (const auto& Viewer : ConnectionViewers)
	{
		if (Actor->IsNetRelevantFor(Viewer.InViewer, Viewer.ViewTarget, Viewer.ViewLocation))
		{
			return true;
		}
	}

	return false;
}

// Returns true if this actor is considered dormant (and all properties caught up) to the current connection
static FORCEINLINE_DEBUGGABLE bool IsActorDormant(FNetworkObjectInfo* ActorInfo, UNetConnection* Connection)
{
	// If actor is already dormant on this channel, then skip replication entirely
	return ActorInfo->DormantConnections.Contains(Connection);
}

// Returns true if this actor wants to go dormant for a particular connection
static FORCEINLINE_DEBUGGABLE bool ShouldActorGoDormant(AActor* Actor, const TArray<FNetViewer>& ConnectionViewers, UActorChannel* Channel, const float Time, const bool bLowNetBandwidth)
{
	if (Actor->NetDormancy <= DORM_Awake || !Channel || Channel->bPendingDormancy || Channel->Dormant)
	{
		// Either shouldn't go dormant, or is already dormant
		return false;
	}

	if (Actor->NetDormancy == DORM_DormantPartial)
	{
		for (int32 viewerIdx = 0; viewerIdx < ConnectionViewers.Num(); viewerIdx++)
		{
			if (!Actor->GetNetDormancy(ConnectionViewers[viewerIdx].ViewLocation, ConnectionViewers[viewerIdx].ViewDir, ConnectionViewers[viewerIdx].InViewer, ConnectionViewers[viewerIdx].ViewTarget, Channel, Time, bLowNetBandwidth))
			{
				return false;
			}
		}
	}

	return true;
}

int32 USpatialNetDriver::ServerReplicateActors_PrepConnections(const float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_SpatialPrioritizeActors);

	int32 NumClientsToTick = ClientConnections.Num();

	bool bFoundReadyConnection = false;

	for (int32 ConnIdx = 0; ConnIdx < ClientConnections.Num(); ConnIdx++)
	{
		USpatialNetConnection* SpatialConnection = Cast<USpatialNetConnection>(ClientConnections[ConnIdx]);
		check(SpatialConnection);
		check(SpatialConnection->State == USOCK_Pending || SpatialConnection->State == USOCK_Open || SpatialConnection->State == USOCK_Closed);
		checkSlow(SpatialConnection->GetUChildConnection() == NULL);

		// Handle not ready channels.
		//@note: we cannot add Connection->IsNetReady(0) here to check for saturation, as if that's the case we still want to figure out the list of relevant actors
		//			to reset their NetUpdateTime so that they will get sent as soon as the connection is no longer saturated
		AActor* OwningActor = SpatialConnection->OwningActor;

		//SpatialGDK: We allow a connection without an owner to process if it's meant to be the connection to the fake SpatialOS client.
		if ((SpatialConnection->bReliableSpatialConnection || OwningActor != NULL) && SpatialConnection->State == USOCK_Open && (SpatialConnection->Driver->Time - SpatialConnection->LastReceiveTime < 1.5f))
		{
			check(SpatialConnection->bReliableSpatialConnection || World == OwningActor->GetWorld());

			bFoundReadyConnection = true;

			// the view target is what the player controller is looking at OR the owning actor itself when using beacons
			SpatialConnection->ViewTarget = SpatialConnection->PlayerController ? SpatialConnection->PlayerController->GetViewTarget() : OwningActor;
		}
		else
		{
			SpatialConnection->ViewTarget = NULL;
		}

		if (SpatialConnection->Children.Num() > 0)
		{
			UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Child connections present on Spatial connection %s! We don't support splitscreen yet, so this will not function correctly."), *SpatialConnection->GetName());
		}
	}

	return bFoundReadyConnection ? NumClientsToTick : 0;
}

int32 USpatialNetDriver::ServerReplicateActors_PrioritizeActors(UNetConnection* InConnection, const TArray<FNetViewer>& ConnectionViewers, const TArray<FNetworkObjectInfo*> ConsiderList, const bool bCPUSaturated, FActorPriority*& OutPriorityList, FActorPriority**& OutPriorityActors)
{
	// Since this function signature is copied from NetworkDriver.cpp, I don't want to change the signature. But we expect
	// that the input connection will be the SpatialOS server connection to the runtime (the first client connection),
	// so let's make sure that assumption continues to hold.
	check(InConnection != nullptr);
	check(GetSpatialOSNetConnection() != nullptr);
	check(InConnection == GetSpatialOSNetConnection());

	// Get list of visible/relevant actors.

	NetTag++;
	InConnection->TickCount++;

	// Set up to skip all sent temporary actors
	for (int32 j = 0; j < InConnection->SentTemporaries.Num(); j++)
	{
		InConnection->SentTemporaries[j]->NetTag = NetTag;
	}

	int32 FinalSortedCount = 0;
	int32 DeletedCount = 0;

	const int32 MaxSortedActors = ConsiderList.Num() + DestroyedStartupOrDormantActors.Num();
	if (MaxSortedActors > 0)
	{
		OutPriorityList = new (FMemStack::Get(), MaxSortedActors) FActorPriority;
		OutPriorityActors = new (FMemStack::Get(), MaxSortedActors) FActorPriority*;

		AGameNetworkManager* const NetworkManager = World->NetworkManager;
		const bool bLowNetBandwidth = NetworkManager ? NetworkManager->IsInLowBandwidthMode() : false;

		const bool bNetRelevancyEnabled = GetDefault<USpatialGDKSettings>()->bUseIsActorRelevantForConnection;

		for (FNetworkObjectInfo* ActorInfo : ConsiderList)
		{
			AActor* Actor = ActorInfo->Actor;

			UActorChannel* Channel = InConnection->ActorChannelMap().FindRef(Actor);

			UNetConnection* PriorityConnection = InConnection;

			// Skip Actor if dormant
			if (IsActorDormant(ActorInfo, InConnection))
			{
				continue;
			}

			// See of actor wants to try and go dormant
			if (ShouldActorGoDormant(Actor, ConnectionViewers, Channel, Time, bLowNetBandwidth))
			{
				// Channel is marked to go dormant now once all properties have been replicated (but is not dormant yet)
				Channel->StartBecomingDormant();
			}

			UE_LOG(LogSpatialOSNetDriver, Verbose, TEXT("Actor %s will be replicated on the catch-all connection"), *Actor->GetName());

			// Check actor relevancy if Net Relevancy is enabled in the GDK settings
			if (bNetRelevancyEnabled && !IsActorRelevantToConnection(Actor, Channel, ConnectionViewers))
			{
				// Early out and do not replicate if actor is not relevant
				continue;
			}

			// Actor is relevant to this connection, add it to the list
			// NOTE - We use NetTag to make sure SentTemporaries didn't already mark this actor to be skipped
			if (Actor->NetTag != NetTag)
			{
				UE_LOG(LogNetTraffic, Log, TEXT("Consider %s alwaysrelevant %d frequency %f "), *Actor->GetName(), Actor->bAlwaysRelevant, Actor->NetUpdateFrequency);

				Actor->NetTag = NetTag;

				OutPriorityList[FinalSortedCount] = FActorPriority(PriorityConnection, Channel, ActorInfo, ConnectionViewers, bLowNetBandwidth);
				OutPriorityActors[FinalSortedCount] = OutPriorityList + FinalSortedCount;

				FinalSortedCount++;

				if (DebugRelevantActors)
				{
					LastPrioritizedActors.Add(Actor);
				}
			}
		}

		// Add in deleted actors
		for (auto It = InConnection->GetDestroyedStartupOrDormantActorGUIDs().CreateIterator(); It; ++It)
		{
			FActorDestructionInfo& DInfo = *DestroyedStartupOrDormantActors.FindChecked(*It);
			OutPriorityList[FinalSortedCount] = FActorPriority(InConnection, &DInfo, ConnectionViewers);
			OutPriorityActors[FinalSortedCount] = OutPriorityList + FinalSortedCount;
			FinalSortedCount++;
			DeletedCount++;
		}

		// Sort by priority
		Sort(OutPriorityActors, FinalSortedCount, FCompareFActorPriority());
	}

	UE_LOG(LogNetTraffic, Log, TEXT("ServerReplicateActors_PrioritizeActors: Potential %04i ConsiderList %03i FinalSortedCount %03i"), MaxSortedActors, ConsiderList.Num(), FinalSortedCount);

	return FinalSortedCount;
}

void USpatialNetDriver::ServerReplicateActors_ProcessPrioritizedActors(UNetConnection* InConnection, const TArray<FNetViewer>& ConnectionViewers, FActorPriority** PriorityActors, const int32 FinalSortedCount, int32& OutUpdated)
{
	SCOPE_CYCLE_COUNTER(STAT_SpatialProcessPrioritizedActors);

	// Since this function signature is copied from NetworkDriver.cpp, I don't want to change the signature. But we expect
	// that the input connection will be the SpatialOS server connection to the runtime (the first client connection),
	// so let's make sure that assumption continues to hold.
	check(InConnection != nullptr);
	check(GetSpatialOSNetConnection() != nullptr);
	check(InConnection == GetSpatialOSNetConnection());

	SET_DWORD_STAT(STAT_SpatialActorsRelevant, 0);
	SET_DWORD_STAT(STAT_SpatialActorsChanged, 0);

	// SpatialGDK - Here Unreal would check if the InConnection was saturated (!IsNetReady) and early out. Removed this as we do not currently use channel saturation.

	int32 ActorUpdatesThisConnection = 0;
	int32 ActorUpdatesThisConnectionSent = 0;

	// SpatialGDK - Entity creation rate limiting based on config value.
	uint32 EntityCreationRateLimit = GetDefault<USpatialGDKSettings>()->EntityCreationRateLimit;
	int32 MaxEntitiesToCreate = (EntityCreationRateLimit > 0) ? EntityCreationRateLimit : INT32_MAX;
	int32 FinalCreationCount = 0;

	// SpatialGDK - Actor replication rate limiting based on config value.
	uint32 ActorReplicationRateLimit = GetDefault<USpatialGDKSettings>()->ActorReplicationRateLimit;
	int32 MaxActorsToReplicate = (ActorReplicationRateLimit > 0) ? ActorReplicationRateLimit : INT32_MAX;
	int32 FinalReplicatedCount = 0;

	for (int32 j = 0; j < FinalSortedCount; j++)
	{
		// Deletion entry
		if (PriorityActors[j]->ActorInfo == NULL && PriorityActors[j]->DestructionInfo)
		{
			// Make sure client has streaming level loaded
			if (PriorityActors[j]->DestructionInfo->StreamingLevelName != NAME_None && !InConnection->ClientVisibleLevelNames.Contains(PriorityActors[j]->DestructionInfo->StreamingLevelName))
			{
				// This deletion entry is for an actor in a streaming level the connection doesn't have loaded, so skip it
				continue;
			}
			UActorChannel* Channel = (UActorChannel*)InConnection->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally);
			if (Channel)
			{
				UE_LOG(LogNetTraffic, Log, TEXT("Server replicate actor creating destroy channel for NetGUID <%s,%s> Priority: %d"), *PriorityActors[j]->DestructionInfo->NetGUID.ToString(), *PriorityActors[j]->DestructionInfo->PathName, PriorityActors[j]->Priority);

				InConnection->GetDestroyedStartupOrDormantActorGUIDs().Remove(PriorityActors[j]->DestructionInfo->NetGUID); // Remove from connections to-be-destroyed list (close bunch of reliable, so it will make it there)
			}
			continue;
		}

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
		static IConsoleVariable* DebugObjectCvar = IConsoleManager::Get().FindConsoleVariable(TEXT("net.PackageMap.DebugObject"));
		static IConsoleVariable* DebugAllObjectsCvar = IConsoleManager::Get().FindConsoleVariable(TEXT("net.PackageMap.DebugAll"));
		if (PriorityActors[j]->ActorInfo &&
			((DebugObjectCvar && !DebugObjectCvar->GetString().IsEmpty() && PriorityActors[j]->ActorInfo->Actor->GetName().Contains(DebugObjectCvar->GetString())) ||
			(DebugAllObjectsCvar && DebugAllObjectsCvar->GetInt() != 0)))
		{
			UE_LOG(LogNetPackageMap, Log, TEXT("Evaluating actor for replication %s"), *PriorityActors[j]->ActorInfo->Actor->GetName());
		}
#endif

		// Normal actor replication
		USpatialActorChannel* Channel = Cast<USpatialActorChannel>(PriorityActors[j]->Channel);
		UE_LOG(LogNetTraffic, Log, TEXT(" Maybe Replicate %s"), *PriorityActors[j]->ActorInfo->Actor->GetName());
		if (Channel == nullptr || Channel->Actor != nullptr) // Make sure didn't just close this channel.
		{
			AActor* Actor = PriorityActors[j]->ActorInfo->Actor;
			bool bIsRelevant = false;

			// SpatialGDK: Here, Unreal would check (again) whether an actor is relevant. Removed such checks.
			// only check visibility on already visible actors every 1.0 + 0.5R seconds
			// bTearOff actors should never be checked
			if (!Actor->GetTearOff() && (!Channel || Time - Channel->RelevantTime > 1.f))
			{
				if (DebugRelevantActors)
				{
					LastNonRelevantActors.Add(Actor);
				}
			}

			// SpatialGDK - Creation of new entities should always be handled and therefore is checked prior to actor throttling.
			// There is an EntityCreationRateLimit to prevent overloading Spatial with creation requests if the developer desires.
			// Creation of a new entity occurs when the channel is currently nullptr or if the channel does not have bCreatedEntity set to true.
			if (FinalCreationCount < MaxEntitiesToCreate && !Actor->GetTearOff() && (Channel == nullptr || Channel->bCreatingNewEntity))
			{
				bIsRelevant = true;
				FinalCreationCount++;
			}
			// SpatialGDK - We will only replicate the highest priority actors up the the rate limit and the final tick of TearOff actors.
			// Actors not replicated this frame will have their priority increased based on the time since the last replicated.
			// TearOff actors would normally replicate their final tick due to RecentlyRelevant, after which the channel is closed.
			// With throttling we no longer always replicate when RecentlyRelevant is true, thus we ensure to always replicate a TearOff actor while it still has a channel.
			else if ((FinalReplicatedCount < MaxActorsToReplicate && !Actor->GetTearOff()) || (Actor->GetTearOff() && Channel != nullptr))
			{
				bIsRelevant = true;
				FinalReplicatedCount++;
			}

			// If the actor is now relevant or was recently relevant.
			const bool bIsRecentlyRelevant = bIsRelevant || (Channel && Time - Channel->RelevantTime < RelevantTimeout);

			if (bIsRecentlyRelevant)
			{
				// Find or create the channel for this actor.
				// we can't create the channel if the client is in a different world than we are
				// or the package map doesn't support the actor's class/archetype (or the actor itself in the case of serializable actors)
				// or it's an editor placed actor and the client hasn't initialized the level it's in
				if (Channel == nullptr && GuidCache->SupportsObject(Actor->GetClass()) && GuidCache->SupportsObject(Actor->IsNetStartupActor() ? Actor : Actor->GetArchetype()))
				{
					if (!Actor->GetClass()->HasAnySpatialClassFlags(SPATIALCLASS_SpatialType))
					{
						// Trying to replicate an actor that isn't supported by Spatial (e.g. marked NotSpatial)
						continue;
					}

					check(Actor->HasAuthority());

					Channel = GetOrCreateSpatialActorChannel(Actor);
					if ((Channel == nullptr) && (Actor->NetUpdateFrequency < 1.0f))
					{
						UE_LOG(LogNetTraffic, Log, TEXT("Unable to replicate %s"), *Actor->GetName());
						PriorityActors[j]->ActorInfo->NextUpdateTime = Actor->GetWorld()->TimeSeconds + 0.2f * FMath::FRand();
					}
				}

				// SpatialGDK - Only replicate actors marked as relevant (rate limiting).
				if (Channel && bIsRelevant)
				{
					// If it is relevant then mark the channel as relevant for a short amount of time.
					Channel->RelevantTime = Time + 0.5f * FMath::SRand();

					// If the channel isn't saturated.
					if (Channel->IsNetReady(0))
					{
						// Replicate the actor.
						UE_LOG(LogNetTraffic, Log, TEXT("- Replicate %s. %d"), *Actor->GetName(), PriorityActors[j]->Priority);
						if (DebugRelevantActors)
						{
							LastRelevantActors.Add(Actor);
						}

						if (Channel->ReplicateActor())
						{
							ActorUpdatesThisConnectionSent++;
							if (DebugRelevantActors)
							{
								LastSentActors.Add(Actor);
							}

							// Calculate min delta (max rate actor will update), and max delta (slowest rate actor will update)
							const float MinOptimalDelta = 1.0f / Actor->NetUpdateFrequency;
							const float MaxOptimalDelta = FMath::Max(1.0f / Actor->MinNetUpdateFrequency, MinOptimalDelta);
							const float DeltaBetweenReplications = (World->TimeSeconds - PriorityActors[j]->ActorInfo->LastNetReplicateTime);

							// Choose an optimal time, we choose 70% of the actual rate to allow frequency to go up if needed
							PriorityActors[j]->ActorInfo->OptimalNetUpdateDelta = FMath::Clamp(DeltaBetweenReplications * 0.7f, MinOptimalDelta, MaxOptimalDelta);
							PriorityActors[j]->ActorInfo->LastNetReplicateTime = World->TimeSeconds;
						}

						ActorUpdatesThisConnection++;
						OutUpdated++;
					}

					// SpatialGDK - Here Unreal would do a second check for channel saturation and early out if needed. Removed such checks.
				}
			}

			// UNR-865 - Handle closing actor channels for non-relevant actors without deleting the entity.
			// If the actor wasn't recently relevant, or if it was torn off, close the actor channel if it exists for this connection
			if (Actor->GetTearOff() && Channel != NULL)
			{
				// Non startup (map) actors have their channels closed immediately, which destroys them.
				// Startup actors get to keep their channels open.
				if (!Actor->IsNetStartupActor())
				{
					UE_LOG(LogNetTraffic, Log, TEXT("- Closing channel for no longer relevant actor %s"), *Actor->GetName());
					// TODO: UNR-952 - Add code here for cleaning up actor channels from our maps.
					Channel->Close(Actor->GetTearOff() ? EChannelCloseReason::TearOff : EChannelCloseReason::Relevancy);
				}
			}
		}
	}

	SET_DWORD_STAT(STAT_SpatialActorsRelevant, ActorUpdatesThisConnection);
	SET_DWORD_STAT(STAT_SpatialActorsChanged, ActorUpdatesThisConnectionSent);

	// SpatialGDK - Here Unreal would return the position of the last replicated actor in PriorityActors before the channel became saturated.
	// In Spatial we use ActorReplicationRateLimit and EntityCreationRateLimit to limit replication so this return value is not relevant.
}

#endif // WITH_SERVER_CODE

void USpatialNetDriver::ProcessRPC(AActor* Actor, UObject* SubObject, UFunction* Function, void* Parameters)
{
	// The RPC might have been called by an actor directly, or by a subobject on that actor
	UObject* CallingObject = SubObject != nullptr ? SubObject : Actor;

	if (IsServer())
	{
		// Creating channel to ensure that object will be resolvable
		GetOrCreateSpatialActorChannel(CallingObject);
	}

	// If this object's class isn't present in the schema database, we will log an error and tell the
	// game to quit. Unfortunately, there's one more tick after that during which RPCs could be called.
	// Check that the class is supported so we don't crash in USpatialClassInfoManager::GetRPCInfo.
	if (!Sender->ValidateOrExit_IsSupportedClass(CallingObject->GetClass()->GetPathName()))
	{
		return;
	}

	FUnrealObjectRef CallingObjectRef = PackageMap->GetUnrealObjectRefFromObject(CallingObject);
	if (!CallingObjectRef.IsValid())
	{
		UE_LOG(LogSpatialOSNetDriver, Warning, TEXT("The target object %s is unresolved; RPC %s will be dropped."), *CallingObject->GetFullName(), *Function->GetName());
		return;
	}
	RPCPayload Payload = Sender->CreateRPCPayloadFromParams(CallingObject, CallingObjectRef, Function, Parameters);

	Sender->ProcessOrQueueOutgoingRPC(CallingObjectRef, MoveTemp(Payload));
}

// SpatialGDK: This is a modified and simplified version of UNetDriver::ServerReplicateActors.
// In our implementation, connections on the server do not represent clients. They represent direct connections to SpatialOS.
// For this reason, things like ready checks, acks, throttling based on number of updated connections, interest management are irrelevant at this level.
int32 USpatialNetDriver::ServerReplicateActors(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_SpatialServerReplicateActors);
	SET_DWORD_STAT(STAT_NumReplicatedActorBytes, 0);
	SET_DWORD_STAT(STAT_NumReplicatedActors, 0);

#if WITH_SERVER_CODE
	// Only process the stand-in client connection, which is the connection to the runtime itself.
	// It will be responsible for replicating all actors, regardless of whether they're owned by a client.
	USpatialNetConnection* SpatialConnection = GetSpatialOSNetConnection();
	if (SpatialConnection == nullptr)
	{
		return 0;
	}
	check(SpatialConnection->bReliableSpatialConnection);

	if (UReplicationDriver* RepDriver = GetReplicationDriver())
	{
		return RepDriver->ServerReplicateActors(DeltaSeconds);
	}

	check(World);

	int32 Updated = 0;

	// Bump the ReplicationFrame value to invalidate any properties marked as "unchanged" for this frame.
	ReplicationFrame++;

	const int32 NumClientsToTick = ServerReplicateActors_PrepConnections(DeltaSeconds);

	// SpatialGDK: This is a formality as there is at least one "perfect" Spatial connection in our design.
	if (NumClientsToTick == 0)
	{
		// No connections are ready this frame
		return 0;
	}

	AWorldSettings* WorldSettings = World->GetWorldSettings();

	bool bCPUSaturated = false;
	float ServerTickTime = GEngine->GetMaxTickRate(DeltaSeconds);
	if (ServerTickTime == 0.f)
	{
		ServerTickTime = DeltaSeconds;
	}
	else
	{
		ServerTickTime = 1.f / ServerTickTime;
		bCPUSaturated = DeltaSeconds > 1.2f * ServerTickTime;
	}

	SET_DWORD_STAT(STAT_SpatialConsiderList, 0);

	TArray<FNetworkObjectInfo*> ConsiderList;
	ConsiderList.Reserve(GetNetworkObjectList().GetActiveObjects().Num());

	// Build the consider list (actors that are ready to replicate)
	ServerReplicateActors_BuildConsiderList(ConsiderList, ServerTickTime);

	SET_DWORD_STAT(STAT_SpatialConsiderList, ConsiderList.Num());

	FMemMark Mark(FMemStack::Get());

	// Make a list of viewers this connection should consider
	TArray<FNetViewer>& ConnectionViewers = WorldSettings->ReplicationViewers;

	ConnectionViewers.Reset();

	// The fake spatial connection will borrow the player controllers from other connections.
	for (int i = 1; i < ClientConnections.Num(); i++)
	{
		USpatialNetConnection* ClientConnection = Cast<USpatialNetConnection>(ClientConnections[i]);
		check(ClientConnection);

		if (ClientConnection->ViewTarget != nullptr)
		{
			new(ConnectionViewers)FNetViewer(ClientConnection, DeltaSeconds);

			// send ClientAdjustment if necessary
			// we do this here so that we send a maximum of one per packet to that client; there is no value in stacking additional corrections
			if (ClientConnection->PlayerController != nullptr)
			{
				ClientConnection->PlayerController->SendClientAdjustment();
			}

			if (ClientConnection->Children.Num() > 0)
			{
				UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Child connections present on Spatial client connection %s! We don't support splitscreen yet, so this will not function correctly."), *ClientConnection->GetName());
			}
		}
	}

	FMemMark RelevantActorMark(FMemStack::Get());

	FActorPriority* PriorityList = NULL;
	FActorPriority** PriorityActors = NULL;

	// Get a sorted list of actors for this connection
	const int32 FinalSortedCount = ServerReplicateActors_PrioritizeActors(SpatialConnection, ConnectionViewers, ConsiderList, bCPUSaturated, PriorityList, PriorityActors);

	// Process the sorted list of actors for this connection
	ServerReplicateActors_ProcessPrioritizedActors(SpatialConnection, ConnectionViewers, PriorityActors, FinalSortedCount, Updated);

	// SpatialGDK - Here Unreal would mark relevant actors that weren't processed this frame as bPendingNetUpdate. This is not used in the SpatialGDK and so has been removed.

	RelevantActorMark.Pop();
	ConnectionViewers.Reset();

	Mark.Pop();

	if (DebugRelevantActors)
	{
		PrintDebugRelevantActors();
		LastPrioritizedActors.Empty();
		LastSentActors.Empty();
		LastRelevantActors.Empty();
		LastNonRelevantActors.Empty();

		DebugRelevantActors = false;
	}

#if !UE_BUILD_SHIPPING
	ConsiderListSize = FinalSortedCount;
#endif

	return Updated;
#else
	return 0;
#endif // WITH_SERVER_CODE
}

void USpatialNetDriver::TickDispatch(float DeltaTime)
{
	// Not calling Super:: on purpose.
	UNetDriver::TickDispatch(DeltaTime);

	if (Connection != nullptr)
	{
		const USpatialGDKSettings* SpatialGDKSettings = GetDefault<USpatialGDKSettings>();
		if (SpatialGDKSettings->bRunSpatialWorkerConnectionOnGameThread)
		{
			Connection->QueueLatestOpList();
		}

		TArray<Worker_OpList*> OpLists = Connection->GetOpList();

		// Servers will queue ops at startup until we've extracted necessary information from the op stream
		if (!bIsReadyToStart)
		{
			HandleStartupOpQueueing(OpLists);
			return;
		}

		{
			SCOPE_CYCLE_COUNTER(STAT_SpatialProcessOps);
			for (Worker_OpList* OpList : OpLists)
			{
				Dispatcher->ProcessOps(OpList);

				Worker_OpList_Destroy(OpList);
			}
		}

		if (SpatialMetrics != nullptr && SpatialGDKSettings->bEnableMetrics)
		{
			SpatialMetrics->TickMetrics(Time);
		}

		if (LoadBalanceEnforcer.IsValid())
		{
			SCOPE_CYCLE_COUNTER(STAT_SpatialUpdateAuthority);
			for (const auto& AclAssignmentRequest : LoadBalanceEnforcer->ProcessQueuedAclAssignmentRequests())
			{
				Sender->SetAclWriteAuthority(AclAssignmentRequest);
			}
		}
	}
}

void USpatialNetDriver::ProcessRemoteFunction(
	AActor* Actor,
	UFunction* Function,
	void* Parameters,
	FOutParmRec* OutParms,
	FFrame* Stack,
	UObject* SubObject)
{
	if (Connection == nullptr)
	{
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Attempted to call ProcessRemoteFunction before connection was established"));
		return;
	}

	USpatialNetConnection* NetConnection = GetSpatialOSNetConnection();
	if (NetConnection == nullptr)
	{
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Attempted to call ProcessRemoteFunction but no SpatialOSNetConnection existed. Has this worker established a connection?"));
		return;
	}

	// This check mimics the way Unreal natively checks whether an AActor has ownership for sending server RPCs.
	// The function GetNetConnection() goes up the AActor ownership chain until it reaches an AActor that is possesed by an AController and
	// hence a UNetConnection. Server RPCs should only be sent by AActor instances that either are possessed by a UNetConnection or are owned by
	// other AActor instances possessed by a UNetConnection. For native Unreal reference see ProcessRemoteFunction() of IpNetDriver.cpp.
	// However if we are on the server, and the RPC is a CrossServer or NetMulticast RPC, this can be invoked without an owner.
	if (!Actor->GetNetConnection() && !(Function->FunctionFlags & (FUNC_NetCrossServer | FUNC_NetMulticast) && IsServer()))
	{
		UE_LOG(LogSpatialOSNetDriver, Warning, TEXT("No owning connection for actor %s. Function %s will not be processed."), *Actor->GetName(), *Function->GetName());
		return;
	}

	// The RPC might have been called by an actor directly, or by a subobject on that actor
	UObject* CallingObject = SubObject ? SubObject : Actor;

	if (!CallingObject->GetClass()->HasAnySpatialClassFlags(SPATIALCLASS_SpatialType))
	{
		UE_LOG(LogSpatialOSNetDriver, Verbose, TEXT("Trying to call RPC %s on object %s (class %s) that isn't supported by Spatial. This RPC will be dropped."), *Function->GetName(), *CallingObject->GetName(), *CallingObject->GetClass()->GetName());
		return;
	}

	// Copied from UNetDriver::ProcessRemoteFunctionForChannel to copy pass-by-ref
	// parameters from OutParms into Parameters's memory.
	if (Stack == nullptr)
	{
		// Look for CPF_OutParm's, we'll need to copy these into the local parameter memory manually
		// The receiving side will pull these back out when needed
		for (TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_OutParm))
			{
				if (OutParms == nullptr)
				{
					continue;
				}

				FOutParmRec* Out = OutParms;

				while (Out->Property != *It)
				{
					Out = Out->NextOutParm;
				}

				void* Dest = It->ContainerPtrToValuePtr< void >(Parameters);

				const int32 CopySize = It->ElementSize * It->ArrayDim;

				check(((uint8*)Dest - (uint8*)Parameters) + CopySize <= Function->ParmsSize);

				It->CopyCompleteValue(Dest, Out->PropAddr);
			}
		}
	}

	if (Function->FunctionFlags & FUNC_Net)
	{
		ProcessRPC(Actor, SubObject, Function, Parameters);
	}
}

void USpatialNetDriver::PollPendingLoads()
{
	if (PackageMap == nullptr)
	{
		return;
	}

	for (auto IterPending = PackageMap->PendingReferences.CreateIterator(); IterPending; ++IterPending)
	{
		if (PackageMap->IsGUIDPending(*IterPending))
		{
			continue;
		}

		FUnrealObjectRef ObjectReference = PackageMap->GetUnrealObjectRefFromNetGUID(*IterPending);

		bool bOutUnresolved = false;
		UObject* ResolvedObject = FUnrealObjectRef::ToObjectPtr(ObjectReference, PackageMap, bOutUnresolved);
		if (ResolvedObject)
		{
			Receiver->ResolvePendingOperations(ResolvedObject, ObjectReference);
		}
		else
		{
			UE_LOG(LogSpatialPackageMap, Warning, TEXT("Object %s which was being asynchronously loaded was not found after loading has completed."), *ObjectReference.ToString());
		}

		IterPending.RemoveCurrent();
	}
}

void USpatialNetDriver::TickFlush(float DeltaTime)
{
	const USpatialGDKSettings* SpatialGDKSettings = GetDefault<USpatialGDKSettings>();

	PollPendingLoads();

	if (IsServer() && GetSpatialOSNetConnection() != nullptr && PackageMap->IsEntityPoolReady() && bIsReadyToStart)
	{
		// Update all clients.
#if WITH_SERVER_CODE

		int32 Updated = ServerReplicateActors(DeltaTime);

		static int32 LastUpdateCount = 0;
		// Only log the zero replicated actors once after replicating an actor
		if ((LastUpdateCount && !Updated) || Updated)
		{
			UE_LOG(LogNetTraffic, Verbose, TEXT("%s replicated %d actors"), *GetDescription(), Updated);
		}
		LastUpdateCount = Updated;

		if (SpatialGDKSettings->bBatchSpatialPositionUpdates && Sender != nullptr)
		{
			if ((Time - TimeWhenPositionLastUpdated) >= (1.0f / SpatialGDKSettings->PositionUpdateFrequency))
			{
				TimeWhenPositionLastUpdated = Time;

				Sender->ProcessPositionUpdates();
			}
		}

#endif // WITH_SERVER_CODE
	}

	if (SpatialGDKSettings->UseRPCRingBuffer() && Sender != nullptr)
	{
		Sender->FlushRPCService();
	}

	ProcessPendingDormancy();

	TimerManager.Tick(DeltaTime);

	if (SpatialGDKSettings->bRunSpatialWorkerConnectionOnGameThread)
	{
		if (Connection != nullptr)
		{
			Connection->ProcessOutgoingMessages();
		}
	}

	// Super::TickFlush() will not call ReplicateActors() because Spatial connections have InternalAck set to true.
	// In our case, our Spatial actor interop is triggered through ReplicateActors() so we want to call it regardless.
	Super::TickFlush(DeltaTime);
}

USpatialNetConnection * USpatialNetDriver::GetSpatialOSNetConnection() const
{
	if (ServerConnection)
	{
		return Cast<USpatialNetConnection>(ServerConnection);
	}
	else if (ClientConnections.Num() > 0)
	{
		return Cast<USpatialNetConnection>(ClientConnections[0]);
	}
	else
	{
		return nullptr;
	}
}

namespace
{
	TOptional<FString> ExtractWorkerIDFromAttribute(const FString& WorkerAttribute)
	{
		const FString WorkerIdAttr = TEXT("workerId:");
		int32 AttrOffset = WorkerAttribute.Find(WorkerIdAttr);

		if (AttrOffset < 0)
		{
			UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Error : Worker attribute does not contain workerId : %s"), *WorkerAttribute);
			return {};
		}

		return WorkerAttribute.RightChop(AttrOffset + WorkerIdAttr.Len());
	}
}

bool USpatialNetDriver::CreateSpatialNetConnection(const FURL& InUrl, const FUniqueNetIdRepl& UniqueId, const FName& OnlinePlatformName, USpatialNetConnection** OutConn)
{
	check(*OutConn == nullptr);
	*OutConn = NewObject<USpatialNetConnection>(GetTransientPackage(), NetConnectionClass);
	check(*OutConn != nullptr);

	USpatialNetConnection* SpatialConnection = *OutConn;

	// We create a "dummy" connection that corresponds to this player. This connection won't transmit any data.
	// We may not need to keep it in the future, but for now it looks like path of least resistance is to have one UPlayer (UConnection) per player.
	// We use an internal counter to give each client a unique IP address for Unreal's internal bookkeeping.
	ISocketSubsystem* SocketSubsystem = GetSocketSubsystem();
	TSharedRef<FInternetAddr> FromAddr = SocketSubsystem->CreateInternetAddr();
	FromAddr->SetIp(UniqueClientIpAddressCounter++);

	SpatialConnection->InitRemoteConnection(this, nullptr, InUrl, *FromAddr, USOCK_Open);
	Notify->NotifyAcceptedConnection(SpatialConnection);

	// TODO: This also currently sets all dormant actors to the active list (because the dormancy needs to be processed for the new connection)
	// This is unnecessary however, as we only have a single relevant connection in Spatial. Could be a performance win to not do this.
	AddClientConnection(SpatialConnection);

	// Set the unique net ID for this player. This and the code below is adapted from World.cpp:4499
	SpatialConnection->PlayerId = UniqueId;
	SpatialConnection->SetPlayerOnlinePlatformName(OnlinePlatformName);

	// Get the worker attribute.
	const TCHAR* WorkerAttributeOption = InUrl.GetOption(TEXT("workerAttribute"), nullptr);
	check(WorkerAttributeOption);
	SpatialConnection->ConnectionOwningWorkerId = FString(WorkerAttributeOption).Mid(1); // Trim off the = at the beginning.

	// Register workerId and its connection.
	if (TOptional<FString> WorkerId = ExtractWorkerIDFromAttribute(SpatialConnection->ConnectionOwningWorkerId))
	{
		UE_LOG(LogSpatialOSNetDriver, Verbose, TEXT("Worker %s 's NetConnection created."), *WorkerId.GetValue());

		WorkerConnections.Add(WorkerId.GetValue(), SpatialConnection);
	}

	// We will now ask GameMode/GameSession if it's ok for this user to join.
	// Note that in the initial implementation, we carry over no data about the user here (such as a unique player id, or the real IP)
	// In the future it would make sense to add metadata to the Spawn request and pass it here.
	// For example we can check whether a user is banned by checking against an OnlineSubsystem.

	// skip to the first option in the URL
	const FString UrlString = InUrl.ToString();
	const TCHAR* Tmp = *UrlString;
	for (; *Tmp && *Tmp != '?'; Tmp++);

	FString ErrorMsg;
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	check(GameMode);

	GameMode->PreLogin(Tmp, SpatialConnection->LowLevelGetRemoteAddress(), SpatialConnection->PlayerId, ErrorMsg);

	if (!ErrorMsg.IsEmpty())
	{
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("PreLogin failure: %s"), *ErrorMsg);
		// TODO: Destroy connection. UNR-584
		return false;
	}

	SpatialConnection->SetClientWorldPackageName(GetWorld()->GetCurrentLevel()->GetOutermost()->GetFName());

	FString RedirectURL;
	GameMode->GameWelcomePlayer(SpatialConnection, RedirectURL);

	return true;
}

void USpatialNetDriver::CleanUpClientConnection(USpatialNetConnection* ConnectionCleanedUp)
{
	if (!ConnectionCleanedUp->ConnectionOwningWorkerId.IsEmpty())
	{
		if (TOptional<FString> WorkerId = ExtractWorkerIDFromAttribute(*ConnectionCleanedUp->ConnectionOwningWorkerId))
		{
			WorkerConnections.Remove(WorkerId.GetValue());
		}
	}
}

TWeakObjectPtr<USpatialNetConnection> USpatialNetDriver::FindClientConnectionFromWorkerId(const FString& WorkerId)
{
	if (TWeakObjectPtr<USpatialNetConnection>* ClientConnectionPtr = WorkerConnections.Find(WorkerId))
	{
		return *ClientConnectionPtr;
	}

	return {};
}

void USpatialNetDriver::ProcessPendingDormancy()
{
	TSet<TWeakObjectPtr<USpatialActorChannel>> RemainingChannels;
	for (auto& PendingDormantChannel : PendingDormantChannels)
	{
		if (PendingDormantChannel.IsValid())
		{
			USpatialActorChannel* Channel = PendingDormantChannel.Get();
			if (Channel->Actor != nullptr)
			{
				if (Receiver->IsPendingOpsOnChannel(*Channel))
				{
					RemainingChannels.Emplace(PendingDormantChannel);
					continue;
				}
			}

			// This same logic is called from within UChannel::ReceivedSequencedBunch when a dormant cmd is received
			Channel->Dormant = 1;
			Channel->ConditionalCleanUp(false, EChannelCloseReason::Dormancy);
		}
	}
	PendingDormantChannels = MoveTemp(RemainingChannels);
}

void USpatialNetDriver::AcceptNewPlayer(const FURL& InUrl, const FUniqueNetIdRepl& UniqueId, const FName& OnlinePlatformName)
{
	USpatialNetConnection* SpatialConnection = nullptr;

	if (!CreateSpatialNetConnection(InUrl, UniqueId, OnlinePlatformName, &SpatialConnection))
	{
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Failed to create SpatialNetConnection!"));
		return;
	}

	FString ErrorMsg;
	SpatialConnection->PlayerController = GetWorld()->SpawnPlayActor(SpatialConnection, ROLE_AutonomousProxy, InUrl, SpatialConnection->PlayerId, ErrorMsg);

	if (SpatialConnection->PlayerController == nullptr)
	{
		// Failed to connect.
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Join failure: %s"), *ErrorMsg);
		SpatialConnection->FlushNet(true);
	}
}

// This function is called for server workers who received the PC over the wire
void USpatialNetDriver::PostSpawnPlayerController(APlayerController* PlayerController, const FString& ClientWorkerId)
{
	check(PlayerController != nullptr);
	checkf(!ClientWorkerId.IsEmpty(), TEXT("A player controller entity must have an owning client worker ID."));

	PlayerController->SetFlags(GetFlags() | RF_Transient);

	FString URLString = FURL().ToString();
	URLString += TEXT("?workerAttribute=") + ClientWorkerId;

	// We create a connection here so that any code that searches for owning connection, etc on the server
	// resolves ownership correctly
	USpatialNetConnection* OwnershipConnection = nullptr;
	if (!CreateSpatialNetConnection(FURL(nullptr, *URLString, TRAVEL_Absolute), FUniqueNetIdRepl(), FName(), &OwnershipConnection))
	{
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Failed to create SpatialNetConnection!"));
		return;
	}

	OwnershipConnection->PlayerController = PlayerController;

	PlayerController->NetPlayerIndex = 0;
	// We need to lie about our authority briefly here so that SetReplicates will succeed.
	// In the case this is being called after receiving an actor over the wire, our authority is intended to be ROLE_SimulatedProxy.
	// (It will get set immediately after this call in SpatialReceiver::CreateActor)
	ENetRole OriginalRole = PlayerController->Role;
	PlayerController->Role = ROLE_Authority;
	PlayerController->SetReplicates(true);
	PlayerController->Role = OriginalRole;
	PlayerController->SetPlayer(OwnershipConnection);
}

bool USpatialNetDriver::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
#if !UE_BUILD_SHIPPING
	if (FParse::Command(&Cmd, TEXT("DUMPCROSSSERVERRPC")))
	{
		return HandleNetDumpCrossServerRPCCommand(Cmd, Ar);
	}
#endif // !UE_BUILD_SHIPPING
	return UNetDriver::Exec(InWorld, Cmd, Ar);
}

// This function is literally a copy paste of UNetDriver::HandleNetDumpServerRPCCommand. Didn't want to refactor to avoid divergence from engine.
#if !UE_BUILD_SHIPPING
bool USpatialNetDriver::HandleNetDumpCrossServerRPCCommand(const TCHAR* Cmd, FOutputDevice& Ar)
{
#if WITH_SERVER_CODE
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		bool bHasNetFields = false;

		ensureMsgf(!ClassIt->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad), TEXT("UNetDriver::HandleNetDumpCrossServerRPCCommand: %s has flag RF_NeedPostLoad. NetFields and ClassReps will be incorrect!"), *GetFullNameSafe(*ClassIt));

		for (int32 i = 0; i < ClassIt->NetFields.Num(); i++)
		{
			UFunction * Function = Cast<UFunction>(ClassIt->NetFields[i]);

			if (Function != NULL && Function->FunctionFlags & FUNC_NetCrossServer)
			{
				bHasNetFields = true;
				break;
			}
		}

		if (!bHasNetFields)
		{
			continue;
		}

		Ar.Logf(TEXT("Class: %s"), *ClassIt->GetName());

		for (int32 i = 0; i < ClassIt->NetFields.Num(); i++)
		{
			UFunction * Function = Cast<UFunction>(ClassIt->NetFields[i]);

			if (Function != NULL && Function->FunctionFlags & FUNC_NetCrossServer)
			{
				const FClassNetCache * ClassCache = NetCache->GetClassNetCache(*ClassIt);

				const FFieldNetCache * FieldCache = ClassCache->GetFromField(Function);

				TArray< UProperty * > Parms;

				for (TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm; ++It)
				{
					Parms.Add(*It);
				}

				if (Parms.Num() == 0)
				{
					Ar.Logf(TEXT("    [0x%03x] %s();"), FieldCache->FieldNetIndex, *Function->GetName());
					continue;
				}

				FString ParmString;

				for (int32 j = 0; j < Parms.Num(); j++)
				{
					if (Cast<UStructProperty>(Parms[j]))
					{
						ParmString += Cast<UStructProperty>(Parms[j])->Struct->GetName();
					}
					else
					{
						ParmString += Parms[j]->GetClass()->GetName();
					}

					ParmString += TEXT(" ");

					ParmString += Parms[j]->GetName();

					if (j < Parms.Num() - 1)
					{
						ParmString += TEXT(", ");
					}
				}

				Ar.Logf(TEXT("    [0x%03x] %s( %s );"), FieldCache->FieldNetIndex, *Function->GetName(), *ParmString);
			}
		}
	}
#endif
	return true;
}
#endif // !UE_BUILD_SHIPPING

USpatialPendingNetGame::USpatialPendingNetGame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USpatialPendingNetGame::InitNetDriver()
{
	check(GIsClient);

	// This is a trimmed down version of UPendingNetGame::InitNetDriver(). We don't send any Unreal connection packets, just set up the net driver.
	if (!GDisallowNetworkTravel)
	{
		// Try to create network driver.
		if (GEngine->CreateNamedNetDriver(this, NAME_PendingNetDriver, NAME_GameNetDriver))
		{
			NetDriver = GEngine->FindNamedNetDriver(this, NAME_PendingNetDriver);
		}
		check(NetDriver);

		if (!NetDriver->InitConnect(this, URL, ConnectionError))
		{
			// error initializing the network stack...
			UE_LOG(LogNet, Warning, TEXT("error initializing the network stack"));
			GEngine->DestroyNamedNetDriver(this, NetDriver->NetDriverName);
			NetDriver = nullptr;

			// ConnectionError should be set by calling InitConnect...however, if we set NetDriver to NULL without setting a
			// value for ConnectionError, we'll trigger the assertion at the top of UPendingNetGame::Tick() so make sure it's set
			if (ConnectionError.Len() == 0)
			{
				ConnectionError = NSLOCTEXT("Engine", "NetworkInit", "Error initializing network layer.").ToString();
			}
		}
	}
	else
	{
		ConnectionError = NSLOCTEXT("Engine", "UsedCheatCommands", "Console commands were used which are disallowed in netplay.  You must restart the game to create a match.").ToString();
	}
}

void USpatialPendingNetGame::SendJoin()
{
	bSentJoinRequest = true;
}

void USpatialNetDriver::AddActorChannel(Worker_EntityId EntityId, USpatialActorChannel* Channel)
{
	EntityToActorChannel.Add(EntityId, Channel);
}

void USpatialNetDriver::RemoveActorChannel(Worker_EntityId EntityId, USpatialActorChannel& Channel)
{
	for (auto& ChannelRefs : Channel.ObjectReferenceMap)
	{
		Receiver->CleanupRepStateMap(ChannelRefs.Value);
	}
	Channel.ObjectReferenceMap.Empty();

	if (!EntityToActorChannel.Contains(EntityId))
	{
		UE_LOG(LogSpatialOSNetDriver, Verbose, TEXT("RemoveActorChannel: Failed to find entity/channel mapping for entity %lld."), EntityId);
		return;
	}

	EntityToActorChannel.FindAndRemoveChecked(EntityId);
}

TMap<Worker_EntityId_Key, USpatialActorChannel*>& USpatialNetDriver::GetEntityToActorChannelMap()
{
	return EntityToActorChannel;
}

USpatialActorChannel* USpatialNetDriver::GetOrCreateSpatialActorChannel(UObject* TargetObject)
{
	check(TargetObject);
	USpatialActorChannel* Channel = GetActorChannelByEntityId(PackageMap->GetEntityIdFromObject(TargetObject));
	if (Channel == nullptr)
	{
		AActor* TargetActor = Cast<AActor>(TargetObject);
		if (TargetActor == nullptr)
		{
			TargetActor = Cast<AActor>(TargetObject->GetOuter());
		}
		check(TargetActor);

		if (USpatialActorChannel* ActorChannel = GetActorChannelByEntityId(PackageMap->GetEntityIdFromObject(TargetActor)))
		{
			// This can happen if schema database is out of date and had no entry for a static subobject.
			UE_LOG(LogSpatialOSNetDriver, Warning, TEXT("GetOrCreateSpatialActorChannel: No channel for target object but channel already present for actor. Target object: %s, actor: %s"), *TargetObject->GetPathName(), *TargetActor->GetPathName());
			return ActorChannel;
		}
		Channel = CreateSpatialActorChannel(TargetActor);
	}
#if !UE_BUILD_SHIPPING
	if (Channel != nullptr && Channel->Actor == nullptr)
	{
		// This shouldn't occur, but can often crop up whilst we are refactoring entity/actor/channel lifecycles.
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("Failed to correctly initialize SpatialActorChannel for [%s]"), *TargetObject->GetName());
	}
#endif // !UE_BUILD_SHIPPING
	return Channel;
}

USpatialActorChannel* USpatialNetDriver::GetActorChannelByEntityId(Worker_EntityId EntityId) const
{
	return EntityToActorChannel.FindRef(EntityId);
}

void USpatialNetDriver::RefreshActorDormancy(AActor* Actor, bool bMakeDormant)
{
	check(IsServer());
	check(Actor);

	const Worker_EntityId EntityId = PackageMap->GetEntityIdFromObject(Actor);
	if (EntityId == SpatialConstants::INVALID_ENTITY_ID)
	{
		UE_LOG(LogSpatialOSNetDriver, Verbose, TEXT("Unable to flush dormancy on actor (%s) without entity id"), *Actor->GetName());
		return;
	}

	const bool bHasAuthority = StaticComponentView->HasAuthority(EntityId, SpatialConstants::DORMANT_COMPONENT_ID);
	if (bHasAuthority == false)
	{
		UE_LOG(LogSpatialOSNetDriver, Warning, TEXT("Unable to flush dormancy on actor (%s) without authority"), *Actor->GetName());
		return;
	}

	const bool bDormancyComponentExists = StaticComponentView->HasComponent(EntityId, SpatialConstants::DORMANT_COMPONENT_ID);

	// If the Actor wants to go dormant, ensure the Dormant component is attached
	if (bMakeDormant)
	{
		if (!bDormancyComponentExists)
		{
			Worker_AddComponentOp AddComponentOp{};
			AddComponentOp.entity_id = EntityId;
			AddComponentOp.data = ComponentFactory::CreateEmptyComponentData(SpatialConstants::DORMANT_COMPONENT_ID);
			Sender->SendAddComponents(AddComponentOp.entity_id, { AddComponentOp.data });
			StaticComponentView->OnAddComponent(AddComponentOp);
		}
	}
	else
	{
		if (bDormancyComponentExists)
		{
			Worker_RemoveComponentOp RemoveComponentOp{};
			RemoveComponentOp.entity_id = EntityId;
			RemoveComponentOp.component_id = SpatialConstants::DORMANT_COMPONENT_ID;
			Sender->SendRemoveComponents(EntityId, { SpatialConstants::DORMANT_COMPONENT_ID });
			StaticComponentView->OnRemoveComponent(RemoveComponentOp);
		}
	}
}

void USpatialNetDriver::AddPendingDormantChannel(USpatialActorChannel* Channel)
{
	PendingDormantChannels.Emplace(Channel);
}

void USpatialNetDriver::RemovePendingDormantChannel(USpatialActorChannel* Channel)
{
	PendingDormantChannels.Remove(Channel);
}

void USpatialNetDriver::RegisterDormantEntityId(Worker_EntityId EntityId)
{
	// Register dormant entities when their actor channel has been closed, but their entity is still alive.
	// This allows us to clean them up when shutting down. Might be nice to not rely on ActorChannels to
	// cleanup in future, but inspect the StaticView and delete all entities that this worker is authoritative over.
	DormantEntities.Emplace(EntityId);
}

void USpatialNetDriver::UnregisterDormantEntityId(Worker_EntityId EntityId)
{
	DormantEntities.Remove(EntityId);
}

bool USpatialNetDriver::IsDormantEntity(Worker_EntityId EntityId) const
{
	return (DormantEntities.Find(EntityId) != nullptr);
}

USpatialActorChannel* USpatialNetDriver::CreateSpatialActorChannel(AActor* Actor)
{
	// This should only be called from GetOrCreateSpatialActorChannel, otherwise we could end up clobbering an existing channel.

	check(Actor != nullptr);
	check(PackageMap != nullptr);

	Worker_EntityId EntityId = PackageMap->GetEntityIdFromObject(Actor);

	check(GetActorChannelByEntityId(EntityId) == nullptr);

	USpatialNetConnection* NetConnection = GetSpatialOSNetConnection();
	check(NetConnection != nullptr);

	USpatialActorChannel* Channel = nullptr;

	if (Actor->GetClass()->HasAnySpatialClassFlags(SPATIALCLASS_Singleton) && !SpatialGDK::Singleton::ShouldHaveLocalWorkerAuthority(Actor, LoadBalanceStrategy, StaticComponentView))
	{
		return Channel;
	}

	Channel = static_cast<USpatialActorChannel*>(NetConnection->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally));
	if (Channel == nullptr)
	{
		UE_LOG(LogSpatialOSNetDriver, Warning, TEXT("Failed to create a channel for actor %s."), *GetNameSafe(Actor));
	}
	else
	{
#if ENGINE_MINOR_VERSION <= 22
		Channel->SetChannelActor(Actor);
#else
		Channel->SetChannelActor(Actor, ESetChannelActorFlags::None);
#endif
	}

	if (Channel != nullptr)
	{
		Channel->RefreshAuthority();
	}

	return Channel;
}

void USpatialNetDriver::WipeWorld(const PostWorldWipeDelegate& LoadSnapshotAfterWorldWipe)
{
	SnapshotManager->WorldWipe(LoadSnapshotAfterWorldWipe);
}

void USpatialNetDriver::DelayedSendDeleteEntityRequest(Worker_EntityId EntityId, float Delay)
{
	FTimerHandle RetryTimer;
	TimerManager.SetTimer(RetryTimer, [this, EntityId]()
	{
		Sender->RetireEntity(EntityId);
	}, Delay, false);
}

void USpatialNetDriver::HandleStartupOpQueueing(const TArray<Worker_OpList*>& InOpLists)
{
	if (InOpLists.Num() == 0)
	{
		return;
	}

	QueuedStartupOpLists.Append(InOpLists);
	if (IsServer())
	{
		bIsReadyToStart = FindAndDispatchStartupOpsServer(InOpLists);

		if (bIsReadyToStart)
		{
			if (GetDefault<USpatialGDKSettings>()->bEnableUnrealLoadBalancer)
			{
				// We know at this point that we have all the information to set the worker's interest query.
				Sender->UpdateServerWorkerEntityInterestAndPosition();
			}

			// We've found and dispatched all ops we need for startup,
			// trigger BeginPlay() on the GSM and process the queued ops.
			// Note that FindAndDispatchStartupOps() will have notified the Dispatcher
			// to skip the startup ops that we've processed already.
			GlobalStateManager->TriggerBeginPlay();
		}
	}
	else
	{
		bIsReadyToStart = FindAndDispatchStartupOpsClient(InOpLists);
	}

	if (!bIsReadyToStart)
	{
		return;
	}

	for (Worker_OpList* OpList : QueuedStartupOpLists)
	{
		Dispatcher->ProcessOps(OpList);
		Worker_OpList_Destroy(OpList);
	}

	// Sanity check that the dispatcher encountered, skipped, and removed
	// all Ops we asked it to skip
	check(Dispatcher->GetNumOpsToSkip() == 0);

	QueuedStartupOpLists.Empty();
}

bool USpatialNetDriver::FindAndDispatchStartupOpsServer(const TArray<Worker_OpList*>& InOpLists)
{
	TArray<Worker_Op*> FoundOps;

	Worker_Op* EntityQueryResponseOp = nullptr;
	FindFirstOpOfType(InOpLists, WORKER_OP_TYPE_ENTITY_QUERY_RESPONSE, &EntityQueryResponseOp);

	if (EntityQueryResponseOp != nullptr)
	{
		FoundOps.Add(EntityQueryResponseOp);
	}

	// To correctly initialize the ServerWorkerEntity on each server during op queueing, we need to catch several ops here.
	// Note that this will break if any other CreateEntity requests are issued during the startup flow.
	{
		Worker_Op* CreateEntityResponseOp = nullptr;
		FindFirstOpOfType(InOpLists, WORKER_OP_TYPE_CREATE_ENTITY_RESPONSE, &CreateEntityResponseOp);

		Worker_Op* AddComponentOp = nullptr;
		FindFirstOpOfTypeForComponent(InOpLists, WORKER_OP_TYPE_ADD_COMPONENT, SpatialConstants::SERVER_WORKER_COMPONENT_ID, &AddComponentOp);

		Worker_Op* AuthorityChangedOp = nullptr;
		FindFirstOpOfTypeForComponent(InOpLists, WORKER_OP_TYPE_AUTHORITY_CHANGE, SpatialConstants::SERVER_WORKER_COMPONENT_ID, &AuthorityChangedOp);

		if (CreateEntityResponseOp != nullptr)
		{
			FoundOps.Add(CreateEntityResponseOp);
		}

		if (AddComponentOp != nullptr)
		{
			FoundOps.Add(AddComponentOp);
		}

		if (AuthorityChangedOp != nullptr)
		{
			FoundOps.Add(AuthorityChangedOp);
		}
	}

	// Search for entity id reservation response and process it.  The entity id reservation
	// can fail to reserve entity ids.  In that case, the EntityPool will not be marked ready,
	// a new query will be sent, and we will process the new response here when it arrives.
	if (!PackageMap->IsEntityPoolReady())
	{
		Worker_Op* EntityIdReservationResponseOp = nullptr;
		FindFirstOpOfType(InOpLists, WORKER_OP_TYPE_RESERVE_ENTITY_IDS_RESPONSE, &EntityIdReservationResponseOp);

		if (EntityIdReservationResponseOp != nullptr)
		{
			FoundOps.Add(EntityIdReservationResponseOp);
		}
	}

	// Search for StartupActorManager ops we need and process them
	if (!GlobalStateManager->IsReady())
	{
		Worker_Op* AddComponentOp = nullptr;
		FindFirstOpOfTypeForComponent(InOpLists, WORKER_OP_TYPE_ADD_COMPONENT, SpatialConstants::STARTUP_ACTOR_MANAGER_COMPONENT_ID, &AddComponentOp);

		Worker_Op* AuthorityChangedOp = nullptr;
		FindFirstOpOfTypeForComponent(InOpLists, WORKER_OP_TYPE_AUTHORITY_CHANGE, SpatialConstants::STARTUP_ACTOR_MANAGER_COMPONENT_ID, &AuthorityChangedOp);

		Worker_Op* ComponentUpdateOp = nullptr;
		FindFirstOpOfTypeForComponent(InOpLists, WORKER_OP_TYPE_COMPONENT_UPDATE, SpatialConstants::STARTUP_ACTOR_MANAGER_COMPONENT_ID, &ComponentUpdateOp);

		if (AddComponentOp != nullptr)
		{
			FoundOps.Add(AddComponentOp);
		}

		if (AuthorityChangedOp != nullptr)
		{
			FoundOps.Add(AuthorityChangedOp);
		}

		if (ComponentUpdateOp != nullptr)
		{
			FoundOps.Add(ComponentUpdateOp);
		}
	}

	if (VirtualWorkerTranslator.IsValid() && !VirtualWorkerTranslator->IsReady())
	{
		Worker_Op* AddComponentOp = nullptr;
		FindFirstOpOfTypeForComponent(InOpLists, WORKER_OP_TYPE_ADD_COMPONENT, SpatialConstants::VIRTUAL_WORKER_TRANSLATION_COMPONENT_ID, &AddComponentOp);

		Worker_Op* AuthorityChangedOp = nullptr;
		FindFirstOpOfTypeForComponent(InOpLists, WORKER_OP_TYPE_AUTHORITY_CHANGE, SpatialConstants::VIRTUAL_WORKER_TRANSLATION_COMPONENT_ID, &AuthorityChangedOp);

		Worker_Op* ComponentUpdateOp = nullptr;
		FindFirstOpOfTypeForComponent(InOpLists, WORKER_OP_TYPE_COMPONENT_UPDATE, SpatialConstants::VIRTUAL_WORKER_TRANSLATION_COMPONENT_ID, &ComponentUpdateOp);

		if (AddComponentOp != nullptr)
		{
			UE_LOG(LogSpatialOSNetDriver, Log, TEXT("Processing Translation component add to bootstrap SpatialVirtualWorkerTranslator."));
			FoundOps.Add(AddComponentOp);
		}

		if (AuthorityChangedOp != nullptr)
		{
			UE_LOG(LogSpatialOSNetDriver, Log, TEXT("Processing Translation component authority change to bootstrap SpatialVirtualWorkerTranslator."));
			FoundOps.Add(AuthorityChangedOp);
		}

		if (ComponentUpdateOp != nullptr)
		{
			UE_LOG(LogSpatialOSNetDriver, Log, TEXT("Processing Translation component update to bootstrap SpatialVirtualWorkerTranslator."));
			FoundOps.Add(ComponentUpdateOp);
		}
	}

	SelectiveProcessOps(FoundOps);

	if (PackageMap->IsEntityPoolReady() &&
		GlobalStateManager->IsReady() &&
		(!VirtualWorkerTranslator.IsValid() || VirtualWorkerTranslator->IsReady()))
	{
		// Return whether or not we are ready to start
		UE_LOG(LogSpatialOSNetDriver, Log, TEXT("Ready to begin processing."));
		return true;
	}

	UE_LOG(LogSpatialOSNetDriver, Log, TEXT("Not yet ready to begin processing, still processing startup ops."));
	return false;
}

bool USpatialNetDriver::FindAndDispatchStartupOpsClient(const TArray<Worker_OpList*>& InOpLists)
{
	if (bMapLoaded)
	{
		return true;
	}
	else
	{
		// Search for the entity query response for the GlobalStateManager
		Worker_Op* Op = nullptr;
		FindFirstOpOfType(InOpLists, WORKER_OP_TYPE_ENTITY_QUERY_RESPONSE, &Op);

		TArray<Worker_Op*> FoundOps;
		if (Op != nullptr)
		{
			FoundOps.Add(Op);
		}

		SelectiveProcessOps(FoundOps);
		return false;
	}
}

void USpatialNetDriver::SelectiveProcessOps(TArray<Worker_Op*> FoundOps)
{
	// For each Op we've found, make a Worker_OpList that just contains that Op,
	// and pass it to the dispatcher for processing. This allows us to avoid copying
	// the Ops around and dealing with memory that is / should be managed by the Worker SDK.
	// The Op remains owned by the original OpList.  Finally, notify the dispatcher to skip
	// these Ops when they are encountered later when we process the queued ops.
	for (Worker_Op* Op : FoundOps)
	{
		Worker_OpList SingleOpList;
		SingleOpList.op_count = 1;
		SingleOpList.ops = Op;

		Dispatcher->ProcessOps(&SingleOpList);
		Dispatcher->MarkOpToSkip(Op);
	}
}

// This should only be called once on each client, in the SpatialMetricsDisplay constructor after the class is replicated to each client.
// This is enforced by the fact that the class is a Singleton spawned on servers by the SpatialNetDriver.
void USpatialNetDriver::SetSpatialMetricsDisplay(ASpatialMetricsDisplay* InSpatialMetricsDisplay)
{
	check(SpatialMetricsDisplay == nullptr);
	SpatialMetricsDisplay = InSpatialMetricsDisplay;
}

#if WITH_EDITOR
void USpatialNetDriver::TrackTombstone(const Worker_EntityId EntityId)
{
	TombstonedEntities.Add(EntityId);
}
#endif

// This should only be called once on each client, in the SpatialDebugger constructor after the class is replicated to each client.
// This is enforced by the fact that the class is a Singleton spawned on servers by the SpatialNetDriver.
void USpatialNetDriver::SetSpatialDebugger(ASpatialDebugger* InSpatialDebugger)
{
	check(!IsServer());
	if (SpatialDebugger != nullptr)
	{
		UE_LOG(LogSpatialOSNetDriver, Error, TEXT("SpatialDebugger should only be set once on each client!"));
		return;
	}

	SpatialDebugger = InSpatialDebugger;
}

FUnrealObjectRef USpatialNetDriver::GetCurrentPlayerControllerRef()
{
	if (USpatialNetConnection* NetConnection = GetSpatialOSNetConnection())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(NetConnection->OwningActor))
		{
			if (PackageMap)
			{
				return PackageMap->GetUnrealObjectRefFromObject(PlayerController);
			}
		}
	}
	return FUnrealObjectRef::NULL_OBJECT_REF;
}

// This is only called if this worker has been selected by SpatialOS to be authoritative
// for the TranslationManager, otherwise the manager will never be instantiated.
void USpatialNetDriver::InitializeVirtualWorkerTranslationManager()
{
	VirtualWorkerTranslationManager = MakeUnique<SpatialVirtualWorkerTranslationManager>(Receiver, Connection, VirtualWorkerTranslator.Get());
	VirtualWorkerTranslationManager->AddVirtualWorkerIds(LoadBalanceStrategy->GetVirtualWorkerIds());
}
