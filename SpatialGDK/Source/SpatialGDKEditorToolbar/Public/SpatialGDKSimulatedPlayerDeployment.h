#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SEditableTextBox.h"

class SWindow;

enum class ECheckBoxState : uint8;

class SSpatialGDKSimulatedPlayerDeployment : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSpatialGDKSimulatedPlayerDeployment) {}

	/** A reference to the parent window */
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

private:
	FProcHandle DeploymentLauncherProcHandle;
	uint32 DeploymentLauncherProcessID;

	FString PrimaryDeploymentName;
	FString AssemblyName;
	FString ProjectName;
	FString PrimaryLaunchConfigPath;
	FString SnapshotPath;
	FString SimulatedPlayerDeploymentName;
	FString SimulatedPlayerLaunchConfigPath;
	uint32 NumOfSimulatedPlayers;

	bool PrimaryDeploymentNameIsValid;
	bool AssemblyNameIsValid;
	bool ProjectNameIsValid;
	bool SimulatedPlayersIsActive;

	/** The parent window of this widget */
	TWeakPtr<SWindow> ParentWindowPtr;

	/** Delegate to commit assembly name */
	void OnDeploymentAssemblyCommited(const FText& InText, ETextCommit::Type InCommitType);
	void SetAssemblyName(const FString & Name);
	FText GetAssemblyName() const;

	/** Delegate to commit project name */
	void OnProjectNameCommited(const FText& InText, ETextCommit::Type InCommitType);
	void SetProjectName(const FString & Name);
	FText GetProjectName() const;

	/** Delegate to commit primary deployment name */
	void OnPrimaryDeploymentNameCommited(const FText& InText, ETextCommit::Type InCommitType);
	void SetPrimaryDeploymentName(const FString & Name);
	FText GetPrimaryDeploymentName() const;

	/** Delegate called when the user has picked a path for the snapshot file */
	void OnSnapshotPathPicked(const FString & PickedPath);
	void SetSnapshotPath(const FString & Path);
	FString GetSnapshotPath() const;

	/** Delegate called when the user has picked a path for the primary launch configuration file */
	void OnPrimaryLaunchConfigPathPicked(const FString & PickedPath);
	void SetPrimaryLaunchConfigPath(const FString & Path);
	FString GetPrimaryLaunchConfigPath() const;

	/** Delegate to commit simulated player deployment name */
	void OnSimulatedPlayerDeploymentNameCommited(const FText& InText, ETextCommit::Type InCommitType);
	void SetSimulatedPlayerDeploymentName(const FString & Name);
	FText GetSimulatedPlayerDeploymentName() const;

	/** Delegate to commit the number of Simulated Players */
	void OnNumberOfSimulatedPlayersCommited(uint32 NewValue);
	void SetNumberOfSimulatedPlayers(uint32 NumberOfPlayers);
	uint32 GetNumberOfSimulatedPlayers() const;

	/** Delegate called when the user has picked a path for the simulated player launch configuration file */
	void OnSimulatedPlayerLaunchConfigPathPicked(const FString & PickedPath);
	void SetSimulatedPlayerLaunchConfigPath(const FString & SimulatedPlayerLaunchConfigPath);
	FString GetSimulatedPlayerLaunchConfigPath() const;

	/** Delegate called when the user clicks the 'Launch Simulated Player Deployment' button */
	FReply OnLaunchClicked();

	/** Delegate called when the user clicks the 'Refresh' button */
	FReply OnRefreshClicked();

	/** Delegate called when the user clicks the 'Stop Deployment' button */
	FReply OnStopClicked();

	/** Delegate called when the user clicks the cloud deployment documentation */
	void OnCloudDocumentationClicked();

	void ValidateAssemblyName();
	void ValidateProjectName();
	void ValidateDeploymentName();

	/** Indicates if the assembly name is valid based on its last check */
	bool IsAssemblyNameValid() const;

	/** Indicates if the project name is valid based on its last check */
	bool IsProjectNameValid() const;

	/** Indicates if the deployment name is valid based on its last check */
	bool IsPrimaryDeploymentNameValid() const;

	bool IsSimulatedPlayersEnabled() const;

	/** Delegate called when the user either clicks the simulated players checkbox */
	void OnCheckedSimulatedPlayers(ECheckBoxState NewCheckedState);

	void SetSimulatedPlayersEnabledState(bool IsEnabled);

	/** Delegate to determine the 'Launch Simulated Player Deployment' button enabled state */
	bool IsDeploymentConfigurationValid() const;
};
