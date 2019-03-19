#include "SpatialGDKSimulatedPlayerDeployment.h"
#include "DesktopPlatformModule.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SFilePathPicker.h"
#include "SharedPointer.h"
#include "Widgets/Input/SHyperlink.h"
#include "SpatialGDKEditorSettings.h"
#include "SpatialGDKEditorCloudLauncherSettings.h"
#include "EditorDirectories.h"

#include "Internationalization/Regex.h"

void SSpatialGDKSimulatedPlayerDeployment::Construct(const FArguments& InArgs)
{
	const USpatialGDKEditorSettings* SpatialGDKSettings = GetDefault<USpatialGDKEditorSettings>();
	USpatialGDKEditorCloudLauncherSettings* SpatialGDKCloudLauncherSettings = GetMutableDefault<USpatialGDKEditorCloudLauncherSettings>();
	AssemblyName = "UnrealAssembly";

	ParentWindowPtr = InArgs._ParentWindow;
	SpatialGDKEditorPtr = InArgs._SpatialGDKEditor;

	ChildSlot
		[
			SNew(SBorder)
			.HAlign(HAlign_Fill)
			.BorderImage(FEditorStyle::GetBrush("ChildWindow.Background"))
			.Padding(4.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(4.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(1.0f)
						[
							SNew(SVerticalBox)
							// Build explanation set
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("NOTE: The assembly has to be built and uploaded manually. Follow the docs "))))
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SHyperlink)
									.Text(FText::FromString(FString(TEXT("here."))))
									.OnNavigate(this, &SSpatialGDKSimulatedPlayerDeployment::OnCloudDocumentationClicked)
								]
							]
							// Separator
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SSeparator)
							]
							// Project 
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("Project Name"))))
									.ToolTipText(FText::FromString(FString(TEXT("The name of the Improbable project."))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SEditableTextBox)
									.Text(FText::FromString(SpatialGDKCloudLauncherSettings->GetProjectName()))
									.ToolTipText(FText::FromString(FString(TEXT("Project Name"))))
									.OnTextCommitted(this, &SSpatialGDKSimulatedPlayerDeployment::OnProjectNameCommited)
									.OnTextChanged(this, &SSpatialGDKSimulatedPlayerDeployment::OnProjectNameCommited, ETextCommit::Default)
								]
							]
							// Assembly Name 
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("Assembly Name"))))
									.ToolTipText(FText::FromString(FString(TEXT("The name of the assembly file."))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SEditableTextBox)
									.Text(FText::FromString(SpatialGDKCloudLauncherSettings->GetAssemblyName()))
									.ToolTipText(FText::FromString(FString(TEXT("The name of the assembly."))))
									.OnTextCommitted(this, &SSpatialGDKSimulatedPlayerDeployment::OnDeploymentAssemblyCommited)
									.OnTextChanged(this, &SSpatialGDKSimulatedPlayerDeployment::OnDeploymentAssemblyCommited, ETextCommit::Default)
								]
							]
							// Pirmary Deployment Name 
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("Deployment Name"))))
									.ToolTipText(FText::FromString(FString(TEXT("The name of the deployment."))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SEditableTextBox)
									.Text(FText::FromString(SpatialGDKCloudLauncherSettings->GetPrimaryDeploymentName()))
									.ToolTipText(FText::FromString(FString(TEXT("The name of the deployment."))))
									.OnTextCommitted(this, &SSpatialGDKSimulatedPlayerDeployment::OnPrimaryDeploymentNameCommited)
									.OnTextChanged(this, &SSpatialGDKSimulatedPlayerDeployment::OnPrimaryDeploymentNameCommited, ETextCommit::Default)
								]
							]
							// Snapshot File + File Picker
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("Snapshot File"))))
									.ToolTipText(FText::FromString(FString(TEXT("The relative path to the snapshot file."))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SFilePathPicker)
									.BrowseButtonImage(FEditorStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
									.BrowseButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
									.BrowseButtonToolTip(FText::FromString(FString(TEXT("Path to the snapshot file"))))
									.BrowseDirectory(SpatialGDKSettings->GetSpatialOSSnapshotFolderPath())
									.BrowseTitle(FText::FromString(FString(TEXT("File picker..."))))
									.FilePath(SpatialGDKCloudLauncherSettings->GetSnapshotPath())
									.FileTypeFilter(TEXT("Snapshot files (*.snapshot)|*.snapshot"))
									.OnPathPicked(this, &SSpatialGDKSimulatedPlayerDeployment::OnSnapshotPathPicked)
								]
							]
							// Primary Launch Config + File Picker
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("Launch Config File"))))
									.ToolTipText(FText::FromString(FString(TEXT("The relative path to the launch configuration file."))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SFilePathPicker)
									.BrowseButtonImage(FEditorStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
									.BrowseButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
									.BrowseButtonToolTip(FText::FromString(FString(TEXT("Path to the primary launch configuration"))))
									.BrowseDirectory(SpatialGDKSettings->GetSpatialOSDirectory())
									.BrowseTitle(FText::FromString(FString(TEXT("File picker..."))))
									.FilePath(SpatialGDKCloudLauncherSettings->GetPrimaryLanchConfigPath())
									.FileTypeFilter(TEXT("Configuration files (*.json)|*.json"))
									.OnPathPicked(this, &SSpatialGDKSimulatedPlayerDeployment::OnPrimaryLaunchConfigPathPicked)
								]
							]
							// Separator
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SSeparator)
							]
							// Explanation text
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SCheckBox)
									.IsChecked(SpatialGDKCloudLauncherSettings->IsSimulatedPlayersEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
									.ToolTipText(FText::FromString(FString(TEXT("Toggle to scale test."))))
									.OnCheckStateChanged(this, &SSpatialGDKSimulatedPlayerDeployment::OnCheckedSimulatedPlayers)
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("Simulated Player Deployment Configuration"))))
								]
							]
							// Simulated Players Deployment Name
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("Deployment Name"))))
									.ToolTipText(FText::FromString(FString(TEXT("The name of the simulated player deployment."))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SEditableTextBox)
									.Text(FText::FromString(SpatialGDKCloudLauncherSettings->GetSimulatedPlayerDeploymentName()))
									.ToolTipText(FText::FromString(FString(TEXT("The name of the simulated player deployment."))))
									.OnTextCommitted(this, &SSpatialGDKSimulatedPlayerDeployment::OnSimulatedPlayerDeploymentNameCommited)
									.OnTextChanged(this, &SSpatialGDKSimulatedPlayerDeployment::OnSimulatedPlayerDeploymentNameCommited, ETextCommit::Default)
									.IsEnabled(this, &SSpatialGDKSimulatedPlayerDeployment::IsSimulatedPlayersEnabled)
								]
							]
							// Simulated Players Number 
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("Number of Simulated Players"))))
									.ToolTipText(FText::FromString(FString(TEXT("The number of Simulated Players to be launched and connect to the game."))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SSpinBox<uint32>)
									.ToolTipText(FText::FromString(FString(TEXT("The name of the deployment."))))
									.MinValue(1)
									.MaxValue(8192)
									.Value(SpatialGDKCloudLauncherSettings->GetNumberOfSimulatedPlayer())
									.OnValueChanged(this, &SSpatialGDKSimulatedPlayerDeployment::OnNumberOfSimulatedPlayersCommited)
									.IsEnabled(this, &SSpatialGDKSimulatedPlayerDeployment::IsSimulatedPlayersEnabled)
								]
							]
							// Simulated Player Launch Config + File Picker
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString(TEXT("Simulated Player Launch Config File"))))
									.ToolTipText(FText::FromString(FString(TEXT("The relative path to the simulated player launch configuration file."))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SFilePathPicker)
									.BrowseButtonImage(FEditorStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
									.BrowseButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
									.BrowseButtonToolTip(FText::FromString(FString(TEXT("Path to the simulated player launch configuration"))))
									.BrowseDirectory(SpatialGDKSettings->GetSpatialOSDirectory())
									.BrowseTitle(FText::FromString(FString(TEXT("File picker..."))))
									.FilePath(SpatialGDKCloudLauncherSettings->GetSimulatedPlayerLaunchConfigPath())
									.FileTypeFilter(TEXT("Configuration files (*.json)|*.json"))
									.OnPathPicked(this, &SSpatialGDKSimulatedPlayerDeployment::OnSimulatedPlayerLaunchConfigPathPicked)
									.IsEnabled(this, &SSpatialGDKSimulatedPlayerDeployment::IsSimulatedPlayersEnabled)
								]
							]
							// Expandeable Logs
							+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 6.0f, 0.0f, 0.0f)
								[
									SNew(SExpandableArea)
									.AreaTitleFont(FEditorStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font")))
									.AreaTitle(FText::FromString(FString(TEXT("Simulated Player Logs"))))
									.BorderImage(FEditorStyle::GetBrush("NoBorder"))
									.IsEnabled(true)
									.InitiallyCollapsed(false)
									.BodyContent()
									[
										SNew(SBox)
										.HeightOverride(250)
										.WidthOverride(400)
										[
											SNew(STextBlock)
											.Text(FText::FromString(FString(TEXT("Loading..."))))
										]
								]
							]
							// Launch Simulated Players Deployment Button
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SBox)
								.WidthOverride(500)
								[
									SNew(SButton)
									.HAlign(HAlign_Center)
									.Text(FText::FromString(FString(TEXT("Launch Simulated Players Deployment"))))
									.OnClicked(this, &SSpatialGDKSimulatedPlayerDeployment::OnLaunchClicked)
									.IsEnabled(this, &SSpatialGDKSimulatedPlayerDeployment::IsDeploymentConfigurationValid)
								]
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
					]
				]
			]
		];
}

