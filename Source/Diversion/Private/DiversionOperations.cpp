// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionOperations.h"
#include "IDiversionWorker.h"
#include "DiversionCommand.h"
#include "DiversionModule.h"
#include "DiversionProvider.h"
#include "DiversionUtils.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include "Misc/Paths.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"


#define LOCTEXT_NAMESPACE "Diversion"

// Wrapper for the commands validity checks
bool ExecuteValidityCheck(FDiversionCommand& InCommand, const FName& WorkerName)
{
	if (!DiversionUtils::DiversionValidityCheck(InCommand.Operation->GetName() == WorkerName,
	InCommand.Operation->GetName().ToString() + " != " + WorkerName.ToString(), InCommand.WsInfo.AccountID)) {
		InCommand.MarkOperationCompleted(false);
		return false;
	}
	return true;
}

FText FAgentHealthCheck::ProgressString = FText::GetEmpty();

FText FGetWsInfo::ProgressString = FText::GetEmpty();

#pragma region Diversion API Workers

// 

FName FDiversionInitRepoWorker::GetName() const
{
	return "InitRepo";
}

bool FDiversionInitRepoWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	bool Success = true;
	TSharedRef<FInitRepo, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FInitRepo>(InCommand.Operation);

	Success &= DiversionUtils::RunRepoInit(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages, 
		Operation->GetRepoPath(), Operation->GetRepoName());

	InCommand.MarkOperationCompleted(Success);
	return Success;
}

//

FName FDiversionAnalyticsEventWorker::GetName() const
{
	return "SendAnalytics";
}

bool FDiversionAnalyticsEventWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	const TSharedRef<FSendAnalytics, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FSendAnalytics>(InCommand.Operation);

	bool Success = true;
	// TODO: Error message and info like in the connect
	Success &= DiversionUtils::SendAnalyticsEvent(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages, Operation->GetEventProperties());

	InCommand.MarkOperationCompleted(Success);
	return Success;
}


//

FName FDiversionGetPotentialConflicts::GetName() const
{
	return "GetPotentialConflicts";
}

bool FDiversionGetPotentialConflicts::Execute(class FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}
	
	bool Success = DiversionUtils::GetPotentialFileClashes(InCommand, InCommand.InfoMessages,
		InCommand.ErrorMessages, PotentialClashes, bRecursiveRequest);
	if(Success)
	{
		QueriedPaths = InCommand.Files;
	}
	InCommand.MarkOperationCompleted(Success);
	return Success;
}

bool FDiversionGetPotentialConflicts::UpdateStates() const
{
	bool bUpdated = IDiversionWorker::UpdateStates();
	auto& Provider = FDiversionModule::Get().GetProvider();
	bool FullRepoUpdateRequested = IsFullRepoStatusQuery(Provider.GetWsInfo().GetPath());
	bUpdated &= FDiversionModule::Get().GetProvider().UpdatePotentialConflictStates(PotentialClashes,
		FullRepoUpdateRequested);
	return bUpdated;
}

bool FDiversionGetPotentialConflicts::IsFullRepoStatusQuery(const FString& RepoPath) const
{
	// Search for the repo path in the queried files, if found and the query
	// is recursive - it means we have a repo-wide status info to update
	return QueriedPaths.Contains(RepoPath) && bRecursiveRequest;
}


//

FName FDiversionAgentHealthCheckWorker::GetName() const
{
	return "AgentHealthCheck";
}

bool FDiversionAgentHealthCheckWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	bool Success = true;

	Success &= DiversionUtils::RunAgentHealthCheck(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages);
	if(Success && InCommand.IsAgentAlive)
	{
		Success &= DiversionUtils::AgentInSync(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages);
	}

	InCommand.MarkOperationCompleted(Success);
	return Success;
}

