// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SDiversionSettings.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "SourceControlOperations.h"
#include "DiversionModule.h"
#include "DiversionUtils.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "DiversionOperations.h"


#define LOCTEXT_NAMESPACE "SDiversionSettings"

TSharedRef<SWidget> SDiversionSettings::DiversionNotInstalledWidget()
{
	const FText DiversionNotInstalled = LOCTEXT("DiversionNotInstalled", "Diversion is not installed. Please install Diversion and restart Unreal Engine.");
	const FText DiversionQuickstart = LOCTEXT("DiversionQuickstart", "Diversion Quickstart Guide");
	const FText DiversionQuickstart_Tooltip = LOCTEXT("DiversionQuickstart_Tooltip", "Open Diversion quickstart guide in your browser");

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(DiversionNotInstalled)
		]
		+ SVerticalBox::Slot()
		.FillHeight(2.5f)
		.Padding(4.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SButton)
						.Text(DiversionQuickstart)
						.ToolTipText(DiversionQuickstart_Tooltip)
						.OnClicked(this, &SDiversionSettings::OnClickedOpenDiversionQuickstart)
						.HAlign(HAlign_Center)
						.ContentPadding(6)
				]
		];
}

TSharedRef<SWidget> SDiversionSettings::DiversionRunAgentWidget()
{
	const FText DiversionRunAgent = LOCTEXT("DiversionRunAgent", "Diversion agent is not running. Please start the agent to use this plugin.");
	const FText DiversionRunAgentButton = LOCTEXT("DiversionRunAgentButton", "Start Diversion Agent");

	return SNew(SVerticalBox)
		.Visibility_Lambda(
			[] {return SettingCurrentScreen() == StartAgent ? EVisibility::Visible : EVisibility::Collapsed; }
		)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(DiversionRunAgent)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
				.Text(DiversionRunAgentButton)
				.OnClicked(this, &SDiversionSettings::OnClickedStartDiversionAgent)
				.HAlign(HAlign_Center)
				.ContentPadding(6)
		];
}

TSharedRef<SWidget> SDiversionSettings::ShowVersionWidget()
{
	const FText DiversionVersions = LOCTEXT("DiversionVersions", "Diversion version");
	const FText DiversionVersions_Tooltip = LOCTEXT("DiversionVersions_Tooltip", "Diversion and Plugin versions");
	
	return SNew(SHorizontalBox)
		.Visibility_Lambda(
			[] {return SettingCurrentScreen() != StartAgent ? EVisibility::Visible : EVisibility::Collapsed; }
		)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
				.Text(DiversionVersions)
				.ToolTipText(DiversionVersions_Tooltip)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.80f)
		[
			SNew(STextBlock)
				.Text(this, &SDiversionSettings::GetVersions)
				.ToolTipText(DiversionVersions_Tooltip)
		];
}

TSharedRef<SWidget> SDiversionSettings::DiversionExistingRepoWidget(const FText& RepositoryRootLabel, const FText& RepositoryRootLabel_Tooltip)
{
	return SNew(SVerticalBox)
		.Visibility_Lambda(
				[] {return SettingCurrentScreen() == RepoInitialized ? EVisibility::Visible : EVisibility::Collapsed; }
			)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
						.Text(RepositoryRootLabel)
						.ToolTipText(RepositoryRootLabel_Tooltip)
				]
			+ SHorizontalBox::Slot()
				.FillWidth(1.85f)
				[
					RootOfLocalDirectoryTextbox(RepositoryRootLabel_Tooltip)
				]
		];
}

TSharedRef<SWidget> SDiversionSettings::RootOfLocalDirectoryTextbox(const FText& RepositoryRootLabel_Tooltip)
{
	return SNew(SMultiLineEditableTextBox)
		.Text(this, &SDiversionSettings::GetPathToWorkspaceRoot)
		.IsReadOnly(true)
		.ToolTipText(RepositoryRootLabel_Tooltip)
		.HScrollBar(
			SNew(SScrollBar)
			.Orientation(Orient_Horizontal)
			.Thickness(FVector2D(1.0f, 1.0f))
		);
}

