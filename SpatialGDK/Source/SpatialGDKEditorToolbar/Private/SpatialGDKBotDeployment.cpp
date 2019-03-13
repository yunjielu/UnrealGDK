#include "SpatialGDKBotDeployment.h"
#include "DesktopPlatformModule.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Text/STextBlock.h"

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
									.Text(FText::FromString(FString(TEXT("Project Name"))))
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
									.Text(FText::FromString(FString(TEXT("Assembly Name"))))
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
									.Text(FText::FromString(FString(TEXT("Deployment Name"))))
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
									SNew(SEditableTextBox)
									.Text(FText::FromString(FString(TEXT("default.snapshot"))))
									.ToolTipText(FText::FromString(FString(TEXT("Relative snapshot file path."))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(0.5f)
								[
									SNew(SButton)
									.HAlign(HAlign_Center)
									.Text(FText::FromString(FString(TEXT("Browse..."))))
									.OnClicked(this, &SSpatialGDKBotDeployment::HandleBrowseSnapshotClicked)
								]
							]
							// Launch Config + File Picker
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
									SNew(SEditableTextBox)
									.Text(FText::FromString(FString(TEXT("Launch Config"))))
									.ToolTipText(FText::FromString(FString(TEXT("Launch Configuration File Path"))))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(0.5f)
								[
									SNew(SButton)
									.HAlign(HAlign_Center)
									.Text(FText::FromString(FString(TEXT("Browse..."))))
									.OnClicked(this, &SSpatialGDKBotDeployment::HandleBrowseLaunchConfigClicked)
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


FReply SSpatialGDKBotDeployment::HandleBrowseLaunchConfigClicked()
{
	FString DefaultPath = FPaths::RootDir();

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

	TArray<FString> OutFiles;
	if (FDesktopPlatformModule::Get()->OpenFileDialog(ParentWindowHandle, FString(TEXT("Choose a Default Configuration")), DefaultPath, TEXT(""), TEXT("Project files (*.json)|*.json"), EFileDialogFlags::None, OutFiles))
	{
		SetDefaultLaunchConfigPath(OutFiles[0]);
	}

	return FReply::Handled();
}

FReply SSpatialGDKBotDeployment::HandleBrowseSnapshotClicked()
{
	FString DefaultPath = FPaths::RootDir();

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

	TArray<FString> OutFiles;
	if (FDesktopPlatformModule::Get()->OpenFileDialog(ParentWindowHandle, FString(TEXT("Choose a Snapshot file")), DefaultPath, TEXT(""), TEXT("Project files (*.snapshot)|*.snapshot"), EFileDialogFlags::None, OutFiles))
	{
		SetDefaultLaunchConfigPath(OutFiles[0]);
	}

	return FReply::Handled();
}

void SSpatialGDKBotDeployment::SetDefaultLaunchConfigPath(FString LaunchConfigPath)
{
	// TODO: Save the launch config path
}

void SSpatialGDKBotDeployment::SetBotLaunchConfigPath(FString BotLaunchConfigPath)
{
	// TODO: Save the path somewhere
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

void SSpatialGDKBotDeployment::OnProjectNameCommited(const FText & InText, ETextCommit::Type InCommitType)
{
	SetProjectName(InText.ToString());
	ValidateProjectName();
}

void SSpatialGDKBotDeployment::SetProjectName(const FString & Name)
{
	ProjectName = Name;
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

void SSpatialGDKBotDeployment::OnNumberOfSimulatedPlayersCommited(uint32 NewValue) const
{
	// TODO: Save the number of simulated players
}

void SSpatialGDKBotDeployment::SetSnapshotPath(FString SnapshotPath)
{
	// TODO: Save the path somewhere
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
		IsDeploymentNameValid() &&
		IsAssemblyNameValid() &&
		IsProjectNameValid();
}