bool FDiversionAgentHealthCheckWorker::UpdateStates() const
{
	auto& Provider = FDiversionModule::Get().GetProvider();
	auto Version = FDiversionVersion(AgentVersion);
	Version.SetAgentAlive(IsAgentAlive);
	Provider.SetDvVersion(Version);
	Provider.SetSyncStatus(SyncStatus);

	return IDiversionWorker::UpdateStates();
}

//

FName FDiversionWsInfoWorker::GetName() const
{
	return "GetWsInfo";
}

bool FDiversionWsInfoWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	bool Success = true;

	FString RootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	Success &= DiversionUtils::GetWorkspaceConfigByPath(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages, RootPath);

	InCommand.MarkOperationCompleted(Success);
	return Success;
}

bool FDiversionWsInfoWorker::UpdateStates() const
{
	auto& Provider = FDiversionModule::Get().GetProvider();
	Provider.SetWorkspaceInfo(WsInfo);

	return IDiversionWorker::UpdateStates();
}

//

FName FDiversionResolveFileWorker::GetName() const {
	return "ResolveFile";
}

bool FDiversionResolveFileWorker::Execute(FDiversionCommand& InCommand) {
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	bool Success = true;

	TSharedRef<FResolveFile, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FResolveFile>(InCommand.Operation);
	Success &= DiversionUtils::RunResolvePath(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages, Operation->GetMergeID(), Operation->GetConflictID());

	InCommand.MarkOperationCompleted(Success);
	return Success;
}

bool FDiversionResolveFileWorker::UpdateStates() const {
	auto& Provider = FDiversionModule::Get().GetProvider();
	Provider.ConflictedFiles.Remove(FileEntry.Path);
	return IDiversionWorker::UpdateStates();
}

//

FName FDiversionFinalizeMergeWorker::GetName() const
{
	return "FinalizeMerge";
}

bool FDiversionFinalizeMergeWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	bool Success = true;

	TSharedRef<FFinalizeMerge, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FFinalizeMerge>(InCommand.Operation);
	Success &= DiversionUtils::RunFinalizeMerge(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages, Operation->GetMergeID());

	InCommand.MarkOperationCompleted(Success);
	return Success;
}

#pragma endregion

#pragma region Source Control Workers

FName FDiversionConnectWorker::GetName() const
{
	return "Connect";
}

bool FDiversionConnectWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	bool Success = true;

	if (!DiversionUtils::IsDiversionInstalled()) {
		StaticCastSharedRef<FConnect>(InCommand.Operation)->SetErrorText(LOCTEXT("DiversionNotInstalled",
			"Diversion is not installed. Documentation available at https://docs.diversion.dev/ue-quickstart")
		);
		Success = false;
		InCommand.MarkOperationCompleted(Success);
		return Success;
	}
	
	if(!InCommand.IsAgentAlive){
		StaticCastSharedRef<FConnect>(InCommand.Operation)->SetErrorText(LOCTEXT("AgentIsDown",
			"Diversion agent is not running. Start it by clicking the start button or by running 'dv' in a cmd.")
		);
		Success = false;
		InCommand.MarkOperationCompleted(Success);
		return Success;
	} 

	if (!InCommand.WsInfo.IsValid()) {
		StaticCastSharedRef<FConnect>(InCommand.Operation)->SetErrorText(LOCTEXT("NotADiversionRepository",
			"Failed to enable Diversion revision control. You need to initialize the project as a Diversion repository first.")
		);
		Success = false;
		InCommand.MarkOperationCompleted(Success);
		return Success;
	}

	InCommand.MarkOperationCompleted(Success);
	return Success;
}

bool FDiversionConnectWorker::UpdateStates() const
{
	return IDiversionWorker::UpdateStates();
}

//

FName FDiversionCheckInWorker::GetName() const
{
	return "CheckIn";
}

static FText ParseCommitResults(const TArray<FString>& InResults)
{
	if (InResults.Num() >= 1)
	{
		const FString& FirstLine = InResults[0];
		return FText::Format(LOCTEXT("CommitMessage", "{0}."), FText::FromString(FirstLine));
	}
	return LOCTEXT("CommitMessageUnknown", "Submitted revision.");
}

