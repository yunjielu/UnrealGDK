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

	bool DeploymentNameIsValid;

	bool AssemblyNameIsValid;

	bool ProjectNameIsValid;

	/** The parent window of this widget */
	TWeakPtr<SWindow> ParentWindowPtr;

	/** Delegate to commit assembly name */
	void OnDeploymentAssemblyCommited(const FText& InText, ETextCommit::Type InCommitType);

	void SetAssemblyName(const FString & Name);

	/** Delegate to commit project name */
	void OnProjectNameCommited(const FText& InText, ETextCommit::Type InCommitType);

	void SetProjectName(const FString & Name);

	/** Delegate to commit deployment name */
	void OnDeploymentNameCommited(const FText& InText, ETextCommit::Type InCommitType);

	void SetDeploymentName(const FString & Name);

	/** Delegate to commit the number of Simulated Players */
	void OnNumberOfSimulatedPlayersCommited(uint32 NewValue) const;

	/** Sets the path to the snapshot file for the deployment */
	void SetSnapshotPath(FString SnapshotPath);

	/** Sets the path to the launch configuration file for the cloud deployment */
	void SetDefaultLaunchConfigPath(FString LaunchConfigPath);

	/** Sets the path to the launch configuration file for the bot deployment */
	void SetBotLaunchConfigPath(FString BotLaunchConfigPath);

	/** Delegate to determine the 'Launch Bot Deployment' button enabled state */
	bool IsBotDeploymentConfigurationValid() const;

	/** Delegate called when the user wants to browse and select the default launch configuration */
	FReply HandleBrowseLaunchConfigClicked();

	/** Delegate called when the user wants to browse and select the snapshot file */
	FReply HandleBrowseSnapshotClicked();

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
};
