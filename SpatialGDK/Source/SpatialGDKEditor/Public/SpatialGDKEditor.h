// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Async/Future.h"
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSpatialGDKEditor, Log, All);

DECLARE_DELEGATE_OneParam(FSpatialGDKEditorErrorHandler, FString);

class SPATIALGDKEDITOR_API FSpatialGDKEditor
{
public:
	FSpatialGDKEditor();

	void GenerateSchema(FSimpleDelegate SuccessCallback, FSimpleDelegate FailureCallback, FSpatialGDKEditorErrorHandler ErrorCallback);
	void GenerateSnapshot(UWorld* World, FString SnapshotFilename, FSimpleDelegate SuccessCallback, FSimpleDelegate FailureCallback, FSpatialGDKEditorErrorHandler ErrorCallback);
	void LaunchCloudDeployment(const TCHAR * ProjectName, const TCHAR * AssemblyName, const TCHAR * PrimaryDeploymentName, const TCHAR * PrimaryLaunchConfigPath,
		const TCHAR * SnapshotPath, const bool IsSimulatedPlayersEnabled, const TCHAR * SimulatedPlayersDeploymentName, const TCHAR * SimulatedPlayerLaunchConfigPath,
		const TCHAR * NumberOfSimulatedPlayers, FSimpleDelegate SuccessCallback, FSimpleDelegate FailureCallback);

	bool IsSchemaGeneratorRunning() { return bSchemaGeneratorRunning; }

private:
	bool bSchemaGeneratorRunning;
	TFuture<bool> SchemaGeneratorResult;
};
