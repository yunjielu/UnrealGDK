#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SEditableTextBox.h"

class SWindow;

class SSpatialGDKBotDeployment : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSpatialGDKBotDeployment) {}

	/** A reference to the parent window */
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)

	///** Callback to be called when the "Disable Source Control" button is pressed. */
	//SLATE_ARGUMENT(FSourceControlLoginClosed, OnSourceControlLoginClosed)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

private:

	FString DeploymentName;

	FString AssemblyName;

	FString ProjectName;

	FString PrimaryLaunchConfigPath;

	FString SnapshotPath;

	FString BotLaunchConfigPath;

	bool DeploymentNameIsValid;

	bool AssemblyNameIsValid;

	bool ProjectNameIsValid;

	uint32 NumOfSimulatedPlayers;

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

	/** Delegate to commit deployment name */
	void OnDeploymentNameCommited(const FText& InText, ETextCommit::Type InCommitType);

	void SetDeploymentName(const FString & Name);

	FText GetDeploymentName() const;

	/** Delegate called when the user has picked a path for the snapshot file */
	void OnSnapshotPathPicked(const FString & PickedPath);

	/** Sets the path to the snapshot file for the primary deployment */
	void SetSnapshotPath(const FString & Path);

	FString GetSnapshotPath() const;

	/** Delegate called when the user has picked a path for the primary launch configuration file */
	void OnPrimaryLaunchConfigPathPicked(const FString & PickedPath);

	/** Sets the path to the launch configuration file for the cloud deployment */
	void SetPrimaryLaunchConfigPath(const FString & Path);

	FString GetPrimaryLaunchConfigPath() const;

	/** Delegate to commit the number of Simulated Players */
	void OnNumberOfSimulatedPlayersCommited(uint32 NewValue);

	void SetNumberOfSimulatedPlayers(uint32 NumberOfPlayers);

	uint32 GetNumberOfSimulatedPlayers() const;

	/** Delegate called when the user has picked a path for the bot launch configuration file */
	void OnBotLaunchConfigPathPicked(const FString & PickedPath);

	/** Sets the path to the launch configuration file for the bot deployment */
	void SetBotLaunchConfigPath(const FString & BotLaunchConfigPath);

	FString GetBotLaunchConfigPath() const;

	/** Delegate called when the user clicks the 'Launch Bot Deployment' button */
	FReply OnLaunchClicked();

	/** Delegate called when the user clicks the 'Refresh' button */
	FReply OnRefreshClicked();

	/** Delegate called when the user clicks the 'Stop Deployment' button */
	FReply OnStopClicked();

	void ValidateAssemblyName();

	void ValidateProjectName();

	void ValidateDeploymentName();

	/** Indicates if the assembly name is valid based on its last check */
	bool IsAssemblyNameValid() const;

	/** Indicates if the project name is valid based on its last check */
	bool IsProjectNameValid() const;

	/** Indicates if the deployment name is valid based on its last check */
	bool IsDeploymentNameValid() const;

	/** Delegate to determine the 'Launch Bot Deployment' button enabled state */
	bool IsBotDeploymentConfigurationValid() const;
};
