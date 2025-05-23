// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "ISourceControlProvider.h"

class SNotificationItem;
namespace ETextCommit { enum Type : int; }

enum class ECheckBoxState : uint8;

class SDiversionSettings : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SDiversionSettings) {}
	
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SDiversionSettings() override;

private:
	FText GetInitialCommitMessage() const { return InitialCommitMessage; }
	FString GetRepoPathString() const { return RepoPath; }

	FText InitialCommitMessage = FText::FromString("Initial Unreal Engine commit");
	FString RepoPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	bool bInitialCommit = true;

	FText GetVersions() const;
	/** Delegate to get repository root and email from provider */
	FText GetPathToWorkspaceRoot() const;

	/** Delegate called when a version control operation has completed */
	void OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);

	/** Asynchronous operation progress notifications */
	TWeakPtr<SNotificationItem> OperationInProgressNotification;
	
	void DisplayInProgressNotification(const FSourceControlOperationRef& InOperation);
	void RemoveInProgressNotification();
	void DisplayInfoNotification(const FString& NotificationText);
	void DisplaySuccessNotification(const FSourceControlOperationRef& InOperation);
	void DisplayFailureNotification(const FSourceControlOperationRef& InOperation);
	void DisplayFailureNotification(const FText& Message) const;

	void LaunchCommitOperation();

private:
	// Widget functions
	TSharedRef<SWidget> MakeSettingsPanel();
	TSharedRef<SWidget> DiversionNotInstalledWidget();
	TSharedRef<SWidget> DiversionRunAgentWidget();
	TSharedRef<SWidget> ShowVersionWidget();
	TSharedRef<SWidget> DiversionExistingRepoWidget(const FText& RepositoryRootLabel, const FText& RepositoryRootLabel_Tooltip);
	TSharedRef<SWidget> RootOfLocalDirectoryTextbox(const FText& RepositoryRootLabel_Tooltip);
	TSharedRef<SWidget> DiversionInitializeRepoWidget(const FText& RepositoryRootLabel, const FText& RepositoryRootLabel_Tooltip);
	TSharedRef<SWidget> AddInitialCommit();
	TSharedRef<SWidget> AddInitializeButton();

	enum SettingsState
	{
		StartAgent,
		RepoCreation,
		RepoInitialized
	};

	static SettingsState SettingCurrentScreen();

private:
	// Click event handlers
	FReply OnClickedOpenDiversionQuickstart() const;
	FReply OnClickedStartDiversionAgent() const;
	/** Delegate to initialize a new Diversion repository */
	FReply OnClickedInitializeDiversionRepository();

	void OnCheckedInitialCommit(ECheckBoxState CheckBoxState) { bInitialCommit = CheckBoxState == ECheckBoxState::Checked; }
	void OnInitialCommitMessageCommited(const FText& Text, ETextCommit::Type Arg) { InitialCommitMessage = Text; }
	void OnProjectPathPicked(const FString& String) { RepoPath = String; }
};