void SSpatialGDKSimulatedPlayerDeployment::OnDeploymentAssemblyCommited(const FText & InText, ETextCommit::Type InCommitType)
{
	SetAssemblyName(InText.ToString());
	ValidateAssemblyName();
}

void SSpatialGDKSimulatedPlayerDeployment::SetAssemblyName(const FString & Name)
{
	AssemblyName = Name;
}

FText SSpatialGDKSimulatedPlayerDeployment::GetAssemblyName() const
{
	return FText::FromString(AssemblyName);
}

void SSpatialGDKSimulatedPlayerDeployment::OnProjectNameCommited(const FText & InText, ETextCommit::Type InCommitType)
{
	USpatialGDKEditorCloudLauncherSettings* SpatialGDKCloudLauncherSettings = GetMutableDefault<USpatialGDKEditorCloudLauncherSettings>();
	SpatialGDKCloudLauncherSettings->SetProjectName(InText.ToString());
	SetProjectName(InText.ToString());
	ValidateProjectName();
}

void SSpatialGDKSimulatedPlayerDeployment::SetProjectName(const FString & Name)
{
	ProjectName = Name;
}

FText SSpatialGDKSimulatedPlayerDeployment::GetProjectName() const
{
	return FText::FromString(ProjectName);
}

void SSpatialGDKSimulatedPlayerDeployment::OnPrimaryDeploymentNameCommited(const FText & InText, ETextCommit::Type InCommitType)
{
	USpatialGDKEditorCloudLauncherSettings* SpatialGDKCloudLauncherSettings = GetMutableDefault<USpatialGDKEditorCloudLauncherSettings>();
	SpatialGDKCloudLauncherSettings->SetPrimaryDeploymentName(InText.ToString());
	SetPrimaryDeploymentName(InText.ToString());
	ValidateDeploymentName();
}