bool FDiversionCheckInWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	bool Success = true;
	const TSharedRef<FCheckIn, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FCheckIn>(InCommand.Operation);

	Success &= DiversionUtils::RunCommit(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages, Operation->GetDescription().ToString());
	if(Success)
	{
		// TODO: print more elaborate success message using the info messages of the command
		Operation->SetSuccessMessage(ParseCommitResults(InCommand.InfoMessages));
		CommittedFilesPaths = InCommand.Files;
	}
	
	InCommand.MarkOperationCompleted(Success);
	return Success;
}

bool FDiversionCheckInWorker::UpdateStates() const
{
	// Remove deleted files from status cache
	auto& Provider = FDiversionModule::Get().GetProvider();
	
	TArray<TSharedRef<ISourceControlState, ESPMode::ThreadSafe>> LocalStates;
	Provider.GetState(CommittedFilesPaths, LocalStates, EStateCacheUsage::Use);
	for (const auto& State : LocalStates)
	{
		if (State->IsDeleted())
		{
			Provider.RemoveFileFromCache(State->GetFilename());
		}
	}

	Provider.SetReloadStatus();
	return IDiversionWorker::UpdateStates();
}

//

FName FDiversionMarkForAddWorker::GetName() const
{
	return "MarkForAdd";
}

bool FDiversionMarkForAddWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	InCommand.MarkOperationCompleted(true);
	return true;
}

//

FName FDiversionDeleteWorker::GetName() const
{
	return "Delete";
}

bool FDiversionDeleteWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	bool bSuccess = true;
	for(auto& File : InCommand.Files)
	{
		if (FPaths::FileExists(File)) {
			// Diversion sync agent will pick up the changes and update automatically
			// UE status will be synced in the background by the provider
			if (!FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*File))
			{
				UE_LOG(LogSourceControl, Error, TEXT("Diversion: Failed to delete file '%s'"), *File);
				bSuccess = false;
			}
		}
	}
	InCommand.MarkOperationCompleted(bSuccess);
	return bSuccess;
}

//

FName FDiversionRevertWorker::GetName() const
{
	return "Revert";
}

bool FDiversionRevertWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	bool Success = true;

	// Run reset on the given paths. Note! this reset is a hard reset, it will remove all changes to the files even erasing new additions.
	// Status will be updated in the BG by the provider
	Success &= DiversionUtils::RunReset(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages);
	Success &= DiversionUtils::NotifyAgentSyncRequired(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages);
	Success &= DiversionUtils::RunUpdateStatus(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages);
	bool temptyReturnContainer;
	Success &= DiversionUtils::GetPotentialFileClashes(InCommand, InCommand.InfoMessages,
		InCommand.ErrorMessages, PotentialClashes, temptyReturnContainer);
	
	bShouldUpdateStates = Success;
	
	InCommand.MarkOperationCompleted(Success);
	return Success;
}

bool FDiversionRevertWorker::UpdateStates() const
{
	if (!bShouldUpdateStates) {
		// Don't update states if we got an error from BE
		// No need to log, the error is in the API call
		return false;
	}

	bool bUpdated = IDiversionWorker::UpdateStates();

	FDiversionProvider& Provider = FDiversionModule::Get().GetProvider();

	// This is only fine since it's a synchronous blocking call!
	bool bIsFullUpdateStatus = false;
	bUpdated &= Provider.UpdateCachedStates(States, bIsFullUpdateStatus, Conflicts);
	bUpdated &= Provider.UpdatePotentialConflictStates(PotentialClashes, bIsFullUpdateStatus);

	return bUpdated;
}

