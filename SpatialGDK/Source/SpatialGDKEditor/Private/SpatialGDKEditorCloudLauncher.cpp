#include "SpatialGDKEditorCloudLauncher.h"

#include "SpatialGDKEditorSettings.h"
#include "SpatialGDKEditorCloudLauncherSettings.h"

bool SpatialGDKCloudLaunch()
{
	const USpatialGDKEditorCloudLauncherSettings* SpatialGDKCloudLauncherSettings = GetDefault<USpatialGDKEditorCloudLauncherSettings>();
	bool SuccessfullyExecuted = true;
	uint32 DeploymentLauncherProcessID;

	const USpatialGDKEditorSettings* SpatialGDKSettings = GetDefault<USpatialGDKEditorSettings>();

	const FString ExecuteAbsolutePath = SpatialGDKSettings->GetSpatialOSDirectory();
	const FString CmdExecutable = TEXT("DeploymentLauncher.exe");

	FString CmdArguments = FString::Printf(
		TEXT("create %s %s %s %s %s "),
		*SpatialGDKCloudLauncherSettings->GetProjectName(),
		*SpatialGDKCloudLauncherSettings->GetAssemblyName(),
		*SpatialGDKCloudLauncherSettings->GetPrimaryDeploymentName(),
		*SpatialGDKCloudLauncherSettings->GetPrimaryLanchConfigPath(),
		*SpatialGDKCloudLauncherSettings->GetSnapshotPath()
	);

	if (SpatialGDKCloudLauncherSettings->IsSimulatedPlayersEnabled())
	{
		CmdArguments = FString::Printf(
			TEXT("%s %s %s %s"),
			*CmdArguments,
			*SpatialGDKCloudLauncherSettings->GetSimulatedPlayerDeploymentName(),
			*SpatialGDKCloudLauncherSettings->GetSimulatedPlayerLaunchConfigPath(),
			*FString::FromInt(SpatialGDKCloudLauncherSettings->GetNumberOfSimulatedPlayer())
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