void SSpatialGDKSimulatedPlayerDeployment::SetPrimaryDeploymentName(const FString & Name)
{
	PrimaryDeploymentName = Name;
}

FText SSpatialGDKSimulatedPlayerDeployment::GetPrimaryDeploymentName() const
{
	return FText::FromString(PrimaryDeploymentName);
}

void SSpatialGDKSimulatedPlayerDeployment::OnSnapshotPathPicked(const FString & PickedPath)
{
	SSpatialGDKSimulatedPlayerDeployment::SetSnapshotPath(PickedPath);
}

void SSpatialGDKSimulatedPlayerDeployment::SetSnapshotPath(const FString & Path)
{
	SnapshotPath = Path;
}

FString SSpatialGDKSimulatedPlayerDeployment::GetSnapshotPath() const
{
	return SnapshotPath;
}

void SSpatialGDKSimulatedPlayerDeployment::OnPrimaryLaunchConfigPathPicked(const FString& PickedPath)
{
	SSpatialGDKSimulatedPlayerDeployment::SetPrimaryLaunchConfigPath(PickedPath);
}

void SSpatialGDKSimulatedPlayerDeployment::SetPrimaryLaunchConfigPath(const FString & Path)
{
	PrimaryLaunchConfigPath = Path;
}

FString SSpatialGDKSimulatedPlayerDeployment::GetPrimaryLaunchConfigPath() const
{
	return PrimaryLaunchConfigPath;
}