// bool FDiversionRevertWorker::UpdateStates() const
// {
// 	if (!bShouldUpdateStates) {
// 		// Don't update states if we got an error from BE
// 		// No need to log, the error is in the API call
// 		return false;
// 	}
//
// 	bool bUpdated = IDiversionWorker::UpdateStates();
//
// 	FDiversionProvider& Provider = FDiversionModule::Get().GetProvider();
//
// 	// This is only fine since it's a synchronous blocking call!
// 	bUpdated &= Provider.UpdateCachedStates(States);
// 	if (OutOnGoingMerge) {
// 		Provider.OnGoingMerge = true;
// 		Provider.ConflictedFiles = Conflicts;
// 	}
//
// 	return bUpdated;
// }

//

// FName FDiversionSyncWorker::GetName() const
// {
//    return "Sync";
// }
//
// bool FDiversionSyncWorker::Execute(FDiversionCommand& InCommand)
// {
// 	if (!DiversionUtils::DiversionValidityCheck(InCommand.Operation->GetName() == GetName(),
// 		InCommand.Operation->GetName().ToString() + " != " + GetName().ToString(), InCommand.WsInfo.AccountID)) {
// 		InCommand.MarkOperationCompleted(false);
// 		return false;
// 	}
//
// 	InCommand.MarkOperationCompleted(InCommand.bCommandSuccessful);
// 	return InCommand.bCommandSuccessful;
// }

//

FName FDiversionUpdateStatusWorker::GetName() const
{
	return "UpdateStatus";
}

bool FDiversionUpdateStatusWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	const TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FUpdateStatus>(InCommand.Operation);
	bool Success = true;

	if (InCommand.Files.Num() == 0)
	{
		UE_LOG(LogSourceControl, Warning, TEXT("Diversion: No files to update status for."));
		InCommand.MarkOperationCompleted(true);
		return true;
	}
	
	if(InCommand.GetConcurrency() == EConcurrency::Synchronous)
	{
		// Synchronous call should only block when updating states history
		if (Operation->ShouldUpdateHistory())
		{
			Success = UpdateHistory(InCommand);
		}
		
		if(!Operation->ShouldUpdateModifiedState())
		{
			// UE uses this flag during commit operation call flow.
			// We want to avoid setting the files as waiting for sync in this case.
			// So it won't think they don't contain changes.
			QueriedPaths = InCommand.Files;
		}
		// Replace the sync call by a background async one -
		// this will trigger it (Avoid waiting for the next check interval)
		bTriggerStatusReload = true;
	}
	else
	{
		if (Operation->ShouldUpdateHistory())
		{
			UpdateHistory(InCommand);
		}
		// Call update status normally
		QueriedPaths = InCommand.Files;
		Success &= DiversionUtils::RunUpdateStatus(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages, false);
		bShouldUpdateStates = Success;
	}
	
	InCommand.MarkOperationCompleted(Success);
	return Success;
}


void FDiversionUpdateStatusWorker::MarkSynchingFiles() const
{
	auto& Provider = FDiversionModule::Get().GetProvider();
	// Mark the files as being synced to give the user immediate feedback
	for(auto& Path : QueriedPaths)
	{
		const TSharedRef<FDiversionState> State = Provider.GetStateInternal(Path);
		if (State->WorkingCopyState != EWorkingCopyState::Unknown)
		{
			State->IsSyncing = true;
			Provider.AddSynchingState(Path, State);
		}
	}
}

bool FDiversionUpdateStatusWorker::IsFullRepoStatusQuery(const FString& RepoPath) const
{
	// Seacrh for the repo path in the queried files, if found and the query
	// is recursive - it means we have a repo-wide status info to update
	return QueriedPaths.Contains(RepoPath) && bRecursiveRequest;
}

