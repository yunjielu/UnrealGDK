#include "SpatialGDKBotDeployment.h"
#include "DesktopPlatformModule.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

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
									.OnTextCommitted(this, &SSpatialGDKBotDeployment::OnProjectNameCommited)
									.OnTextChanged(this, &SSpatialGDKBotDeployment::OnProjectNameCommited, ETextCommit::Default)
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
									.Text(FText::FromString(FString(TEXT("Launch BotDeployment"))))
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
	if (FDesktopPlatformModule::Get()->OpenFileDialog(ParentWindowHandle, FString(TEXT("Choose a Default Configuration")), DefaultPath, TEXT(""), TEXT("Project files (*.uproject)|*.uproject"), EFileDialogFlags::None, OutFiles))
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

void SSpatialGDKBotDeployment::OnDeploymentAssemblyCommited(const FText & InText, ETextCommit::Type InCommitType) const
{
	// TODO: Save the assembly
}

void SSpatialGDKBotDeployment::OnProjectNameCommited(const FText & InText, ETextCommit::Type InCommitType) const
{
	// TODO: Save this InText.ToString() to somewhere
}

void SSpatialGDKBotDeployment::OnDeploymentNameCommited(const FText & InText, ETextCommit::Type InCommitType) const
{
	// TODO: Save the deployment name
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

bool SSpatialGDKBotDeployment::IsBotDeploymentConfigurationValid() const
{
	return false;
}