TSharedRef<SWidget> SDiversionSettings::DiversionInitializeRepoWidget(const FText& RepositoryRootLabel, const FText& RepositoryRootLabel_Tooltip)
{
	return SNew(SVerticalBox)
		.Visibility_Lambda(
			[] {return SettingCurrentScreen() == RepoCreation ? EVisibility::Visible : EVisibility::Collapsed; }
		)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)[
			SNew(SSeparator)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
					.Text(RepositoryRootLabel)
					.ToolTipText(RepositoryRootLabel_Tooltip)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.85f)
			[
				SNew(SDirectoryPicker)
					.OnDirectoryChanged(this, &SDiversionSettings::OnProjectPathPicked)
					.Directory(RepoPath)
					.ToolTipText(RepositoryRootLabel_Tooltip)
			]
		]
		//+ SVerticalBox::Slot()
		//.AutoHeight()
		//.Padding(2.0f)
		//.VAlign(VAlign_Center)[
		//	AddInitialCommit()
		//]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)[
			AddInitializeButton()
		];
}

TSharedRef<SWidget> SDiversionSettings::AddInitialCommit()
{
	const FText InitialDiversionCommit_Tooltip = LOCTEXT("InitialDiversionCommit_Tooltip", "Create an initial commit");
	const FText InitialDiversionCommit = LOCTEXT("InitialDiversionCommit", "Create an initial commit");
	const FText InitialCommitMessage_Tooltip = LOCTEXT("InitialCommitMessage_Tooltip", "Message of initial commit");

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SCheckBox)
				.ForegroundColor(FSlateColor::UseForeground())
				.ToolTipText(InitialDiversionCommit_Tooltip)
				.IsChecked(ECheckBoxState::Checked)
				.OnCheckStateChanged(this, &SDiversionSettings::OnCheckedInitialCommit)
				[
					SNew(STextBlock)
						.Text(InitialDiversionCommit)
				]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.85f)
		[
			SNew(SMultiLineEditableTextBox)
				.Text(this, &SDiversionSettings::GetInitialCommitMessage)
				.ToolTipText(InitialCommitMessage_Tooltip)
				.OnTextCommitted(this, &SDiversionSettings::OnInitialCommitMessageCommited)
		];
}

TSharedRef<SWidget> SDiversionSettings::AddInitializeButton()
{
	const FText DiversionInitRepository = LOCTEXT("DiversionInitRepository", "Initialize project with Diversion");
	const FText DiversionInitRepository_Tooltip = LOCTEXT("DiversionInitRepository_Tooltip", "Initialize current project as a new Diversion repository");

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SButton)
				.Text(DiversionInitRepository)
				.ToolTipText(DiversionInitRepository_Tooltip)
				.OnClicked(this, &SDiversionSettings::OnClickedInitializeDiversionRepository)
				.HAlign(HAlign_Center)
				.ContentPadding(6)
		];
}

SDiversionSettings::SettingsState SDiversionSettings::SettingCurrentScreen()
{
	auto& Provider = FDiversionModule::Get().GetProvider();
	if (!Provider.IsAgentAlive(EConcurrency::Asynchronous, false))
		return SettingsState::StartAgent;
	if (!Provider.IsEnabled())
		return SettingsState::RepoCreation;
	return SettingsState::RepoInitialized;
}

TSharedRef<SWidget> SDiversionSettings::MakeSettingsPanel()
{
	if(!DiversionUtils::IsDiversionInstalled())
	{
		// This is the only screen that won't dynamically change based on the status.
		// It's expected that the user will restart UE after installing Diversion.
		return DiversionNotInstalledWidget();
	}

	TSharedRef<SVerticalBox> MainSettingsBox =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			ShowVersionWidget()
		];

	auto RunAgentScreen = DiversionRunAgentWidget();
	MainSettingsBox->AddSlot()[RunAgentScreen];

	const FText RepositoryRootLabel = LOCTEXT("RepositoryRootLabel", "Root of the repository");
	const FText RepositoryRootLabel_Tooltip = LOCTEXT("RepositoryRootLabel_Tooltip", "Path to the root of the Diversion repository");

	auto ExistingRepoScreen = DiversionExistingRepoWidget(RepositoryRootLabel, RepositoryRootLabel_Tooltip);
	MainSettingsBox->AddSlot()[ExistingRepoScreen];
	auto InitializeRepoScreen = DiversionInitializeRepoWidget(RepositoryRootLabel, RepositoryRootLabel_Tooltip);
	MainSettingsBox->AddSlot()[InitializeRepoScreen];

	return MainSettingsBox;
}

void SDiversionSettings::Construct(const FArguments& InArgs)
{
	ChildSlot [ MakeSettingsPanel() ];
}

SDiversionSettings::~SDiversionSettings()
{
	RemoveInProgressNotification();
}

