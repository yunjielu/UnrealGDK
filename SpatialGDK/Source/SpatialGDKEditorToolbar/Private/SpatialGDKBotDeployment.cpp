#include "SpatialGDKBotDeployment.h"
#include "DesktopPlatformModule.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SFilePathPicker.h"
#include "EditorDirectories.h"

#include "Internationalization/Regex.h"

void SSpatialGDKBotDeployment::Construct(const FArguments& InArgs)
{
	ParentWindowPtr = InArgs._ParentWindow;

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
						.Padding(0.0f)
						[
							// Project 
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
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
									.Text(this, &SSpatialGDKBotDeployment::GetProjectName)
									.ToolTipText(FText::FromString(FString(TEXT("Project Name"))))
									.OnTextCommitted(this, &SSpatialGDKBotDeployment::OnProjectNameCommited)
									.OnTextChanged(this, &SSpatialGDKBotDeployment::OnProjectNameCommited, ETextCommit::Default)
								]
							]
							// Assembly Name 
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
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
									.Text(this, &SSpatialGDKBotDeployment::GetAssemblyName)
									.ToolTipText(FText::FromString(FString(TEXT("The name of the assembly."))))
									.OnTextCommitted(this, &SSpatialGDKBotDeployment::OnDeploymentAssemblyCommited)
									.OnTextChanged(this, &SSpatialGDKBotDeployment::OnDeploymentAssemblyCommited, ETextCommit::Default)
								]
							]
							// Deployment Name 
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
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
									.Text(this, &SSpatialGDKBotDeployment::GetDeploymentName)
									.ToolTipText(FText::FromString(FString(TEXT("The name of the deployment."))))
									.OnTextCommitted(this, &SSpatialGDKBotDeployment::OnDeploymentNameCommited)
									.OnTextChanged(this, &SSpatialGDKBotDeployment::OnDeploymentNameCommited, ETextCommit::Default)
								]
							]
							// Snapshot File + File Picker
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
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
									.BrowseDirectory(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_OPEN))
									.BrowseTitle(FText::FromString(FString(TEXT("File picker..."))))
									.FilePath(this, &SSpatialGDKBotDeployment::GetSnapshotPath)
									.FileTypeFilter(TEXT("Snapshot files (*.snapshot)|*.snapshot"))
									.OnPathPicked(this, &SSpatialGDKBotDeployment::OnSnapshotPathPicked)
								]
							]
							// Primary Launch Config + File Picker
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
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
									.BrowseDirectory(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_OPEN))
									.BrowseTitle(FText::FromString(FString(TEXT("File picker..."))))
									.FilePath(this, &SSpatialGDKBotDeployment::GetPrimaryLaunchConfigPath)
									.FileTypeFilter(TEXT("Configuration files (*.json)|*.json"))
									.OnPathPicked(this, &SSpatialGDKBotDeployment::OnPrimaryLaunchConfigPathPicked)
								]
							]
							// Simulated Players Number 
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
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
									.Value(0)
									.OnValueChanged(this, &SSpatialGDKBotDeployment::OnNumberOfSimulatedPlayersCommited)
								]
							]
							// Simulated Player Launch Config + File Picker
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
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
									.BrowseDirectory(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_OPEN))
									.BrowseTitle(FText::FromString(FString(TEXT("File picker..."))))
									.FilePath(this, &SSpatialGDKBotDeployment::GetBotLaunchConfigPath)
									.FileTypeFilter(TEXT("Configuration files (*.json)|*.json"))
									.OnPathPicked(this, &SSpatialGDKBotDeployment::OnBotLaunchConfigPathPicked)
								]
							]
							// Launch Bot Deployment Button
							+ SVerticalBox::Slot()
							.FillHeight(2.0f)
							.Padding(2.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SBox)
								.WidthOverride(500)
								[
									SNew(SButton)
									.HAlign(HAlign_Center)
									.Text(FText::FromString(FString(TEXT("Launch Bot Deployment"))))
									.OnClicked(this, &SSpatialGDKBotDeployment::OnLaunchClicked)
									.IsEnabled(this, &SSpatialGDKBotDeployment::IsBotDeploymentConfigurationValid)
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

void SSpatialGDKBotDeployment::OnDeploymentAssemblyCommited(const FText & InText, ETextCommit::Type InCommitType)
{
	SetAssemblyName(InText.ToString());
	ValidateAssemblyName();
}

void SSpatialGDKBotDeployment::SetAssemblyName(const FString & Name)
{
	AssemblyName = Name;
}

FText SSpatialGDKBotDeployment::GetAssemblyName() const
{
	return FText::FromString(AssemblyName);
}

void SSpatialGDKBotDeployment::OnProjectNameCommited(const FText & InText, ETextCommit::Type InCommitType)
{
	SetProjectName(InText.ToString());
	ValidateProjectName();
}

void SSpatialGDKBotDeployment::SetProjectName(const FString & Name)
{
	ProjectName = Name;
}

FText SSpatialGDKBotDeployment::GetProjectName() const
{
	return FText::FromString(ProjectName);
}

void SSpatialGDKBotDeployment::OnDeploymentNameCommited(const FText & InText, ETextCommit::Type InCommitType)
{
	 SetDeploymentName(InText.ToString());
	 ValidateDeploymentName();
}

void SSpatialGDKBotDeployment::SetDeploymentName(const FString & Name)
{
	DeploymentName = Name;
}

FText SSpatialGDKBotDeployment::GetDeploymentName() const
{
	return FText::FromString(DeploymentName);
}

void SSpatialGDKBotDeployment::OnSnapshotPathPicked(const FString & PickedPath)
{
	SSpatialGDKBotDeployment::SetSnapshotPath(PickedPath);
}

void SSpatialGDKBotDeployment::SetSnapshotPath(const FString & Path)
{
	SnapshotPath = Path;
}

FString SSpatialGDKBotDeployment::GetSnapshotPath() const
{
	return SnapshotPath;
}

void SSpatialGDKBotDeployment::OnPrimaryLaunchConfigPathPicked(const FString& PickedPath)
{
	SSpatialGDKBotDeployment::SetPrimaryLaunchConfigPath(PickedPath);
}

void SSpatialGDKBotDeployment::SetPrimaryLaunchConfigPath(const FString & Path)
{
	PrimaryLaunchConfigPath = Path;
}

FString SSpatialGDKBotDeployment::GetPrimaryLaunchConfigPath() const
{
	return PrimaryLaunchConfigPath;
}

void SSpatialGDKBotDeployment::OnNumberOfSimulatedPlayersCommited(uint32 NewValue)
{
	SSpatialGDKBotDeployment::SetNumberOfSimulatedPlayers(NewValue);
}

void SSpatialGDKBotDeployment::SetNumberOfSimulatedPlayers(uint32 NumberOfPlayers)
{
	NumOfSimulatedPlayers = NumberOfPlayers;
}

void SSpatialGDKBotDeployment::OnBotLaunchConfigPathPicked(const FString & PickedPath)
{
	SSpatialGDKBotDeployment::SetBotLaunchConfigPath(PickedPath);
}

void SSpatialGDKBotDeployment::SetBotLaunchConfigPath(const FString & Path)
{
	BotLaunchConfigPath = Path;
}

FString SSpatialGDKBotDeployment::GetBotLaunchConfigPath() const
{
	return BotLaunchConfigPath;
}

FReply SSpatialGDKBotDeployment::OnLaunchClicked()
{
	// TODO: Launch Bot Deployment :)
	return FReply::Handled();
}

FReply SSpatialGDKBotDeployment::OnRefreshClicked()
{
	// TODO: Invoke Platform SDK to refresh the deployment list
	return FReply::Handled();
}

FReply SSpatialGDKBotDeployment::OnStopClicked()
{
	// TODO: Invoke the Platform SDK to stop the deployments
	return FReply::Handled();
}

void SSpatialGDKBotDeployment::ValidateAssemblyName()
{
	const FRegexPattern AssemblyPattern(TEXT("^[a-zA-Z0-9_.-]{5,64}$"));
	FRegexMatcher RegMatcher(AssemblyPattern, AssemblyName);

	AssemblyNameIsValid = RegMatcher.FindNext();
}

void SSpatialGDKBotDeployment::ValidateProjectName()
{
	const FRegexPattern ProjectPattern(TEXT("^[a-z0-9_]{3,32}$"));
	FRegexMatcher RegMatcher(ProjectPattern, ProjectName);

	ProjectNameIsValid = RegMatcher.FindNext();
}

void SSpatialGDKBotDeployment::ValidateDeploymentName()
{
	const FRegexPattern DeploymentPattern(TEXT("^[a-z0-9_]{2,32}$"));
	FRegexMatcher RegMatcher(DeploymentPattern, DeploymentName);

	DeploymentNameIsValid = RegMatcher.FindNext();
}

bool SSpatialGDKBotDeployment::IsAssemblyNameValid() const
{
	return AssemblyNameIsValid;
}

bool SSpatialGDKBotDeployment::IsProjectNameValid() const
{
	return ProjectNameIsValid;
}

bool SSpatialGDKBotDeployment::IsDeploymentNameValid() const
{
	return DeploymentNameIsValid;
}

bool SSpatialGDKBotDeployment::IsBotDeploymentConfigurationValid() const
{
	return
		IsAssemblyNameValid() &&
		IsProjectNameValid() &&
		IsDeploymentNameValid();
}
