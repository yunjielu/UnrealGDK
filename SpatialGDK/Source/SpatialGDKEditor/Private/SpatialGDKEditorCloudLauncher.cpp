#include "SpatialGDKEditorCloudLauncher.h"

#include "SpatialGDKEditorSettings.h"

bool SpatialGDKCloudLaunch(const TCHAR * ProjectName, const TCHAR * AssemblyName, const TCHAR * PrimaryDeploymentName, const TCHAR * PrimaryLaunchConfigPath,
	const TCHAR * SnapshotPath, const bool IsSimulatedPlayersEnabled, const TCHAR * SimulatedPlayersDeploymentName, const TCHAR * SimulatedPlayerLaunchConfigPath,
	const TCHAR * NumberOfSimulatedPlayers)
{
	bool SuccessfullyExecuted = true;
	uint32 DeploymentLauncherProcessID;

	const USpatialGDKEditorSettings* SpatialGDKSettings = GetDefault<USpatialGDKEditorSettings>();

	const FString ExecuteAbsolutePath = SpatialGDKSettings->GetSpatialOSDirectory();
	const FString CmdExecutable = TEXT("DeploymentLauncher.exe");

	FString CmdArguments = FString::Printf(
		TEXT("create %s %s %s %s %s "),
		ProjectName, AssemblyName, PrimaryDeploymentName, PrimaryLaunchConfigPath, SnapshotPath
	);

	if (IsSimulatedPlayersEnabled)
	{
		CmdArguments = FString::Printf(
			TEXT("%s %s %s %s"),
			*CmdArguments,
			SimulatedPlayersDeploymentName,
			SimulatedPlayerLaunchConfigPath,
			NumberOfSimulatedPlayers
		);
	}

	// Attmpt to detect a system wide version of the svn command line tools
	void* ReadPipe = nullptr, *WritePipe = nullptr;

	FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

	FString ConsoleOutput;
	FProcHandle DeploymentLauncherProcHandle = FPlatformProcess::CreateProc(
		*(CmdExecutable), *CmdArguments, false, true, false, &DeploymentLauncherProcessID, 0,
		*ExecuteAbsolutePath, ReadPipe, WritePipe);

	FPlatformProcess::Sleep(5);
	while (DeploymentLauncherProcHandle.IsValid() && !SuccessfullyExecuted)
	{
		ConsoleOutput = FPlatformProcess::ReadPipe(ReadPipe);
		SuccessfullyExecuted = false;
	}
	return SuccessfullyExecuted;
}
