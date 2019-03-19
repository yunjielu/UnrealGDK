// Copyright (c) Improbable Worlds Ltd, All Rights Reserved
#include "SpatialGDKEditorCloudLauncherSettings.h"

#include "Settings/LevelEditorPlaySettings.h"
#include "Internationalization/Regex.h"

USpatialGDKEditorCloudLauncherSettings::USpatialGDKEditorCloudLauncherSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FString USpatialGDKEditorCloudLauncherSettings::ToString()
{
	return TEXT("TODO");
	/*TArray<FStringFormatArg> Args;
	Args.Add(SpatialOSDirectory.Path);
	Args.Add(bDeleteDynamicEntities);
	Args.Add(bGenerateDefaultLaunchConfig);
	Args.Add(SpatialOSLaunchConfig.FilePath);
	Args.Add(bStopSpatialOnExit);
	Args.Add(SpatialOSSnapshotPath.Path);
	Args.Add(SpatialOSSnapshotFile);
	Args.Add(GeneratedSchemaOutputFolder.Path);
	Args.Add(bGeneratePlaceholderEntitiesInSnapshot);

	return FString::Format(TEXT(
		"ProjectRootFolder={0}, "
		"bDeleteDynamicEntities={1}"
		"bGenerateDefaultLaunchConfig={2}"
		"SpatialOSLaunchArgument={3}, "
		"bStopSpatialOnExit={4}, "
		"SpatialOSSnapshotPath={5}, "
		"SpatialOSSnapshotFile={6}, "
		"GeneratedSchemaOutputFolder={7}"
		"bGeneratePlaceholderEntitiesInSnapshot={8}")
		, Args);*/
}

void USpatialGDKEditorCloudLauncherSettings::ValidateAssemblyName()
{
	const FRegexPattern AssemblyPattern(TEXT("^[a-zA-Z0-9_.-]{5,64}$"));
	FRegexMatcher RegMatcher(AssemblyPattern, AssemblyName);

	AssemblyNameIsValid = RegMatcher.FindNext();
}

void USpatialGDKEditorCloudLauncherSettings::ValidateProjectName()
{
	const FRegexPattern ProjectPattern(TEXT("^[a-z0-9_]{3,32}$"));
	FRegexMatcher RegMatcher(ProjectPattern, ProjectName);

	ProjectNameIsValid = RegMatcher.FindNext();
}

void USpatialGDKEditorCloudLauncherSettings::ValidateDeploymentName()
{
	const FRegexPattern DeploymentPattern(TEXT("^[a-z0-9_]{2,32}$"));
	FRegexMatcher RegMatcher(DeploymentPattern, PrimaryDeploymentName);

	PrimaryDeploymentNameIsValid = RegMatcher.FindNext();
}

void USpatialGDKEditorCloudLauncherSettings::SetPrimaryDeploymentName(const FString & Name)
{
	PrimaryDeploymentName = Name;
}

void USpatialGDKEditorCloudLauncherSettings::SetAssemblyName(const FString & Name)
{
	AssemblyName = Name;
}

void USpatialGDKEditorCloudLauncherSettings::SetProjectName(const FString & Name)
{
	ProjectName = Name;
}

void USpatialGDKEditorCloudLauncherSettings::SetPrimaryLaunchConfigPath(const FString & Path)
{
	PrimaryLaunchConfigPath.FilePath = Path;
}

void USpatialGDKEditorCloudLauncherSettings::SetSnapshotPath(const FString & Path)
{
	SnapshotPath.FilePath = Path;
}

void USpatialGDKEditorCloudLauncherSettings::SetSimulatedPlayersEnabledState(bool IsEnabled)
{
	SimulatedPlayersIsEnabled = IsEnabled;
}

void USpatialGDKEditorCloudLauncherSettings::SetSimulatedPlayerDeploymentName(const FString & Name)
{
	SimulatedPlayerDeploymentName = Name;
}

void USpatialGDKEditorCloudLauncherSettings::SetSimulatedPlayerLaunchConfigPath(const FString & Path)
{
	SimulatedPlayerLaunchConfigPath.FilePath = Path;
}

void USpatialGDKEditorCloudLauncherSettings::SetNumberOfSimulatedPlayers(uint32 Number)
{
	NumberOfSimulatedPlayers = Number;
}