void SSpatialGDKSimulatedPlayerDeployment::OnSimulatedPlayerDeploymentNameCommited(const FText & InText, ETextCommit::Type InCommitType)
{
	SSpatialGDKSimulatedPlayerDeployment::SetSimulatedPlayerDeploymentName(InText.ToString());
}

void SSpatialGDKSimulatedPlayerDeployment::SetSimulatedPlayerDeploymentName(const FString & Name)
{
	SimulatedPlayerDeploymentName = Name;
}

FText SSpatialGDKSimulatedPlayerDeployment::GetSimulatedPlayerDeploymentName() const
{
	return FText::FromString(SimulatedPlayerDeploymentName);
}

void SSpatialGDKSimulatedPlayerDeployment::OnNumberOfSimulatedPlayersCommited(uint32 NewValue)
{
	SSpatialGDKSimulatedPlayerDeployment::SetNumberOfSimulatedPlayers(NewValue);
}

void SSpatialGDKSimulatedPlayerDeployment::SetNumberOfSimulatedPlayers(uint32 NumberOfPlayers)
{
	NumOfSimulatedPlayers = NumberOfPlayers;
}

uint32 SSpatialGDKSimulatedPlayerDeployment::GetNumberOfSimulatedPlayers() const
{
	return NumOfSimulatedPlayers;
}

void SSpatialGDKSimulatedPlayerDeployment::OnSimulatedPlayerLaunchConfigPathPicked(const FString & PickedPath)
{
	SSpatialGDKSimulatedPlayerDeployment::SetSimulatedPlayerLaunchConfigPath(PickedPath);
}

void SSpatialGDKSimulatedPlayerDeployment::SetSimulatedPlayerLaunchConfigPath(const FString & Path)
{
	SimulatedPlayerLaunchConfigPath = Path;
}

FString SSpatialGDKSimulatedPlayerDeployment::GetSimulatedPlayerLaunchConfigPath() const
{
	return SimulatedPlayerLaunchConfigPath;
}

FReply SSpatialGDKSimulatedPlayerDeployment::OnLaunchClicked()
{
	if (TSharedPtr<FSpatialGDKEditor> SpatialGDKEditorSharedPtr = SpatialGDKEditorPtr.Pin()) {
		FNotificationInfo Info(FText::FromString(TEXT("Starting simulated player deployment...")));
		Info.bUseSuccessFailIcons = true;
		Info.bFireAndForget = false;

		TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);

		NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);

		SpatialGDKEditorSharedPtr->LaunchCloudDeployment(
			*SSpatialGDKSimulatedPlayerDeployment::GetProjectName().ToString(),
			*SSpatialGDKSimulatedPlayerDeployment::GetAssemblyName().ToString(),
			*SSpatialGDKSimulatedPlayerDeployment::GetPrimaryDeploymentName().ToString(),
			*SSpatialGDKSimulatedPlayerDeployment::GetPrimaryLaunchConfigPath(),
			*SSpatialGDKSimulatedPlayerDeployment::GetSnapshotPath(),
			SSpatialGDKSimulatedPlayerDeployment::IsSimulatedPlayersEnabled(),
			*SSpatialGDKSimulatedPlayerDeployment::GetSimulatedPlayerDeploymentName().ToString(),
			*SSpatialGDKSimulatedPlayerDeployment::GetSimulatedPlayerLaunchConfigPath(),
			*FString::FromInt(SSpatialGDKSimulatedPlayerDeployment::GetNumberOfSimulatedPlayers()),
			FSimpleDelegate::CreateLambda([NotificationItem]() {
				NotificationItem->SetText(FText::FromString(TEXT("We have liftoff")));
				NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
			}),
			FSimpleDelegate::CreateLambda([NotificationItem]() {
				NotificationItem->SetText(FText::FromString(TEXT("We don't have liftoff")));
				NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
			})
		);
		return FReply::Handled();
	}

	return FReply::Handled();
}

