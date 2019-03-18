// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSpatialGDKCloudLauncher, Log, All);

SPATIALGDKEDITOR_API bool SpatialGDKCloudLaunch(const TCHAR * ProjectName, const TCHAR * AssemblyName, const TCHAR * PrimaryDeploymentName, const TCHAR * PrimaryLaunchConfigPath,
	const TCHAR * SnapshotPath, const bool IsSimulatedPlayersEnabled, const TCHAR * SimulatedPlayersDeploymentName, const TCHAR * SimulatedPlayerLaunchConfigPath,
	const TCHAR * NumberOfSimulatedPlayers);