bool FDiversionUpdateStatusWorker::UpdateStates() const
{
	bool bUpdated = true;
	FDiversionProvider& Provider = FDiversionModule::Get().GetProvider();
	
	// update states history, if any
	for (const auto& History : Histories)
	{
		const TSharedRef<FDiversionState> State = Provider.GetStateInternal(History.Key);
		State->History = History.Value;
	}
	
	bool FullRepoUpdateRequested = IsFullRepoStatusQuery(Provider.GetWsInfo().GetPath());

	// This is a sync call - return here and trigger a BG status call
	if(bTriggerStatusReload)
	{
		if(!FullRepoUpdateRequested)
		{
			MarkSynchingFiles();
		}
		// Force a reload of the status to get the latest changes
		Provider.BackgroundStatusTriggerInstantCall();
		return true;
	}

	if(Provider.GetSyncStatus() != DiversionUtils::EDiversionWsSyncStatus::Completed)
	{
		// Handle synching status - mark the queried files as being synced
		if(!FullRepoUpdateRequested)
		{
			MarkSynchingFiles();
		}
	}

	// Async status call
	if(bShouldUpdateStates)
	{
		Provider.UpdateCachedStates(States, FullRepoUpdateRequested, Conflicts);
	}
	else
	{
		UE_LOG(LogSourceControl, Warning, TEXT("Diversion: Issue when updating file statuses"));
	}
	
	return true;
}

bool FDiversionUpdateStatusWorker::UpdateHistory(FDiversionCommand& InCommand)
{
	// When fetching history, we need to have the updated states data. so running update status first
	bool Success = DiversionUtils::RunUpdateStatus(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages);

	// TODO: optimize this like plastic implementation
	for(auto& State : States)
	{
		// Get the history of the file in the current branch
		Success &= DiversionUtils::RunGetHistory(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages, State.Key, nullptr);
		if (State.Value.IsConflicted())
		{
			// Get the revision data for the conflicted with file on the other branch
			DiversionUtils::RunGetHistory(InCommand, InCommand.InfoMessages, InCommand.ErrorMessages, 
				State.Key, &State.Value.PendingResolveInfo.RemoteRevision);
		}
	}

	return Success;
}

//

FName FDiversionCopyWorker::GetName() const
{
	return "Copy";
}

bool FDiversionCopyWorker::Execute(FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	const auto Operation = StaticCastSharedRef<FCopy>(InCommand.Operation);
	InCommand.bCommandSuccessful = true;

	if(InCommand.Files.Num() == 1)
	{
		// "branch" here is equal to move, in Perforce terminology
		if (Operation->CopyMethod == FCopy::ECopyMethod::Branch)
		{
			// Todo: Copy operator generally works, need to find if there's Diversion related edge cases
		}
	}

	InCommand.MarkOperationCompleted(InCommand.bCommandSuccessful);
	return InCommand.bCommandSuccessful;
}

bool FDiversionCopyWorker::UpdateStates() const
{
	return IDiversionWorker::UpdateStates();
}

//

FName FDiversionResolveWorker::GetName() const
{
	return "Resolve";
}

bool FDiversionResolveWorker::Execute(class FDiversionCommand& InCommand)
{
	if(!ExecuteValidityCheck(InCommand, GetName())) { return false;}

	for (const auto& File : InCommand.Files)
	{
		const auto FileName = DiversionUtils::ConvertRelativePathToDiversionFull(File, InCommand.WsInfo.GetPath());
		if (FDiversionResolveInfo* ResolveInfo = InCommand.ConflictedFiles.Find(FileName)) {
			FilesToResolve.Add(FileName, *ResolveInfo);
		}
	}

	InCommand.MarkOperationCompleted(InCommand.bCommandSuccessful);
	return InCommand.bCommandSuccessful;
}

bool FDiversionResolveWorker::UpdateStates() const
{
	auto& Provider = FDiversionModule::Get().GetProvider();
	Provider.FilesToResolve = FilesToResolve;
	// Remove the resolved files from the provider conflicted files
	for(const auto& File : FilesToResolve)
	{
		Provider.ConflictedFiles.Remove(File.Key);
	}

	return IDiversionWorker::UpdateStates();
}

#pragma endregion

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif


#undef LOCTEXT_NAMESPACE