FReply SSpatialGDKSimulatedPlayerDeployment::OnRefreshClicked()
{
	// TODO: Invoke Platform SDK to refresh the deployment list
	return FReply::Handled();
}

FReply SSpatialGDKSimulatedPlayerDeployment::OnStopClicked()
{
	// TODO: Invoke the Platform SDK to stop the deployments
	return FReply::Handled();
}

void SSpatialGDKSimulatedPlayerDeployment::OnCloudDocumentationClicked()
{
	FString WebError;
	FPlatformProcess::LaunchURL(TEXT("https://docs.improbable.io/unreal/alpha/content/get-started/tutorial#build-your-assemblies"), TEXT(""), &WebError);
	if (!WebError.IsEmpty())
	{
		FNotificationInfo Info(FText::FromString(WebError));
		Info.ExpireDuration = 3.0f;
		Info.bUseSuccessFailIcons = true;
		TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
		NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
		NotificationItem->ExpireAndFadeout();
	}
}

void SSpatialGDKSimulatedPlayerDeployment::ValidateAssemblyName()
{
	const FRegexPattern AssemblyPattern(TEXT("^[a-zA-Z0-9_.-]{5,64}$"));
	FRegexMatcher RegMatcher(AssemblyPattern, AssemblyName);

	AssemblyNameIsValid = RegMatcher.FindNext();
}

void SSpatialGDKSimulatedPlayerDeployment::ValidateProjectName()
{
	const FRegexPattern ProjectPattern(TEXT("^[a-z0-9_]{3,32}$"));
	FRegexMatcher RegMatcher(ProjectPattern, ProjectName);

	ProjectNameIsValid = RegMatcher.FindNext();
}

void SSpatialGDKSimulatedPlayerDeployment::ValidateDeploymentName()
{
	const FRegexPattern DeploymentPattern(TEXT("^[a-z0-9_]{2,32}$"));
	FRegexMatcher RegMatcher(DeploymentPattern, PrimaryDeploymentName);

	PrimaryDeploymentNameIsValid = RegMatcher.FindNext();
}

bool SSpatialGDKSimulatedPlayerDeployment::IsAssemblyNameValid() const
{
	return AssemblyNameIsValid;
}

bool SSpatialGDKSimulatedPlayerDeployment::IsProjectNameValid() const
{
	return ProjectNameIsValid;
}

bool SSpatialGDKSimulatedPlayerDeployment::IsPrimaryDeploymentNameValid() const
{
	return PrimaryDeploymentNameIsValid;
}

bool SSpatialGDKSimulatedPlayerDeployment::IsSimulatedPlayersEnabled() const
{
	return SimulatedPlayersIsActive;
}

void SSpatialGDKSimulatedPlayerDeployment::OnCheckedSimulatedPlayers(ECheckBoxState NewCheckedState)
{
	SSpatialGDKSimulatedPlayerDeployment::SetSimulatedPlayersEnabledState(NewCheckedState == ECheckBoxState::Checked);
}

void SSpatialGDKSimulatedPlayerDeployment::SetSimulatedPlayersEnabledState(bool IsEnabled)
{
	SimulatedPlayersIsActive = IsEnabled;
}

bool SSpatialGDKSimulatedPlayerDeployment::IsDeploymentConfigurationValid() const
{
	return
		IsAssemblyNameValid() &&
		IsProjectNameValid() &&
		IsPrimaryDeploymentNameValid();
}
