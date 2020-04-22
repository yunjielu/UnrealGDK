# Changelog
All notable changes to the SpatialOS Game Development Kit for Unreal will be documented in this file.

The format of this Changelog is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased-`x.y.z`] - 2020-xx-xx

### New Known Issues:
- After upgrading to Unreal Engine `4.24.3` using `git pull`, you may be left in a state where several `.isph` and `.ispc` files are missing. This state produces [compile errors](https://forums.unrealengine.com/unreal-engine/announcements-and-releases/1695917-unreal-engine-4-24-released?p=1715142#post1715142) when you build the engine. You can fix this by running `git restore .` in the root of your `UnrealEngine` repository.

### Breaking Changes:
- Simulated Player worker configurations now require a dev auth token and deployment flag instead of a login token and player identity token. See the Example Project for an example of how to set this up.
- Singletons have been removed as a class specifier and you will need to remove your usages of it. Replicating the behavior of former singletons is achievable through ensuring your Actor is spawned once by a single server-side worker in your deployment.

### Features:
- Unreal Engine `4.24.3` is now supported. You can find the `4.24.3` version of our engine fork [here](https://github.com/improbableio/UnrealEngine/tree/4.24-SpatialOSUnrealGDK-preview).
- Added a new variable `QueuedOutgoingRPCWaitTime`. Outgoing RPCs will now be dropped if: more than `QueuedOutgoingRPCWaitTime` time has passed; the worker is never expected to become authoritative in zoning/offloading scenario; the Actor is being destroyed.
- In local deployments of the Example Project you can now launch Simulated Players in one click. Running `LaunchSimPlayerClient.bat` will launch a single Simulated Player client. Running `Launch10SimPlayerClients.bat` will launch 10.
- Added an AuthorityIntent component to be used in the future for UnrealGDK code to control loadbalancing.
- Added support for the UE4 Network Profile to measure relative size of RPC and Actor replication data.
- Added a VirtualWorkerTranslation component to be used in future UnrealGDK loadbalancing.
- Added partial framework for use in future UnrealGDK controlled loadbalancing.
- Add SpatialToggleMetricsDisplay console command.  bEnableMetricsDisplay must be enabled in order for the display to be available.  You must then must call SpatialToggleMetricsDisplay on each client that wants to view the metrics display.
- Enabled compression in modular-udp networking stack
- Switched off default rpc-packing. This can still be re-enabled in SpatialGDKSettings.ini
- Starting a local deployment now checks if the required runtime port is blocked and allows the user to kill it
- A configurable actor component 'SpatialPingComponent' is now available for player controllers to measure round-trip ping to their current authoritative server worker. The latest ping value can be accessed raw through the component via 'GetPing()' or otherwise via the rolling average stored in 'PlayerState'.
- The `GenerateSchema`, `GenerateSchemaAndSnapshots`, and `CookAndGenerateSchema` commandlets can be invoked with the `-AdditionalSchemaCompilerArguments="..."` command line switch to output additional compiled schema formats. If no such switch is provided, only the schema descriptor will be produced. This switch's value should be a subset of the arguments that can be passed to the schema compiler directly (e.g., `--bundle_out="path/to/bundle.sb"`). A full list of possibles values is available via the [schema compiler documentation](https://docs.improbable.io/reference/14.2/shared/schema/introduction#schema-compiler-cli-reference)
- Added the AllowUnresolvedParameters function flag that disables warnings for processing RPCs with unresolved parameters. This flag can be enabled through Blueprints or by adding a tag to the `UFUNCTION` macro.
- A warning is shown if a cloud deployment is launched with the `manual_worker_connection_only` flag set to true
- Server travel supported for single server game worlds. Does not currently support zoning or off-loading.
- Enabled the SpatialOS toolbar for MacOS.
- Improved workflow around schema generation issues and launching local builds. A warning will now show if attempting to run a local deployment after a schema error.
- DeploymentLauncher can parse a .pb.json launch configuration.
- DeploymentLauncher can launch a Simulated Player deployment independently from the target deployment.
Usage: `DeploymentLauncher createsim <project-name> <assembly-name> <target-deployment-name> <sim-deployment-name> <sim-deployment-json> <sim-deployment-region> <num-sim-players> <auto-connect>`
- Added `HeartbeatTimeoutWithEditorSeconds` and use it if WITH_EDITOR is defined to prevent workers disconnecting when debugging while running in editor.
- Added the `bAsyncLoadNewClassesOnEntityCheckout` setting to SpatialGDKSettings that allows loading new classes asynchronously when checking out entities. This is off by default.
- Added `IndexYFromSchema` functions for the `Coordinates`, `WorkerRequirementSet`, `FRotator`, and `FVector` classes. Remapped the `GetYFromSchema` functions for the same classes to invoke `IndexYFromSchema` internally, in line with other implementations of the pattern.
- The logic responsible for taking an Actor and generating the array of Components that represents it as an Entity in SpatialOS has been extracted into `EntityFactory`.
- Clients will now validate schema against the server and log a warning if they do not match.
- Entries in the SchemaDatabase are now sorted to improve efficiancy when browsing the asset in the editor. (DW-Sebastien)
- Load Balancing Strategies and Locking Strategies can be set per-level using SpatialWorldSettings.
- Batch Spatial Position Updates now defaults to false.
- Added `bEnableNetCullDistanceInterest` (defaulted true) to enable client interest to be exposed through component tagging. This functionality has closer parity to native unreal client interest.
- Added `bEnableNetCullDistanceFrequency` (defaulted false) to enable client interest queries to use frequency. This functionality is configured using `InterestRangeFrequencyPairs` and `FullFrequencyNetCullDistanceRatio`.
- Added support for Android.
- Introduced feature flag `bEnableResultTypes` (defaulted true). This configures Interest queries to only include the set of components required to run. Should give bandwidth savings depending on your game.
- Dynamic interest overrides are disabled if the `bEnableResultTypes` flag is set to true.
- Moved Dev Auth settings from runtime settings to editor settings.
- Added the option to use the development authentication flow using the command line.
- Added a button to generate the Development Authentication Token inside the Unreal Editor. To use it, navigate to **Edit** > **Project Setting** > **SpatialOS GDK for Unreal** > **Editor Settings** > **Cloud Connection**.
- Added a new settings section allowing you to configure the launch arguments when running a a client on a mobile device. To use it, navigate to **Edit** > **Project Setting** > **SpatialOS GDK for Unreal** > **Editor Settings** > **Mobile**.
- Added settings to choose which runtime version to launch with local and cloud deployment launch command.
- With the `--OverrideResultTypes` flag flipped, servers will no longer check out server RPC components on actors they do not own. This should give a bandwidth saving to server workers in offloaded and zoned games.
- The `InstallGDK` scripts now `git clone` the correct version of the `UnrealGDK` and `UnrealGDKExampleProject` for the `UnrealEngine` branch you have checked out. They read `UnrealGDKVersion.txt` & `UnrealGDKExampleProjectVersion.txt` to determine what the correct branches are.
- Enabling the Unreal GDK load balancer now creates a single query per server worker, depending on the defined load balancing strategy.
- The `bEnableServerQBI` property has been removed, and the flag `--OverrideServerInterest` has been removed.
- SpatialDebugger worker regions are now cuboids rather than planes, and can have their WorkerRegionVerticalScale adjusted via a setting in the SpatialDebugger.
- Added custom warning timeouts per RPC failure condition.
- SpatialPingComponent can now also report average ping measurements over a specified number of recent pings. You can specify the number of measurements recorded in `PingMeasurementsWindowSize` and get the measurement data by calling `GetAverageData`. There is also a delegate `OnRecordPing` that will be broadcast whenever a new ping measurement is recorded.
- The Spatial Output Log window now displays deployment startup errors.
- Added `bEnableClientQueriesOnServer` (defaulted false) which makes the same queries on the server as on clients if the unreal load balancer is enabled. Enable this to avoid clients seeing entities the server does not if the server's interest query has not been configured correctly.
- Added log warning when AddPendingRPC fails due to ControllerChannelNotListening.
- When running with Offloading enabled, Actors will have local authority (ROLE_Authority) on servers for longer periods of time to allow more native Unreal functionality to work without problems.
- When running with Offloading enabled, and trying to spawn Actors on a server which will not be the Actor Group owner for them, an error is logged and the Actor is deleted.
- The GDK now uses SpatialOS runtime version 14.5.1 by default.
- Config setting `bPreventAutoConnectWithLocator` has been renamed to `bPreventClientCloudDeploymentAutoConnect`. It has been moved to GDK Setting. If using this feature please update enable the setting in GDK Settings.
- USpatialMetrics::WorkerMetricsRecieved was made static.
- Added the ability to connect to a local deployment when launching on a device by checking "Connect to a local deployment" and specifying the local IP of your computer in the Launch dropdown.
- The Spatial GDK now default enables RPC Ring Buffers, and the legacy RPC mode will be removed in a subsequent release.
- The `bPackRPCs` property has been removed, and the flag `--OverrideRPCPacking` has been removed.
- Added `OnClientOwnershipGained` and `OnClientOwnershipLost` events on Actors and ActorComponents. These events trigger when an Actor is added to or removed from the ownership hierarchy of a client's PlayerController.
- You can now generate valid schema for classes that start with a leading digit. The generated schema class will be prefixed with `ZZ` internally.
- Handover properties will be automatically replicated when required for load balancing. `bEnableHandover` is off by default.
- Added `OnPlayerSpawnFailed` delegate to `SpatialGameInstance`. This is helpful if you have established a successful connection but the server worker crashed. `OnConnected` and `OnConnectionFailed` are now also blueprint-assignable.

## Bug fixes:
- Fixed a bug that caused queued RPCs to spam logs when an entity is deleted.
- Take into account OverrideSpatialNetworking command line argument as early as possible (LocalDeploymentManager used to query bSpatialNetworking before the command line was parsed).
- Servers maintain interest in AlwaysRelevant Actors.
- GetActorSpatialPosition now returns last spectator sync location while player is spectating.
- The default cloud launch configuration is now empty.
- Fixed an crash caused by attempting to read schema from an unloaded class.
- Unresolved object references in replicated arrays of structs should now be properly handled and eventually resolved.
- Fix tombstone-related assert that could fire and bring down the editor.
- Actors placed in the level with bNetLoadOnClient=false that go out of view will now be reloaded if they come back into view.
- Fix crash in SpatialDebugger caused by dereference of invalid weak pointer.
- Fixed connection error when using spatial cloud connect external.
- The command line argument "receptionistHost <URL>" will now not override connections to "127.0.0.1".
- The receptionist will now be used for appropriate URLs after connecting to a locator URL.
- You can now access the worker flags via `USpatialStatics::GetWorkerFlag` instead of `USpatialWorkerFlags::GetWorkerFlag`.
- Fix crash in SpatialDebugger when GDK-space load balancing is disabled.
- Fixed issue where schema database failed to load previous saved state when working in editor.
- Attempting to launch a cloud deployment will now run the spatial auth process as it is required. Previously the deployment would simply fail.
- Minor spelling fix to connection log message.
- Added %s token to debug strings in GlobalStateManager to display actor class name in log.
- The server no longer crashes, when received RPCs are processed recursively.
- Fix to serialize SoftObjectPointers when they are not resolved yet.
- Fix to handle replicated properties depending on asynchronously loaded packages.
- Fix to component interest constraints constructed from schema.
- Track properties containing references to replicated actors, in order to resolve them again if the actor they reference moves out and back into relevance.
- Fix problem where PIE sessions sometimes fail to start due to missing schema for SpatialDebugger blueprint.
- Fixed an issue where newly created subobjects would have empty state when RepNotify was called for a property pointing to that subobject.
- Fixed an issue where deleted, initially dormant startup actors would still be present on other workers.
- Force activation of RPC ring buffer when load balancing is enabled, to allow RPC handover when authority changes
- Fixed a race where a client leaving the deployment could leave its actor behind on the server, to be cleaned up after a long timeout.
- Fixed crash caused by state persisting across a transition from one deployment to another in SpatialGameInstance.
- Fixed crash when starting + stopping PIE multiple times.
- Fixed crash when shadow data was uninitialized when resolving unresolved objects.
- Fixed sending component RPCs on a recently created actor.
- Fix problem where load balanced cloud deploys could fail to start while under heavy load.

### External contributors:
@DW-Sebastien

## [`0.8.1-preview`] - 2020-03-17

### Internal:
### Adapted from 0.6.5
- **SpatialOS GDK for Unreal** > **Editor Settings** > **Region Settings** has been moved to **SpatialOS GDK for Unreal** > **Runtime Settings** > **Region Settings**.
- You can now choose which SpatialOS service region you want to use by adjusting the **Region where services are located** setting. You must use the service region that you're geographically located in.
- Deployments can now be launched in China, when the **Region where services are located** is set to `CN`.

### Features:
- Updated the version of the local API service used by the UnrealGDK.
- The Spatial output log will now be open by default.
- The GDK now uses SpatialOS 14.5.0.

### Bug fixes:
- Replicated references to newly created dynamic subobjects will now be resolved correctly.
- Fixed a bug that caused the local API service to memory leak.
- Cloud deployment flow will now correctly report errors when a deployment fails to launch due to a missing assembly.
- Errors are now correctly reported when you try to launch a cloud deployment without an assembly.
- The Start deployment button will no longer become greyed out when a `spatial auth login` process times out.

## [`0.8.0-preview`] - 2019-12-17

### Breaking Changes:
- This is the last GDK version to support Unreal Engine 4.22. You will need to upgrade your project to use Unreal Engine 4.23 (`4.23-SpatialOSUnrealGDK-preview`) in order to continue receiving GDK releases and support.
- When upgrading to Unreal Engine 4.23 you must:
1. `git checkout 4.23-SpatialOSUnrealGDK-preview`
1. `git pull`
1. Download and install the `-v15 clang-8.0.1-based` toolchain from this [Unreal Engine Documentation page](https://docs.unrealengine.com/en-US/Platforms/Linux/GettingStarted/index.html).
1. Navigate to the root of GDK repo and run `Setup.bat`.
1. Run `Setup.bat`, which is located in the root directory of the `UnrealEngine` repository.
1. Run `GenerateProjectFiles.bat`, which is in the same root directory.

For more information, check the [Keep your GDK up to date](https://documentation.improbable.io/gdk-for-unreal/docs/keep-your-gdk-up-to-date) SpatialOS documentation.

### Features:
- You can now call `SpatialToggleMetricsDisplay` from the console in your Unreal clients in order to view metrics. `bEnableMetricsDisplay` must be enabled on clients where you want to use this feature.
- The modular-udp networking stack now uses compression by default.
- Reduced network latency by switching off default rpc-packing. If you need this on by default, you can re-enable it by editing `SpatialGDKSettings.ini`
- When you start a local deployment, the GDK now checks the port required by the runtime and, if it's in use, prompts you to kill that process.
- You can now measure round-trip ping from a player controller to the server-worker that's currently authoritative over it using the configurable actor component 'SpatialPingComponent'. The latest ping value can be accessed through the component via 'GetPing()' or via the rolling average stored in 'PlayerState'.
- You can disable the warnings that trigger when RPCs are processed with unresolved parameters using the `AllowUnresolvedParameters` function flag. This flag can be enabled through Blueprints or by adding a tag to the `UFUNCTION` macro.
- Improved logging around entity creation.
- Unreal Engine `4.23.1` is now supported. You can find the `4.23.1` version of our engine fork [here](https://github.com/improbableio/UnrealEngine/tree/4.23-SpatialOSUnrealGDK-preview).
- In Example Project, the default session duration has increased from 5 minutes to 120 minutes so you don't have to re-deploy while playtesting.
- In Example Project, the default lobby timer has decreased from 15 seconds to 3 seconds so you don't have to wait for your playtest to start.
- Added in-editor support for exposing a local runtime at a particular IP address. This offers the same functionality as the `--runtime_ip` option in the SpatialOS CLI.
- Spatial networking is now always enabled in built assemblies.

### Bug fixes:
- Fixed a bug that could cause name collisions in schema generated for sublevels.
- Downgraded name collisions during schema generation from Warning to Display.
- Replicating a static subobject after it has been deleted on a client no longer results in client attaching a new dynamic subobject.
- Fixed a bug that caused entity pool reservations to cease after a request times out.
- Running `BuildWorker.bat` for `SimulatedPlayer` no longer fails if the project path has a space in it.
- Fixed a crash when starting PIE with out-of-date schema.
- Fixed an issue where launching a cloud deployment with an invalid assembly name or deployment name wouldn't show a helpful error message.

### Internal:
Features listed in the internal section are not ready to use but, in the spirit of open development, we detail every change we make to the GDK.
- We've added a partial loadbalancing framework. When this is completed in a future release, you will be able to control loadbalancing using server-workers.

## [`0.7.1-preview`] - 2019-12-06

### Adapted from 0.6.3
### Bug fixes:
- The C Worker SDK now communicates on port 443 instead of 444. This change is intended to protect your cloud deployments from DDoS attacks.

### Internal:
Features listed in the internal section are not ready to use but, in the spirit of open development, we detail every change we make to the GDK.
- The GDK is now compatible with the `CN` launch region. When Improbable's online services are fully working in China, they will work with this version of the GDK. You will be able to create SpatialOS Deployments in China by specifying the `CN` region in the Deployment Launcher.
- `Setup.bat` and `Setup.sh` both accept the `--china` flag, which will be required in order to run SpatialOS CLI commands in the `CN` region.
- **SpatialOS GDK for Unreal** > **Editor Settings** now contains a **Region Settings** section. You will be required to set **Region where services are located** to `CN` in order to create SpatialOS Deployments in China.

## [`0.7.0-preview`] - 2019-10-11

### New Known Issues:
- MSVC v14.23 removes `typeinfo.h` and replaces it with `typeinfo`. This change causes errors when building the Unreal Engine. This issue **only affects Visual Studio 2019** users. You can work around the issue by:
1. Open Visual Studio Installer.
1. Select "Modify" on your Visual Studio 2019 installation.
1. In the Installation details section uncheck all workloads and components until only **Visual Studio code editor** remains.
1. Select the following items in the Workloads tab:
* **Universal Windows Platform development**
* **.NET desktop development**
* You must also select the **.NET Framework 4.6.2 development tools** component in the Installation details section.
* **Desktop development with C++**
* You must then deselect **MSVC v142 - VS 2019 C++ x64/x86 build tools (v14.23)**, which was added as part of the **Desktop development with C++** Workload. You will be notified that: "If you continue, we'll remove the componenet and any items liseted above that depend on it." Select remove to confirm your decision.
* Lastly, add **MSVC v142 - VS 2019 C++ x64/x86 build tools (v14.22)** from the Individual components tab.
5. Select "Modify" to confirm your changes.

### Breaking Changes:
- If your project uses replicated subobjects that do not inherit from ActorComponent or GameplayAbility, you now need to enable generating schema for them using `SpatialType` UCLASS specifier, or by checking the Spatial Type checkbox on blueprints.
- Chunk based interest is no longer supported. All interest is resolved using query-based interest (QBI). You should remove streaming query and chunk based interest options from worker and launch config files to avoid unnecessary streaming queries being generated.
- If you already have a project that you are upgrading to this version of the GDK, it is encouraged to follow the upgrade process to SpatialOS `14.1.0`:
1. Open the `spatialos.json` file in the `spatial/` directory of your project.
1. Replace the `sdk_version` value and the version value of all dependencies with `14.1.0`.
1. Replace all other instances of the version number in the file.

### Features:
- The GDK now uses SpatialOS `14.1.0`.
- Visual Studio 2019 is now supported.
- You can now delete your schema database using options in the GDK toolbar and the commandlet.
- The GDK now checks that schema and a snapshot are present before attempting to start a local deployment. If either are missing then an error message is displayed.
- Added optional net relevancy check in replication prioritization. If enabled, an actor will only be replicated if IsNetRelevantFor is true for one of the connected client's views.
- You can now specify which actors should not persist as entities in your Snapshot. You do this by adding the flag `SPATIALCLASS_NotPersistent` to a class or by entering `NotPersistent` in the `Class Defaults` > `Spatial Description` field on blueprints.
- Deleted startup actors are now tracked.
- Added a user bindable delegate to `SpatialMetrics` which triggers when worker metrics have been received.
- Local deployments now create a new log file known as `launch.log` which will contain logs relating to starting and running a deployment. Additionally it will contain worker logs which are forwarded to the SpatialOS runtime.
- Added a new setting to SpatialOS Runtime Settings `Worker Log Level` which allows configuration of which verbosity of worker logs gets forwarded to the SpatialOS runtime.
- Added a new developer tool called 'Spatial Output Log' which will show local deployment logs from the `launch.log` file.
- Added logging for queued RPCs.
- Added several new STAT annotations into the ServerReplicateActors call chain.
- The GDK no longer generates schema for all UObject subclasses. Schema generation for Actor, ActorComponent and GameplayAbility subclasses is enabled by default, other classes can be enabled using `SpatialType` UCLASS specifier, or by checking the Spatial Type checkbox on blueprints.
- Added new experimental CookAndGenerateSchemaCommandlet that generates required schema during a regular cook.
- Added the `OverrideSpatialOffloading` command line flag. This allows you to toggle offloading at launch time.
- The initial connection from a worker will attempt to use relevant command line arguments (receptionistHost, locatorHost) to inform the connection. If these are not provided the standard connection flow will be followed. Subsequent connections will not use command line arguments.
- The command "Open 0.0.0.0" can be used to connect a worker using its command line arguments, simulating initial connection.
- The command "ConnectToLocator <login> <playerToken>" has been added to allow for explicit connections to deployments.
- Add SpatialDebugger and associated content.  This tool can be enabled via the SpatialToggleDebugger console command.  Documentation will be added for this soon.

### Bug fixes:
- Fixed a bug where the spatial daemon started even with spatial networking disabled.
- Fixed an issue that could cause multiple Channels to be created for an Actor.
- PlayerControllers on non-auth servers now have BeginPlay called with correct authority.
- Attempting to replicate unsupported types (such as TMap) results in an error rather than crashing the game.
- Generating schema when the schema database is locked by another process will no longer crash the editor.
- When the schema compiler fails, schema generation now displays an error.
- Fixed crash during initialization when running GenerateSchemaCommandlet.
- Generating schema after deleting the schema database now correctly triggers an initial schema generation.
- Streaming levels with query-based interest (QBI) enabled no longer produces errors if the player connection owns unreplicated actors.
- Fixed an issue that prevented player movement in a zoned deployment.
- Fixed an issue that caused queued incoming RPCs with unresolved references to never be processed.
- Muticast RPCs that are sent shortly after an actor is created are now correctly processed by all clients.
- When replicating an actor, the owner's Spatial position will no longer be used if it isn't replicated.
- Fixed a crash upon checking out an actor with a deleted static subobject.
- Fixed an issue where launching a cloud deployment with an invalid assembly name or deployment name wouldn't show a helpful error message.

## [`0.6.5`] - 2020-01-13
### Internal:
Features listed in the internal section are not ready to use but, in the spirit of open development, we detail every change we make to the GDK.
- **SpatialOS GDK for Unreal** > **Editor Settings** > **Region Settings** has been moved to **SpatialOS GDK for Unreal** > **Runtime Settings** > **Region Settings**.
- Local deployments can now be launched in China, when the **Region where services are located** is set to `CN`.

## [`0.6.4`] - 2019-12-13
### Bug fixes:
- The Inspector button in the SpatialOS GDK for Unreal toolbar now opens the correct URL.

## [`0.6.3`] - 2019-12-05
### Bug fixes:
- The C Worker SDK now communicates on port 443 instead of 444. This change is intended to protect your cloud deployments from DDoS attacks.

### Internal:
Features listed in the internal section are not ready to use but, in the spirit of open development, we detail every change we make to the GDK.
- The GDK is now compatible with the `CN` launch region. When Improbable's online services are fully working in China, they will work with this version of the GDK. You will be able to create SpatialOS Deployments in China by specifying the `CN` region in the Deployment Launcher.
- `Setup.bat` and `Setup.sh` both accept the `--china` flag, which will be required in order to run SpatialOS CLI commands in the `CN` region.
- **SpatialOS GDK for Unreal** > **Editor Settings** now contains a **Region Settings** section. You will be required to set **Region where services are located** to `CN` in order to create SpatialOS Deployments in China.

## [`0.6.2`] - 2019-10-10

- The GDK no longer relies on an ordering of entity and interest queries that is not guaranteed by the SpatialOS runtime.
- The multiserver offloading tutorial has been simplified and re-factored.

## [`0.6.1`] - 2019-08-15

### Features:
- The [Multiserver zoning shooter tutorial](https://docs.improbable.io/unreal/alpha/content/tutorials/multiserver-shooter/tutorial-multiserver-intro) has been updated to use the Example Project.

### Bug fixes:
- Simulated player launch configurations are no longer invalid when the GDK is installed as an Engine Plugin.
- RPCs that have been queued for execution for more than 1 second (the default value in `SpatialGDKSettings QueuedIncomingRPCWaitTime`) are now executed even if there are unresolved parameters. This stops unresolved parameters from blocking the execution queue.
- Offloading is no longer enabled by default in the Example Project. You can toggle offloading on using [these steps](https://docs.improbable.io/unreal/alpha/content/tutorials/offloading-tutorial/offloading-setup#step-4-enable-offloading).
- Guns no longer intermittently detatch from simulated players in the Example Project.
- Default cloud deployment settings are now correctly set. This means you don't need to manually reset them before doing a cloud deployment.

## [`0.6.0`] - 2019-07-31

### Breaking Changes:
* You must [re-build](https://docs.improbable.io/unreal/alpha/content/get-started/example-project/exampleproject-setup#step-4-build-the-dependencies-and-launch-the-project) your [Example Project](https://github.com/spatialos/UnrealGDKExampleProject) if you're upgrading it to `0.6.0`.
* This is the last GDK version to support Unreal Engine 4.20. You will need to upgrade your project to use Unreal Engine 4.22 (`4.22-SpatialOSUnrealGDK-release`) in order to continue receiving GDK releases and support.

### New Known Issues:
- Workers will sometimes not gain authority when quickly reconnecting to an existing deployment, resulting in a failure to spawn or simulate. When using the editor if you Play - Stop - Play in quick succession you can sometimes fail to launch correctly.

### Features:
- The GDK now uses SpatialOS `13.8.1`.
- Dynamic components are now supported. You can now dynamically attach and remove replicated subobjects to Actors.
- Local deployment startup time has been significantly reduced.
- Local deployments now start automatically when you select `Play`. This means you no longer need to select `Start` in the GDK toolbar before you select `Play` in the Unreal toolbar.
- If your schema has changed during a local deployment, the next time you select `Play` the deployment will automatically restart.
- Local deployments no longer run in a seperate Command Prompt. Logs from these deployments are now found in the Unreal Editor's Output Log.
- SpatialOS Runtime logs can now be found at `<GameRoot>\spatial\logs\localdeployment\<timestamp>\runtime.log`.
- An option to `Show spatial service button` has been added to the SpatialOS Settings menu. This button can be useful when debugging.
- Every time you open a GDK project in the Unreal Editor, 'spatial service' will be restarted. This ensures the service is always running in the correct SpatialOS project. You can disable this auto start feature via the new SpatialOS setting `Auto-start local deployment`.
- Added external schema code-generation tool for [non-Unreal server-worker types]({{urlRoot}}/content/non-unreal-server-worker-types). If you create non-Unreal server-worker types using the [SpatialOS Worker SDK](https://docs.improbable.io/reference/13.8/shared/sdks-and-data-overview) outside of the GDK and your Unreal Engine, you manually create [schema]({{urlRoot}/content/glossary#schema). Use the new [helper-script]({{urlRoot}}/content/helper-scripts) to generate Unreal code from manually-created schema; it enables your Unreal game code to interoperate with non-Unreal server-worker types.
- Added simulated player tools, which will allow you to create logic to simulate the behavior of real players. Simulated players can be used to scale test your game to hundreds of players. Simulated players can be launched locally as part of your development flow for quick iteration, as well as part of a separate "simulated player deployment" to connect a configurable amount of simulated players to your running game deployment.
- Factored out writing of Linux worker start scripts into a library, and added a standalone `WriteLinuxScript.exe` to _just_ write the launch script (for use in custom build pipelines).
- Added temporary MaxNetCullDistanceSquared to SpatialGDKSettings to prevent excessive net cull distances impacting runtime performance. Set to 0 to disable.
- Added `OnWorkerFlagsUpdated`, a delegate that can be directly bound to in C++. To bind via blueprints you can use the blueprint callable functions `BindToOnWorkerFlagsUpdated` and `UnbindToOnWorkerFlagsUpdated`. You can use `OnWorkerFlagsUpdated` to register when worker flag updates are received, which allows you to tweak values at deployment runtime.
- RPC frequency and payload size can now be tracked using console commands: `SpatialStartRPCMetrics` to start recording RPCs and `SpatialStopRPCMetrics` to stop recording and log the collected information. Using these commands will also start/stop RPC tracking on the server.
- Spatial now respects `bAlwaysRelevant` and clients will always checkout Actors that have `bAlwaysRelevant` set to true.

### Bug fixes:
- The `improbable` namespace has been renamed to `SpatialGDK`. This prevents namespace conflicts with the C++ SDK.
- Disconnected players no longer remain on the server until they time out if the client was shut down manually.
- Fixed support for relative paths as the engine association in your games .uproject file.
- RPCs on `NotSpatial` types are no longer queued forever and are now dropped instead.
- Fixed issue where an Actor's Spatial position was not updated if it had an owner that was not replicated.
- BeginPlay is now only called with authority on startup actors once per deployment.
- Fixed null pointer dereference crash when trying to initiate a Spatial connection without an existing one.
- URL options are now properly sent through to the server when doing a ClientTravel.
- The correct error message is now printed when the SchemaDatabase is missing.
- `StartEditor.bat` is now generated correctly when you build a server worker outside of editor.
- Fixed an issue with logging errored blueprints after garbage collection which caused an invalid pointer crash.
- Removed the ability to configure snapshot save folder. Snapshots should always be saved to `<ProjectRoot>/spatial/snapshots`. This prevents an issue with absolute paths being checked in which can break snapshot generation.
- Introduced a new module, `SpatialGDKServices`, on which `SpatialGDK` and `SpatilGDKEditorToolbar` now depend. This resolves a previously cyclic dependency.
- RPCs called before entity creation are now queued in case they cannot yet be executed. Previously they were simply dropped. These RPCs are also included in RPC metrics.
- RPCs are now guaranteed to arrive in the same order for a given actor and all of its subobjects on single-server deployments. This matches native Unreal behavior.

## [`0.5.0-preview`](https://github.com/spatialos/UnrealGDK/releases/tag/0.5.0-preview) - 2019-06-25
- Prevented `Spatial GDK Content` from appearing under Content Browser in the editor, as the GDK plugin does not contain any game content.

### Breaking Changes:
- If you are using Unreal Engine 4.22, the AutomationTool and UnrealBuildTool now require [.NET 4.6.2](https://dotnet.microsoft.com/download/dotnet-framework/net462).

### New Known Issues:

### Features:
- Unreal Engine 4.22 is now supported. You can find the 4.22 verson of our engine fork [here](https://github.com/improbableio/UnrealEngine/tree/4.22-SpatialOSUnrealGDK-release).
- Setup.bat can now take a project path as an argument. This allows the UnrealGDK to be installed as an Engine Plugin, pass the project path as the first variable if you are running Setup.bat from UnrealEngine/Engine/Plugins.
- Removed the need for setting the `UNREAL_HOME` environment variable. The build and setup scripts will now use your project's engine association to find the Unreal Engine directory. If an association is not set they will search parent directories looking for the 'Engine' folder.
- Added the `ASpatialMetricsDisplay` class, which you can use to view UnrealWorker stats as an overlay on the client.
- Added the runtime option `bEnableHandover`, which you can use to toggle property handover when running in non-zoned deployments.
- Added the runtime option `bEnableMetricsDisplay`, which you can use to auto spawn `ASpatialMetricsDisplay`, which is used to remote debug server metrics.
- Added the runtime option `bBatchSpatialPositionUpdates`, which you can use to batch spatial position updates to the runtime.
- Started using the [schema_compiler tool](https://docs.improbable.io/reference/13.8/shared/schema/introduction#using-the-schema-compiler-directly) to generate [schema descriptors](https://docs.improbable.io/reference/13.8/shared/flexible-project-layout/build-process/schema-descriptor-build-process#schema-descriptor-introduction) rather than relying on 'spatial local launch' to do this.
- Changed Interest so that NetCullDistanceSquared is used to define the distance from a player that the actor type is *interesting to* the player. This replaces CheckoutRadius which defined the distance that an actor is *interested in* other types. Requires engine update to remove the CheckoutRadius property which is no longer used.
- Added ActorInterestComponent that can be used to define interest queries that are more complex than a radius around the player position.
- Enabled new Development Authentication Flow
- Added new "worker" entities which are created for each server worker in a deployment so they correctly receive interest in the global state manager.
- Added support for spawning actors with ACLs configured for offloading using actor groups.
- Removed the references to the `Number of servers` slider in the Play in editor drop-down menu. The number of each server worker type to launch in PIE is now specified within the launch configuration in the `Spatial GDK Editor Settings` settings tab.
- Added `SpatialWorkerId` which is set to the worker ID when the worker associated to the `UGameInstance` connects.
- Added `USpatialStatics` helper blueprint library exposing functions for checking if SpatialOS networking is enabled, whether offloading is enabled, and more SpatialOS related checks.


### Bug fixes:
- BeginPlay is not called with authority when checking out entities from Spatial.
- Launching SpatialOS would fail if there was a space in the full directory path.
- GenerateSchemaAndSnapshots commandlet no longer runs a full schema generation for each map.
- Reliable RPC checking no longer breaks compatibility between development and shipping builds.
- Fixed an issue with schema name collisions.
- Running Schema (Full Scan) now clears generated schema files first.
- [Singleton actor's](https://docs.improbable.io/unreal/latest/content/singleton-actors#singleton-actors) authority and state now resumes correctly when reconnecting servers to snapshot.
- Retrying reliable RPCs with `UObject` arguments that were destroyed before the RPC was retried no longer causes a crash.
- Fixed path naming issues in setup.sh
- Fixed an assert/crash in `SpatialMetricsDisplay` that occurred when reloading a snapshot.
- Added Singleton and SingletonManager to query-based interest (QBI) constraints to fix issue preventing Test configuration builds from functioning correctly.
- Failing to `NetSerialize` a struct in spatial no longer causes a crash, it now prints a warning. This matches native Unreal behavior.
- Query response delegates now execute even if response status shows failure. This allows handlers to implement custom retry logic such as clients querying for the GSM.
- Fixed a crash where processing unreliable RPCs made assumption that the worker had authority over all entities in the SpatialOS op
- Ordering and reliability for single server RPCs on the same Actor are now guaranteed.

### External contributors:

In addition to all of the updates from Improbable, this release includes x improvements submitted by the incredible community of SpatialOS developers on GitHub! Thanks to these contributors:

* @cyberbibby

## [`0.4.2`](https://github.com/spatialos/UnrealGDK/releases/tag/0.4.2) - 2019-05-20

### New Known Issues:
- `BeginPlay()` is not called on all `WorldSettings` actors [#937](https://github.com/spatialos/UnrealGDK/issues/937)
- Replicated properties within `DEBUG` or `WITH_EDITORONLY_DATA` macros are not supported [#939](https://github.com/spatialos/UnrealGDK/issues/939)
- Client connections will be closed by the `ServerWorker` when using blueprint or C++ breakpoints during play-in-editor sessions [#940](https://github.com/spatialos/UnrealGDK/issues/940)
- Clients that connect after a Startup Actor (with `bNetLoadOnClient = true`) will not delete the Actor [#941](https://github.com/spatialos/UnrealGDK/issues/941)
- Generating schema while asset manager is asynchronously loading causes editor to crash [#944](https://github.com/spatialos/UnrealGDK/issues/944)

### Bug fixes:
- Adjusted dispatcher tickrate to reduce latency
- GenerateSchemaAndSnapshots commandlet no longer runs a full schema generation for each map.
- Launching SpatialOS would fail if there was a space in the full directory path.
- Fixed an issue with schema name collisions.
- Schema generation now respects "Directories to never cook".
- The editor no longer crashes during schema generation when the database is readonly.
- Replicating `UInterfaceProperty` no longer causes crashes.

## [`0.4.1`](https://github.com/spatialos/UnrealGDK/releases/tag/0.4.1) - 2019-05-01

### Bug fixes:
- Fixed an issue where schema components were sometimes generated with incorrect component IDs.

## [`0.4.0`](https://github.com/spatialos/UnrealGDK/releases/tag/0.4.0) - 2019-04-30

### Features:
- The GDK now uses SpatialOS `13.6.2`.
- Added this Changelog
- Added an error when unsupported replicated gameplay abilities are found in schema generation.
- Demoted various logs to Verbose in SpatialSender and SpatialReceiver
- You can now use the Project Settings window to pass [command line flags](https://docs.improbable.io/reference/latest/shared/spatial-cli/spatial-local-launch#spatial-local-launch) to local deployments launched from the GDK toolbar.
- You can now adjust the SpatialOS update frequency and the distance an action must move before we update its SpatialOS position.

### Bug fixes:
- The worker disconnection flow is now handled by `UEngine::OnNetworkFailure` rather than the existing `OnDisconnection` callback, which has been removed.
- Fix duplicated log messages in `spatial CLI` output when running in PIE.
- Fixed deserialization of strings from schema.
- Ensure that components added in blueprints are replicated.
- Fixed potential loading issue when attempting to load the SchemaDatabase asset.
- Add pragma once directive to header file.
- Schema files are now generated correctly for subobjects of the blueprint classes.
- Fixed being unable to launch SpatialOS if project path had spaces in it.
- Editor no longer crashes when setting LogSpatialSender to Verbose.
- Server-workers quickly restarted in the editor will connect to runtime correctly.
- Game no longer crashes when connecting to Spatial with async loading thread suspended.

## [`0.3.0`](https://github.com/spatialos/UnrealGDK/releases/tag/0.3.0) - 2019-04-04

### New Known Issues:
- Enabling Query Based Interest is needed for level streaming support, but this might affect performance in certain scenarios and is currently being investigated.
- Replicated `TimelineComponents` are not supported.

For current known issues, please visit [this](https://docs.improbable.io/unreal/alpha/known-issues) docs page

### Features:
- The default connection protocol is now TCP.
- Query Based Interest is now supported as an opt-in feature.
- Level streaming is now supported. You must enable Query Based Interest checkbox in the Runtime Settings to use level streaming.
- The GDK Toolbar now recognises when a local deployment is running, and contextually displays start and stop buttons. - (@DW-Sebastien)
- Added interface support for Unreal Engine 4.21 `UNetConnection`. - (@GeorgeR)
- Unreliable RPCs are now implemented using events instead of commands. This resolves associated performance issues.
- The `delete dynamic entities` setting now works when used in conjunction with multiple processes.
- You can now determine the type of a SpatialOS worker from within the game instance.
- Entity IDs are now reserved in batches instead of individually. This accelerates the creation of SpatialOS entities.
- You can now serialize and deserialize component data defined in external schema (schema that is not-generated by the Unreal GDK). You can use this to send and receive data, and edit snapshots.
- Improved logging during RPCs.

### Bug fixes:
- The GDK now automatically compiles all dirty blueprints before generating schema.
- Attempting to load a class which is not present in the schema database now causes the game to quit instead of crashing the entire editor.
- `Actor::ReplicateSubobjects` is now called in the replication flow. This means that Subobjects are now replicated correctly.
- Schema generation is no longer fatally halted when blueprints fail to compile.
- `AActor::TornOff` is now called when a `TearOff` event is received. This is in-line with the native implementation.
- References to objects within streaming levels, that are resolved before the level has streamed in, no longer cause defective behavior on the client.
- Attempting to replicate a `NonSpatial` actor no longer causes a crash.
- The SpatialOS Launcher now launches the correct game client, even when `UnrealCEFSubProcess.exe` is present in the assembly.
- Duplicate startup-actors are no longer created when a server-worker reconnects to a deployment.
- `BeginPlay` is no-longer called authoritatively when a server-worker reconnects to a deployment.
- Fast Array Serialization now operates correctly in conjunction with `GameplayAbilitySystem`.
- Reference parameters for RPCs are now correctly supported.
- Clients now load the map specified by the global state manager, rather than loading the `GameDefaultMap` _before_ querying the global state manager.
- Automatically generated launch configurations for deployments with a prime numbers of server-workers are now generated with the correct number of rows and columns.
- Generating schema for a level blueprint no longer deletes schema that has been generated for other levels.
- Deleting recently created actors no longer causes crashes.
- Having multiple EventGraphs no longer causes incorrect RPCs to be called.
- `TimerManager`, which is used by SpatialOS networking, is no longer subject to time dilation in the `World` instance.
- Clients no longer crash after being assigned multiple players.
- `GetWorkerFlag` can now be called from C++ classes.
- Pathless mapname arguments are now supported by the GDK commandlet.
- When `NotifyBeginPlay` is called, `BeginPlay` is no longer called on actors before their `Role` is correctly set.
- Deployments containing multiple server-workers no longer fails to initialize properly when launched through PIE with the `use single process` option unchecked.

### External contributors:

In addition to all of the updates from Improbable, this release includes 2 improvements submitted by the incredible community of SpatialOS developers on GitHub! Thanks to these contributors:

* @DW-Sebastien
* @GeorgeR

## [`0.2.0`](https://github.com/spatialos/UnrealGDK/releases/tag/0.2.0) - 2019-02-26

Startup actors revamp is merged! Snapshots are now simpler. Many bugfixes.

### New Known Issues:
- A warning about an out of date net driver is printed at startup of clients and server.

For current known issues, please visit [this](https://docs.improbable.io/unreal/alpha/known-issues) docs page

### Features:
- Actors placed in the level are no longer saved to the snapshot. They are instead spawned dynamically at the start of the game. This should fix quite a few issues such as missing references, and non-replicated instanced data being incorrectly set
- Pass player name and login options in the login URL
- Server will identify clients that have been disconnected from Spatial and trigger the cleanup on their NetConnection
- Exposed SpatialOS connection events in `USpatialNetDriver`
- Dynamic Component Ids now start from 10000, Gdk Components will now use 9999 - 0 to avoid future clashes
- Report an error during schema generation if a blueprint RPC has a "by reference" argument
- Launch configs can now be auto-generated to match the selected number of servers to launch from within the PIE editor
- Placeholder entities placed into the generated snapshot are now optional with a UI switch in the SpatialOS Settings
- Implemented updated functionality for UnrealGDKEditorCommandlet: Whenever loading a map for schema/snapshot generation, all sublevels will also be loaded before generation is started
	1. Will now loop through maps (skipping duplicates) during schema generation, to leverage the "iterative schema generation" feature
	2. Accepts an additional argument -MapPaths that can specify a collection of specific maps and/or directories (recursive) containing maps, delimited by semicolons. If not provided, defaults to "All maps in project"
	3. The paths passed in via -MapPaths are flexible

### Bug fixes:
- StartPlayInEditorGameInstance() now correctly call OnStart() on PIE_Client - (@DW-Sebastien)
- Redirect logging in the cloud to output to the correct file
- Changed type of key in `TMap` so Linux build will not give errors
- Disabled loopback of component updates
- Fix hanging on shutdown for PIE when disconnected from SpatialOS
- Fixed an issue which caused a character controller to not be destroyed when leaving the view of an observing client
- Fixed crash on multiserver PIE shutdown
- Fixed single-worker shutdown issues when launching SpatialOS through Unreal Engine 4 with Use - - Single Process unchecked in Play Options
- Fixed crash on closing client from cloud deployment
- Fix `DeleteDynamicEntities` not getting used correctly in shutdown
- Only call `BeginPlay()` on Actors if the World has begun play
- Fixed an issue with schema generation for the default GameMode
- Deleting the schema database reset the starting component ID
- Report invalid name errors during schema generation instead of when launching a deployment.
- `SchemaDatabase` can now be deleted and component ids will reset.
- `COND_InitialOnly` are only replicated once at the start
- Fixed a bug where standalone clients run locally would not connect to spatial
- Missing classes when connecting via a non-editor client
- Schema is now generated for classes that only have RPCs
- Fixed issue where properties won’t be replicated at the start of the game sometimes
- Fixed path bug when specifying snapshot output file in the settings
- Fixed up default connection flows
- Fixed issue will stale shadow data when crossing worker boundaries.
- Removed actors from replication consider list if Unreal server-worker is not authoritative over said actor
- Remove legacy flag "qos_max_unacked_pings_rate" in generated default config - (@DW-Sebastien)

### External contributors:
@DW-Sebastien

## [`0.1.0`](https://github.com/spatialos/UnrealGDK/releases/tag/0.1.0) - 2019-02-08

## Release Notes 0.1.0

Support for the new Player Auth APIs has been added and general stability improvements.

### New Known Issues:
Level streaming is currently not supported.
For other current known issues, please visit [this docs page](https://docs.improbable.io/unreal/alpha/known-issues).

### Features:
* Support for the new Player Auth APIs
* FUniqueNetId support
* Support for the new network protocol KCP
* Lazy loading of FClassInfo
* Augmented BuildWorker.bat to support additional UBT parameters
* Add IsValid() to FUnrealObjRef

### Bug fixes:
* Fixed critical errors related to Unresolved Objects
* Fixed a bug with Player State appearing to be null
* Fixed a bug related to Create Entity responses coming off the wire after a corresponding actor has been deleted
* Fixed a bug with activating actor components. We now check Initial Data for Actor components and only apply updates if `bReplicates` is true
* Fixed a bug when replicating a null list / array
* Fixed a crash with unresolved handover properties
* Changed RakNet to default network protocol temporarily to avoid performance issues with KCP
* Fixed a bug where cloud logging would not work correctly