FText SDiversionSettings::GetVersions() const
{
	FDiversionProvider& Provider = FDiversionModule::Get().GetProvider();
	return FText::FromString(Provider.GetDiversionVersion().ToString() + TEXT(" (plugin v") + Provider.GetPluginVersion() + TEXT(")"));
}

FText SDiversionSettings::GetPathToWorkspaceRoot() const
{
	FDiversionModule& Diversion = FDiversionModule::Get();
	return FText::FromString(Diversion.GetProvider().GetPathToWorkspaceRoot());
}

FReply SDiversionSettings::OnClickedInitializeDiversionRepository()
{
	FDiversionModule& Diversion = FDiversionModule::Get();

	auto SendAnalyticsOperation = ISourceControlOperation::Create<FSendAnalytics>();
	SendAnalyticsOperation->SetEventName(FText::FromString("UE Plugin Init"));
	Diversion.GetProvider().Execute(SendAnalyticsOperation, nullptr, TArray<FString>(), EConcurrency::Asynchronous);

	const FString& PathToDiversionBinary = Diversion.AccessSettings().GetBinaryPath();
	const FString PathToProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	TArray<FString> InfoMessages;
	// TArray<FString> ErrorMessages;

	// 1.a. Synchronous (very quick) "dv init" operation: initialize a Diversion local repository with a .diversion/ subdirectory
	// Force init flags - a map with the -f flag to force the initialization of the repository with value of empty string
	TMap<FString, FString> forceFlags = { {TEXT("f"), TEXT("")} };
	// Escaping to support paths with spaces
	auto EscapedRepoPath = TEXT("\"") + RepoPath + TEXT("\"");

	// Make sure the provided RepoPath from options contains(is parent of) the project directory
	if (!PathToProjectDir.StartsWith(RepoPath))
	{
		// TODO: Use endpoint to make sure we can initialize the repository in this path?
		const FText NotificationText = FText::Format(LOCTEXT("InvalidRepoPath", "Provided repository path is not a parent of the current project: {0}. Please choose a dir that is a parent of the project dir (or the project dir itself)"), FText::FromString(RepoPath));
		DiversionUtils::ShowErrorNotification(NotificationText);
		return FReply::Handled();
	}

	#if PLATFORM_WINDOWS
	FString FullRepoPath = FPaths::ConvertRelativePathToFull(RepoPath).Replace(TEXT("/"), TEXT("\\"));
	#elif PLATFORM_LINUX || PLATFORM_MAC
	FString FullRepoPath = FPaths::ConvertRelativePathToFull(RepoPath);
	#endif
	if(FullRepoPath.EndsWith(TEXT("\\")) || FullRepoPath.EndsWith(TEXT("/")))
	{
		FullRepoPath = FullRepoPath.LeftChop(1);
	}
	const FString RepoName = FPaths::GetPathLeaf(FullRepoPath);

	auto InitRepoOperation = ISourceControlOperation::Create<FInitRepo>();
	InitRepoOperation->SetRepoName(RepoName);
	InitRepoOperation->SetRepoPath(FullRepoPath);

	Diversion.GetProvider().Execute(InitRepoOperation, nullptr, TArray<FString>(), EConcurrency::Synchronous,
		FSourceControlOperationComplete::CreateSP(this, &SDiversionSettings::OnSourceControlOperationComplete));

	Diversion.GetProvider().GetDiversionVersion(EConcurrency::Synchronous, true);
	Diversion.GetProvider().GetWsInfo(EConcurrency::Synchronous, true);

	// Check the new repository status to enable connection (branch, user e-mail)
	if(!Diversion.GetProvider().IsRepoFound())
	{
		// TODO: Currently error messages are returned as Info as well.
		// Will be solved upon changing to REST API call
		auto Msg = InfoMessages.Num() > 0 ? InfoMessages[0] : "Unknown error";
		const FText NotificationText = FText::Format(LOCTEXT("Initialization_Failure", "Repository initialization failed: {0}"), FText::FromString(Msg));
		DiversionUtils::ShowErrorNotification(NotificationText);
	}

	DisplayInfoNotification("Diversion is now synching your files in the background, this might take a while. Source control operations will be available once initial sync will be finished.");

	// Retrive agent status to update sync status as well
	Diversion.GetProvider().IsAgentAlive(EConcurrency::Synchronous, true);

	//if (Diversion.GetProvider().IsAvailable())
	//{
	//	if (bInitialCommit)
	//	{
	//		LaunchCommitOperation();
	//	}
	//}

	// Update the provider
	return FReply::Handled();
}

// Launch an asynchronous "Check-In(Commit)" operation and start an ongoing notification
void SDiversionSettings::LaunchCommitOperation()
{
	const auto CheckInOperation = ISourceControlOperation::Create<FCheckIn>();
	CheckInOperation->SetDescription(InitialCommitMessage);
	FDiversionModule& DiversionModule = FModuleManager::LoadModuleChecked<FDiversionModule>("Diversion");

	const ECommandResult::Type Result = DiversionModule.GetProvider().
		Execute(CheckInOperation, nullptr, TArray<FString>(), EConcurrency::Synchronous,
			FSourceControlOperationComplete::CreateSP(this, &SDiversionSettings::OnSourceControlOperationComplete));
	if (Result == ECommandResult::Succeeded)
	{
		DisplayInProgressNotification(CheckInOperation);
	}
	else
	{
		DisplayFailureNotification(CheckInOperation);
	}
}

FReply SDiversionSettings::OnClickedOpenDiversionQuickstart() const
{
	FPlatformProcess::LaunchURL(TEXT("https://docs.diversion.dev/quickstart?utm_source=ue-plugin&utm_medium=plugin"), nullptr, nullptr);
	return FReply::Handled();
}

FReply SDiversionSettings::OnClickedStartDiversionAgent() const
{
	auto& Provider = FDiversionModule::Get().GetProvider();
	DiversionUtils::StartDiversionAgent(FDiversionModule::Get().AccessSettings().GetBinaryPath());
	// Update the provider
	if (!Provider.IsAgentAlive(EConcurrency::Synchronous, true)) {
		FText Message = FText::FromString("Failed starting Diversion Sync Agent");
		DisplayFailureNotification(Message);
	}
	Provider.GetWsInfo(EConcurrency::Synchronous, true);

	return FReply::Handled();
}

/// Delegate called when a version control operation has completed: launch the next one and manage notifications
void SDiversionSettings::OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	RemoveInProgressNotification();

	// Report result with a notification
	if (InResult == ECommandResult::Succeeded)
	{
		DisplaySuccessNotification(InOperation);
	}
	else
	{
		DisplayFailureNotification(InOperation);
	}
}

// Display an ongoing notification during the whole operation
void SDiversionSettings::DisplayInProgressNotification(const FSourceControlOperationRef& InOperation)
{
	FNotificationInfo Info(InOperation->GetInProgressString());
	Info.bFireAndForget = false;
	Info.ExpireDuration = 0.0f;
	Info.FadeOutDuration = 1.0f;
	OperationInProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);
	if (OperationInProgressNotification.IsValid())
	{
		OperationInProgressNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

// Remove the ongoing notification at the end of the operation
void SDiversionSettings::RemoveInProgressNotification()
{
	if (OperationInProgressNotification.IsValid())
	{
		OperationInProgressNotification.Pin()->ExpireAndFadeout();
		OperationInProgressNotification.Reset();
	}
}

// Display a temporary success notification at the end of the operation
void SDiversionSettings::DisplayInfoNotification(const FString& NotificationText)
{
	FNotificationInfo Info(FText::FromString(NotificationText));
	Info.bUseSuccessFailIcons = true;
	Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.DefaultMessage"));
	Info.ExpireDuration = 10.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

// Display a temporary success notification at the end of the operation
void SDiversionSettings::DisplaySuccessNotification(const FSourceControlOperationRef& InOperation)
{
	const FText NotificationText = FText::Format(LOCTEXT("InitialCommit_Success", "{0} operation was successfull!"), FText::FromName(InOperation->GetName()));
	FNotificationInfo Info(NotificationText);
	Info.bUseSuccessFailIcons = true;
	Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
	FSlateNotificationManager::Get().AddNotification(Info);
}

// Display a temporary failure notification at the end of the operation
void SDiversionSettings::DisplayFailureNotification(const FSourceControlOperationRef& InOperation)
{
	const FText NotificationText = FText::Format(LOCTEXT("InitialCommit_Failure", "Error: {0} operation failed!"), FText::FromName(InOperation->GetName()));
	FNotificationInfo Info(NotificationText);
	Info.ExpireDuration = 8.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}


void SDiversionSettings::DisplayFailureNotification(const FText& Message) const
{
	FNotificationInfo Info(Message);
	Info.ExpireDuration = 8.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

#undef LOCTEXT_NAMESPACE
