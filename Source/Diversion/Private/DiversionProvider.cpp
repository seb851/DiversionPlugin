// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionProvider.h"
#include "DiversionState.h"
#include "Misc/Paths.h"
#include "Misc/QueuedThreadPool.h"
#include "DiversionCommand.h"
#include "DiversionConfig.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"
#include "DiversionModule.h"
#include "DiversionUtils.h"
#include "SDiversionSettings.h"
#include "SourceControlOperations.h"
#include "Logging/MessageLog.h"
#include "ScopedSourceControlProgress.h"
#include "Interfaces/IPluginManager.h"
#include "DiversionOperations.h"
#include "DiversionOperations.h"

#define LOCTEXT_NAMESPACE "Diversion"

static FName ProviderName("Diversion");


void OverrideLocalizationStrings()
{
	FTextLocalizationResource LocalizationResource;
	
	// PackagesDialogModule Assets checkout overrides
	LocalizationResource.AddEntry(FTextKey("FileHelpers"),
		FTextKey("Warning_Notification"),
		FString("Warning: Assets have conflict in Revision Control or cannot be written to disk"),
		FString("Warning: Assets you have edited are also being edited in another workspace. To see who edited them cancel this operation and hover the asset in the Content Browser."),
		100);
	
	LocalizationResource.AddEntry(FTextKey("PackagesDialogModule"),
		FTextKey("CheckoutPackagesDialogTitle"),
		FString("Check Out Assets"),
		FString("Override Auto Soft-Locked Assets"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("PackagesDialogModule"),
		FTextKey("CheckoutPackagesDialogMessage"),
		FString("Select assets to check out."),
		FString("Select assets to save."),
		100);

	LocalizationResource.AddEntry(FTextKey("PackagesDialogModule"),
		FTextKey("CheckoutPackagesWarnMessage"),
		FString("Warning: There are modified assets which you will not be able to check out as they are locked or not at the head revision. You may lose your changes if you continue, as you will be unable to submit them to revision control."),
		FString("Warning: These assets are currently being edited in another workspace or branch and were automatically soft-locked. Saving files that are auto soft-locked is possibe, but might create conflicts."),
		100);

	LocalizationResource.AddEntry(FTextKey("PackagesDialogModule"),
		FTextKey("Dlg_MakeWritableButton"),
		FString("Make Writable"),
		FString("Override Soft-Lock"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("PackagesDialogModule"),
		FTextKey("Dlg_MakeWritableTooltip"),
		FString("Makes selected files writable on disk"),
		FString("Save your changes, potentially creating conflicts"),
		100);

	LocalizationResource.AddEntry(FTextKey("PackagesDialogModule"),
		FTextKey("Dlg_CheckOutButtonp"),
		FString("Check Out Selected"),
		FString("Reset Changes"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("PackagesDialogModule"),
		FTextKey("Dlg_CheckOutTooltip"),
		FString("Attempt to Check Out Checked Assets"),
		FString("Use the asset's context menu to reset changes"),
		100);
	
	// General SPackagesDialog overrides
	LocalizationResource.AddEntry(FTextKey("SPackagesDialog"),
		FTextKey("CheckedOutByColumnLabel"),
		FString("Checked Out By"),
		FString("Auto Soft-Locked By"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("UnsavedAssetsTracker"),
		FTextKey("Locked_by_Other_Warning"),
		FString("Warning: Assets you have edited are locked by another user."),
		FString("Warning: Assets you have edited are also being edited in another branch or workspace."),
		100);
	
	LocalizationResource.AddEntry(FTextKey("AssetSourceControlContextMenu"),
		FTextKey("SCCRevert"),
		FString("Revert"),
		FString("Reset"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("AssetSourceControlContextMenu"),
		FTextKey("SCCRevertTooltip"),
		FString("Reverts the selected assets to their original state from revision control."),
		FString("Resets the selected assets to their original state from Diversion."),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl"),
		FTextKey("SourceControl_CheckIn"),
		FString("Checking file(s) into Revision Control..."),
		FString("Committing file(s) to Diversion..."),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl"),
		FTextKey("SourceControl_Revert"),
		FString("Reverting file(s) in Revision Control..."),
		FString("Resetting file(s) in Diversion..."),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl.RevertWindow"),
		FTextKey("Title"),
		FString("Revert Files"),
		FString("Reset Files"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl.Revert"),
		FTextKey("SelectFiles"),
		FString("Select the files that should be reverted below"),
		FString("Select the files to reset below"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl.Revert"),
		FTextKey("RevertUnchanged"),
		FString("Revert Unchanged Only"),
		FString("Reset Unchanged Only"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SSourceControlRevert"),
		FTextKey("RevertButton"),
		FString("Revert"),
		FString("Reset"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("FileSourceControlActions"),
		FTextKey("SCCRevertTooltip"),
		FString("Reverts the selected files to their original state from revision control."),
		FString("Resets the selected files to their original state from Diversion."),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControlWindows"),
		FTextKey("ChooseAssetsToCheckInIndicator"),
		FString("Checking for assets to check in..."),
		FString("Waiting for selection of assets to commit..."),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControlWindows"),
		FTextKey("NoAssetsToCheckIn"),
		FString("No assets to check in!"),
		FString("No changes to commit"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("UICommands.SourceControlCommands"),
		FTextKey("SubmitContent_ToolTip"),
		FString("Opens a dialog with check in options for content and levels."),
		FString("Opens a dialog with commit options for content and levels."),
		100);
	
	LocalizationResource.AddEntry(FTextKey("UICommands.SourceControlCommands"),
		FTextKey("SubmitContent"),
		FString("Submit Content"),
		FString("Commit Changes"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("UICommands.SourceControlCommands"),
		FTextKey("CheckOutModifiedFiles_ToolTip"),
		FString("Opens a dialog to check out any assets which have been modified."),
		FString("Saves all modified assets."),
		100);
	
	LocalizationResource.AddEntry(FTextKey("UICommands.SourceControlCommands"),
		FTextKey("CheckOutModifiedFiles"),
		FString("Check Out Modified Files"),
		FString("Save Modified Files"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl.SubmitPanel"),
		FTextKey("ChangeListDesc"),
		FString("Changelist Description"),
		FString("Commit Message"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl.SubmitPanel"),
		FTextKey("OKButton"),
		FString("Submit"),
		FString("Commit"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl.SubmitPanel"),
		FTextKey("KeepCheckedOut"),
		FString("Keep Files Checked Out"),
		FString("Thank you for using Diversion!"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl.SubmitPanel"),
		FTextKey("ChangeListDescWarning"),
		FString("Changelist description is required to submit"),
		FString("Commit message is required"),
		100);
	
	LocalizationResource.AddEntry(FTextKey("SourceControl.SubmitWindow"),
		FTextKey("Title"),
		FString("Submit Files"),
		FString("Commit Files"),
		100);
	
	FTextLocalizationManager::Get().UpdateFromLocalizationResource(LocalizationResource);
}

void FDiversionProvider::Init(bool bForceConnection)
{
	// Init() is called multiple times at startup: do not check dv each time
	if (!bDiversionAvailable)
	{
		OverrideLocalizationStrings();
		LastHttpTickTime = FPlatformTime::Seconds();
		if (GetDiversionVersion(EConcurrency::Synchronous, true).IsValid())
		{
			GetWsInfo(EConcurrency::Synchronous, true);
		}
		CheckDiversionAvailability();

		// Handle "Resolve" on package save and "Finalize Merge"
		ConflictedFiles.Empty();
		UPackage::PackageSavedEvent.AddRaw(this, &FDiversionProvider::OnPackageSave);

		auto& Module = FDiversionModule::Get();
		if (!DiversionUtils::DiversionValidityCheck(&Module != nullptr, 
				"Diversion Module is not valid!", GetWsInfo().AccountID)) {
			return;
		}
		// Set the original account ID with which the plugin was loaded
		Module.SetOriginalAccountID(GetWsInfo().AccountID);

		auto SendAnalyticsOperation = ISourceControlOperation::Create<FSendAnalytics>();
		SendAnalyticsOperation->SetEventName(FText::FromString("UE Plugin Load"));
		SendAnalyticsOperation->SetEventProperties({{"ue_softlock_enabled", IsDiversionSoftLockEnabled() ? "true" : "false"}});
		Execute(SendAnalyticsOperation, nullptr, TArray<FString>(), EConcurrency::Asynchronous);

		FAgentHealthCheck::SetProgressString();
		FGetWsInfo::SetProgressString();

		// Set the project path delegate
		ISourceControlModule::Get().RegisterSourceControlProjectDirDelegate(
			FSourceControlProjectDirDelegate::CreateLambda([this]() {
				return WsInfo.Get().GetPath();
			}));

		// Start background checks
		auto BackgroundStatusDelegate = DiversionTimerDelegate::CreateLambda([this]() {
			if (!DiversionUtils::DiversionValidityCheck(IsInGameThread(),
	"Call Update Status called outside of main thread",
					FDiversionModule::Get().GetOriginalAccountID())) {
				return;
			}

			const auto Operation = ISourceControlOperation::Create<FUpdateStatus>();
			Execute(Operation, nullptr, { WsInfo.Get().GetPath() }, EConcurrency::Asynchronous);
		});
		BackgroundStatus = MakeUnique<FTimedDelegateWrapper>(
			BackgroundStatusDelegate, 5.f);
		BackgroundStatus->Start();

		auto BackgroundPotentialConflictsDelegate = DiversionTimerDelegate::CreateLambda([this]() {
			if (!DiversionUtils::DiversionValidityCheck(IsInGameThread(),
			"Get Potential Conflicts called outside of main thread",
					FDiversionModule::Get().GetOriginalAccountID())) {
				return;
			}

			auto operation = ISourceControlOperation::Create<FGetPotentialConflicts>();
			Execute(operation, nullptr, { WsInfo.Get().GetPath() }, EConcurrency::Asynchronous);
		});
		BackgroundPotentialConflicts = MakeUnique<FTimedDelegateWrapper>(
			BackgroundPotentialConflictsDelegate, 60.f);
		BackgroundPotentialConflicts->Start();
	}

	// TODO bForceConnection: used by commandlets (typically for CI/CD scripts)
}

FString FDiversionProvider::GetBranchId()
{
	return WsInfo.Get().BranchID;
}

bool FDiversionProvider::CheckDiversionAvailability()
{
	if(!DiversionUtils::IsDiversionInstalled())
	{
		DiversionUtils::PrintSupportNeededLogLine("Diversion agent is not installed. Please install the desktop app as documented at https://docs.diversion.dev/quickstart?utm_source=ue-plugin&utm_medium=plugin or contact support if the issue persists.", true);

		return false;
	}

	if (const TSharedPtr<IPlugin> Plugin = FDiversionModule::GetPlugin())
	{
		PluginVersion = Plugin->GetDescriptor().VersionName;
		UE_LOG(LogSourceControl, Log, TEXT("Diversion plugin %s"), *PluginVersion);
	}
	else
	{
		// TODO: Can we get more info here?
		DiversionUtils::PrintSupportNeededLogLine("Failed loading Diversion plugin. Try disabling and re-enabling the plugin (Edit -> Plugins -> Diversion), or reinstalling on the marketplace. Please contact support if the issue persists.");

		return false;
	}

	if(!IsAgentAlive())
	{
		DiversionUtils::PrintSupportNeededLogLine("Diversion agent is not running or properly configured. Check Diversion tray icon or open Diversion Desktop to verify agent is up.", true);

		return false;
	}

	bDiversionAvailable = GetDiversionVersion().IsValid();
	return bDiversionAvailable;
}

void FDiversionProvider::Close()
{

	// clear the cache
	StateCache.Empty();

	bDiversionAvailable = false;
	UserEmail.Empty();

	// Wait for all commands to finish but exit if it takes too long
	// This is to avoid a deadlock when the engine is shutting down	

	auto WaitForCommandsDelegate = WaitForConditionPredicate::CreateLambda([this]() { 
		Tick();
		return CommandQueue.Num() == 0; 
	});
	bool Success = DiversionUtils::WaitForCondition(WaitForCommandsDelegate, 30.0);
	if(!Success)
	{
		UE_LOG(LogSourceControl, Error, TEXT("Failed to fully close Diversion provider, there are still pending commands"));
	}
	UPackage::PackageSavedEvent.RemoveAll(this);

	// Reset localization strings
	FTextLocalizationManager::Get().RefreshResources();
}

FText FDiversionProvider::GetStatusText() const
{
	FString AgentStatus = IsAgentAlive() ? "alive" : "not running";
	FString WsSyncStatus = IsAgentAlive() ? DiversionUtils::GetWsSyncStatusString(SyncStatus.Get()) : "";
	// Combine AgentStatus WsSyncStatus
	FString Status = FString::Printf(TEXT("%s. %s"), *AgentStatus, *WsSyncStatus);

	FFormatNamedArguments Args;
	Args.Add(TEXT("Version"), FText::FromString(GetDiversionVersion().ToString()));
	Args.Add(TEXT("PluginVersion"), FText::FromString(PluginVersion));
	Args.Add(TEXT("AgentAlive"), FText::FromString(Status));
	Args.Add(TEXT("PathToWorkspaceRoot"), FText::FromString(WsInfo.Get().GetPath()));
	Args.Add(TEXT("WorkspaceName"), FText::FromString(GetWorkspaceName()));
	Args.Add(TEXT("RepositoryName"), FText::FromString(WsInfo.Get().RepoName));
	Args.Add(TEXT("BranchName"), FText::FromString(WsInfo.Get().BranchName));

	return FText::Format(NSLOCTEXT("Status", "Provider: Diversion\nEnabledLabel", "Version {Version} (plugin {PluginVersion})\nAgent {AgentAlive}\nWorkspace: {WorkspaceName}\nRepository: {RepositoryName}\nBranch: {BranchName}\n"), Args);
}

TMap<ISourceControlProvider::EStatus, FString> FDiversionProvider::GetStatus() const
{
	TMap<EStatus, FString> Result;
	Result.Add(EStatus::Enabled, IsEnabled() ? TEXT("Yes") : TEXT("No"));
	Result.Add(EStatus::Connected, (IsEnabled() && IsAvailable()) ? TEXT("Yes") : TEXT("No"));
	Result.Add(EStatus::WorkspacePath, WsInfo.Get().GetPath());
	Result.Add(EStatus::Workspace, GetWorkspaceName());
	Result.Add(EStatus::Repository, WsInfo.Get().RepoName);
	Result.Add(EStatus::Branch, WsInfo.Get().BranchName);
	return Result;
}

/** Quick check if version control is enabled */
bool FDiversionProvider::IsEnabled() const
{
	return IsRepoFound();
}

/** Diversion version for feature checking */

const FDiversionVersion& FDiversionProvider::GetDiversionVersion(EConcurrency::Type Concurrency, bool InForceUpdate)
{
	if (!DiversionUtils::DiversionValidityCheck(IsInGameThread(), 
			"GetDiversionVersion called outside of main thread", FDiversionModule::Get().GetOriginalAccountID())) {
		return DvVersion.Get();
	}

	return DvVersion.GetUpdate(DvVersion.Get(),
		FOnCacheUpdate::CreateLambda([this, &Concurrency]() {
			auto operation = ISourceControlOperation::Create<FAgentHealthCheck>();
			Execute(operation, nullptr, TArray<FString>(), Concurrency);
			}), InForceUpdate);
}

const FDiversionVersion& FDiversionProvider::GetDiversionVersion() const 
{
	return DvVersion.Get();
}

WorkspaceInfo FDiversionProvider::GetWsInfo(EConcurrency::Type Concurrency, bool InForceUpdate)
{
	if (!DiversionUtils::DiversionValidityCheck(IsInGameThread(),
		"GetWsInfo called outside of main thread", FDiversionModule::Get().GetOriginalAccountID())) {
		return WsInfo.Get();
	}

	return WsInfo.GetUpdate(
		WsInfo.Get(), FOnCacheUpdate::CreateLambda([this, &Concurrency]() {
			auto operation = ISourceControlOperation::Create<FGetWsInfo>();
			Execute(operation, nullptr, TArray<FString>(), Concurrency);
		}), InForceUpdate);
}

WorkspaceInfo FDiversionProvider::GetWsInfo() const {
	return WsInfo.Get();
}

void FDiversionProvider::SetDvVersion(const FDiversionVersion& InVersion)
{
	if (!DiversionUtils::DiversionValidityCheck(IsInGameThread(),
		"SetDvVersion must be called on the main game thread", FDiversionModule::Get().GetOriginalAccountID())) {
		return;
	}

	DvVersion.Set(InVersion);
}

void FDiversionProvider::SetWorkspaceInfo(const WorkspaceInfo& InWsInfo)
{
	if (!DiversionUtils::DiversionValidityCheck(IsInGameThread(),
		"SetWorkspaceInfo must be called on the main game thread", FDiversionModule::Get().GetOriginalAccountID())) {
		return;
	}

	WsInfo.Set(InWsInfo);
}

void FDiversionProvider::SetSyncStatus(const DiversionUtils::EDiversionWsSyncStatus& InSyncStatus)
{
	if (!DiversionUtils::DiversionValidityCheck(IsInGameThread(),
		"SetSyncStatus must be called on the main game thread", FDiversionModule::Get().GetOriginalAccountID())) {
		return;
	}

	SyncStatus.Set(InSyncStatus);
}

DiversionUtils::EDiversionWsSyncStatus FDiversionProvider::GetSyncStatus() const
{
	return SyncStatus.Get();
}

bool FDiversionProvider::IsRepoFound() const {
	FString RepoId = WsInfo.Get().RepoID;
	return (!RepoId.IsEmpty()) && (RepoId != "N/a");
}

/** Quick check if version control is available for use (useful for server-based providers) */
bool FDiversionProvider::IsAvailable() const
{
	return IsRepoFound() && IsAgentAlive() && 
		((SyncStatus.Get() == DiversionUtils::EDiversionWsSyncStatus::InProgress) ||
		(SyncStatus.Get() == DiversionUtils::EDiversionWsSyncStatus::Completed));
}

const FName& FDiversionProvider::GetName(void) const
{
	return ProviderName;
}

ECommandResult::Type FDiversionProvider::GetState(const TArray<FString>& InFiles, TArray< TSharedRef<ISourceControlState, ESPMode::ThreadSafe> >& OutState, EStateCacheUsage::Type InStateCacheUsage)
{
	if (!IsEnabled())
	{
		return ECommandResult::Failed;
	}

	TArray<FString> AbsoluteFiles = SourceControlHelpers::AbsoluteFilenames(InFiles);

	if (InStateCacheUsage == EStateCacheUsage::ForceUpdate)
	{
		Execute(ISourceControlOperation::Create<FUpdateStatus>(), AbsoluteFiles);
	}

	for (const auto& AbsoluteFile : AbsoluteFiles)
	{
		OutState.Add(GetStateInternal(*AbsoluteFile));
	}

	return ECommandResult::Succeeded;
}

void FDiversionProvider::AddModifiedState(const TSharedRef<class FDiversionState>& InState)
{
	ModifiedStates.Add(InState->LocalFilename, InState);
}

void FDiversionProvider::UpdateModifiedStates(const TMap<FString, TSharedRef<FDiversionState>>& InNewModifiedStates)
{
	for (auto& State : ModifiedStates) {
		if (!InNewModifiedStates.Contains(State.Key)) {
			State.Value->ResetState();
		}
	}
	// Reset the list of modified states only if we requested a full repo status update
	ModifiedStates = InNewModifiedStates;
}

bool FDiversionProvider::UpdateCachedStates(const TMap<FString, FDiversionState>& InNewStates,
	const bool IsFullStatusUpdate, const TMap<FString, FDiversionResolveInfo>& InConflictedFiles)
{
	int NbStatesUpdated = 0;
	TMap<FString, TSharedRef<FDiversionState>> NewModifiedStates;
	
	// Update the local cached states
	for (const auto& [_, NewStateValue] : InNewStates)
	{
		const TSharedRef<FDiversionState> CachedState = GetStateInternal(NewStateValue.LocalFilename);
		CachedState->WorkingCopyState = NewStateValue.WorkingCopyState;
		CachedState->PendingResolveInfo = NewStateValue.PendingResolveInfo;
		CachedState->TimeStamp = NewStateValue.TimeStamp;
		if(NewStateValue.Hash != TEXT(""))
		{
			CachedState->Hash = NewStateValue.Hash;
		}
		NbStatesUpdated++;

		// State handling parts:
		// Used for modified states cache management
		// TODO: better explain this
		if(IsFullStatusUpdate)
		{
			if(CachedState->IsModified())
			{
				// Make sure to add only states that has active modifications
				NewModifiedStates.Add(CachedState->LocalFilename, CachedState);
			}
		}
	}

	// Handles updating the list of modified states and resetting
	// reverted previously modified states.
	// TODO: Fix the bug in UE level - part of AssetRegistry module
	if(IsFullStatusUpdate)
	{
		UpdateModifiedStates(NewModifiedStates);
		// Reset the caching interval timer to maintain the inetrval between calls to the BE 
		// Note - This doesn't affect calls generated by UE!
	}

	// Remove synching from states if necessary
	if (SyncStatus.Get() == DiversionUtils::EDiversionWsSyncStatus::Completed)
	{
		for (auto& State : SynchingStates)
		{
			State.Value->IsSyncing = false;
		}
	}

	// TODO: Separate the handling of conflicted files from the general state update
	// Handle conflicted files
	ConflictedFiles = InConflictedFiles;
	UpdateConflictedStates();

	BackgroundStatusSkipToNextInterval();
	return (NbStatesUpdated > 0);
}

bool FDiversionProvider::UpdatePotentialConflictStates(
	const TMap<FString, TArray<EDiversionPotentialClashInfo>>& InPotentialClashes,
	bool IsFullStatusUpdate)
{
	
	check(IsInGameThread());

	int NbStatesUpdated = 0;

	// Clear potential clashes cache only if we perform a full status update
	if(IsFullStatusUpdate)
	{
		TArray<FString> KeysToRemove;
		// Reset the potential clashes that are no longer in the list
		for(auto& [FileName, State] : PotentialClashesCache)
		{
			if(!InPotentialClashes.Contains(FileName))
			{
				State->PotentialClashes.ResetPotentialClashes();
				KeysToRemove.Add(FileName);
			}
			NbStatesUpdated++;
		}
		
		// Remove the keys that are no longer in the list
		for(const auto& Key : KeysToRemove)
		{
			PotentialClashesCache.Remove(Key);
		}
	}
	
	// Update existing states or add new ones
	for(const auto& [Filename, PotentialClashInfo] : InPotentialClashes)
	{
		if(const auto* CachedState = PotentialClashesCache.Find(Filename); CachedState != nullptr)
		{
			(*CachedState)->PotentialClashes.SetPotentialClashes(PotentialClashInfo);
		}
		else
		{
			TSharedRef<FDiversionState> NewCachedState = GetStateInternal(Filename);
			NewCachedState->PotentialClashes.SetPotentialClashes(PotentialClashInfo);
			PotentialClashesCache.Add(Filename, NewCachedState);
		}
		NbStatesUpdated++;
	}
	
	return (NbStatesUpdated > 0);
}

void FDiversionProvider::UpdateConflictedStates()
{
	for(auto& [FileName, ConflictInfo] : ConflictedFiles)
	{
		const TSharedRef<FDiversionState> CachedState = GetStateInternal(FileName);
		CachedState->WorkingCopyState = EWorkingCopyState::Conflicted;
		CachedState->PendingResolveInfo = ConflictInfo;

		// Add to modified states cache so they will be reseted once merges are resolved or removed
		AddModifiedState(CachedState);
	}
}


void FDiversionProvider::GetCurrentPotentialClashes(TArray<FString>& OutPotentialClashedPaths) const
{
	PotentialClashesCache.GetKeys(OutPotentialClashedPaths);
}

TSharedRef<FDiversionState, ESPMode::ThreadSafe> FDiversionProvider::GetStateInternal(const FString& Filename)
{
	TSharedRef<FDiversionState, ESPMode::ThreadSafe>* State = StateCache.Find(Filename);
	if (State != NULL)
	{
		// found cached item
		return (*State);
	}
	else
	{
		// cache an unknown state for this item
		TSharedRef<FDiversionState, ESPMode::ThreadSafe> NewState = MakeShareable(new FDiversionState(Filename));
		StateCache.Add(Filename, NewState);
		return NewState;
	}
}

void FDiversionProvider::AddSynchingState(const FString& Path, const TSharedRef<class FDiversionState>& InState)
{
	SynchingStates.Add(Path, InState);
}

ECommandResult::Type FDiversionProvider::GetState(const TArray<FSourceControlChangelistRef>& InChangelists, TArray<FSourceControlChangelistStateRef>& OutState, EStateCacheUsage::Type InStateCacheUsage)
{
	return ECommandResult::Failed;
}

bool IsConfigFile(const FString& FilePath)
{
	// Check if the file has an .ini extension
	if (!FilePath.EndsWith(TEXT(".ini")))
	{
		return false;
	}

	// Get the path to the project's Config directory
	FString ConfigDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());

	// Check if FilePath starts with the project's Config directory path
	if (FilePath.StartsWith(ConfigDir))
	{
		return true;
	}

	return false;
}

TArray<FSourceControlStateRef> FDiversionProvider::GetCachedStateByPredicate(TFunctionRef<bool(const FSourceControlStateRef&)> Predicate) const
{
	TArray<FSourceControlStateRef> Result;
	for (const auto& CacheItem : StateCache)
	{
		FSourceControlStateRef State = CacheItem.Value;
		
		// Ignore configuration files states, since they are added already by the SCC
		if (Predicate(State) && !IsConfigFile(State->GetFilename()))
		{
			Result.Add(State);
		}
	}
	return Result;
}

bool FDiversionProvider::RemoveFileFromCache(const FString& Filename)
{
	return StateCache.Remove(Filename) > 0;
}


FDelegateHandle FDiversionProvider::RegisterSourceControlStateChanged_Handle(const FSourceControlStateChanged::FDelegate& SourceControlStateChanged)
{
	return OnSourceControlStateChanged.Add(SourceControlStateChanged);
}

void FDiversionProvider::UnregisterSourceControlStateChanged_Handle(FDelegateHandle Handle)
{
	OnSourceControlStateChanged.Remove(Handle);
}

ECommandResult::Type FDiversionProvider::Execute(const FSourceControlOperationRef& InOperation, FSourceControlChangelistPtr InChangelist, const TArray<FString>& InFiles, EConcurrency::Type InConcurrency, const FSourceControlOperationComplete& InOperationCompleteDelegate)
{
	// Workers/Commands must not call to other workers/commands and to this command directly!
	// Workers should call directly to the SyncAPI methods and update the provider state accordingly
	if (!DiversionUtils::DiversionValidityCheck(IsInGameThread(),
		"Provider Execute called outside of main thread", FDiversionModule::Get().GetOriginalAccountID())) {
		return ECommandResult::Failed;
	}

	if (!IsEnabled() && !(CanExecuteBeforeEnabled.Contains(InOperation->GetName())))
	{
		// Note that IsEnabled() always returns true so unless it is changed, this code will never be executed
		InOperationCompleteDelegate.ExecuteIfBound(InOperation, ECommandResult::Failed);
		return ECommandResult::Failed;
	}

	TArray<FString> AbsoluteFiles = SourceControlHelpers::AbsoluteFilenames(InFiles);

	// Query to see if we allow this operation
	TSharedPtr<IDiversionWorker, ESPMode::ThreadSafe> Worker = CreateWorker(InOperation->GetName());
	if (!Worker.IsValid())
	{
		// this operation is unsupported by this version control provider
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("OperationName"), FText::FromName(InOperation->GetName()));
		Arguments.Add(TEXT("ProviderName"), FText::FromName(GetName()));
		FText Message(FText::Format(LOCTEXT("UnsupportedOperation", "Operation '{OperationName}' not supported by revision control provider '{ProviderName}'"), Arguments));
		FMessageLog("SourceControl").Error(Message);
		InOperation->AddErrorMessge(Message);

		InOperationCompleteDelegate.ExecuteIfBound(InOperation, ECommandResult::Failed);
		return ECommandResult::Failed;
	}

	FDiversionCommand* Command = new FDiversionCommand(InOperation, Worker.ToSharedRef(), InConcurrency);
	Command->Files = AbsoluteFiles;
	Command->OperationCompleteDelegate = InOperationCompleteDelegate;

	// fire off operation
	if (InConcurrency == EConcurrency::Synchronous)
	{
		Command->bAutoDelete = false;
		return ExecuteSynchronousCommand(*Command, InOperation->GetInProgressString());
	}
	else
	{
		Command->bAutoDelete = true;
		return IssueCommand(*Command);
	}
}

bool FDiversionProvider::CanExecuteOperation(const FSourceControlOperationRef& InOperation) const
{
	return WorkersMap.Find(InOperation->GetName()) != nullptr;
}

bool FDiversionProvider::CanCancelOperation(const FSourceControlOperationRef& InOperation) const
{
	return false;
}

void FDiversionProvider::CancelOperation(const FSourceControlOperationRef& InOperation)
{
}

bool FDiversionProvider::UsesLocalReadOnlyState() const
{
	return false;
}

bool FDiversionProvider::UsesChangelists() const
{
	return false;
}

bool FDiversionProvider::UsesUncontrolledChangelists() const
{
	return true;
}

bool FDiversionProvider::UsesCheckout() const
{
	return false;
}

bool FDiversionProvider::UsesFileRevisions() const
{
	return false;
}

bool FDiversionProvider::UsesSnapshots() const
{
	return false;
}

bool FDiversionProvider::AllowsDiffAgainstDepot() const
{
	return true;
}

TOptional<bool> FDiversionProvider::IsAtLatestRevision() const
{
	return TOptional<bool>();
}

TOptional<int> FDiversionProvider::GetNumLocalChanges() const
{
	if(GetSyncStatus() == DiversionUtils::EDiversionWsSyncStatus::Completed)
	{
		return TOptional<int>(ModifiedStates.Num());
	} else
	{
		return TOptional<int>(0);
	}
}

TSharedPtr<IDiversionWorker, ESPMode::ThreadSafe> FDiversionProvider::CreateWorker(const FName& InOperationName) const
{
	const FGetDiversionWorker* Operation = WorkersMap.Find(InOperationName);
	if (Operation != nullptr)
	{
		return Operation->Execute();
	}

	return nullptr;
}

void FDiversionProvider::RegisterWorker(const FName& InName, const FGetDiversionWorker& InDelegate)
{
	WorkersMap.Add(InName, InDelegate);
}

void FDiversionProvider::OutputCommandMessages(const FDiversionCommand& InCommand) const
{
	FMessageLog SourceControlLog("SourceControl");

	for (int32 ErrorIndex = 0; ErrorIndex < InCommand.ErrorMessages.Num(); ++ErrorIndex)
	{
		SourceControlLog.Error(FText::FromString(InCommand.ErrorMessages[ErrorIndex]));
	}

	for (int32 InfoIndex = 0; InfoIndex < InCommand.InfoMessages.Num(); ++InfoIndex)
	{
		SourceControlLog.Info(FText::FromString(InCommand.InfoMessages[InfoIndex]));
	}
}

void FDiversionProvider::Tick()
{
	if (bDiversionAvailable) {
		// Revalidate the agent status and WsInfo
		IsAgentAlive(EConcurrency::Asynchronous, ReloadStatusRequired);
		GetWsInfo(EConcurrency::Asynchronous, ReloadStatusRequired);
		if(ReloadStatusRequired)
		{
			ReloadStatusRequired = false;
		}
	}

	DiversionUtils::HttpTick(LastHttpTickTime);
	bool bStatesUpdated = false;
	for (int32 CommandIndex = 0; CommandIndex < CommandQueue.Num(); ++CommandIndex)
	{
		FDiversionCommand& Command = *CommandQueue[CommandIndex];
		if (Command.bExecuteProcessed)
		{
			// Remove command from the queue
			CommandQueue.RemoveAt(CommandIndex);

			// let command update the states of any files
			bStatesUpdated |= Command.Worker->UpdateStates();

			// dump any messages to output log
			OutputCommandMessages(Command);

			Command.ReturnResults();

			// commands that are left in the array during a tick need to be deleted
			if (Command.bAutoDelete)
			{
				// Only delete commands that are not running 'synchronously'
				delete &Command;
			}

			// only do one command per tick loop, as we don't want concurrent modification
			// of the command queue (which can happen in the completion delegate)
			break;
		}
	}

	if (bStatesUpdated)
	{
		OnSourceControlStateChanged.Broadcast();
	}
}

TArray< TSharedRef<ISourceControlLabel> > FDiversionProvider::GetLabels(const FString& InMatchingSpec) const
{
	TArray< TSharedRef<ISourceControlLabel> > Tags;

	return Tags;
}

TArray<FSourceControlChangelistRef> FDiversionProvider::GetChangelists(EStateCacheUsage::Type InStateCacheUsage)
{
	return TArray<FSourceControlChangelistRef>();
}

#if SOURCE_CONTROL_WITH_SLATE
TSharedRef<class SWidget> FDiversionProvider::MakeSettingsWidget() const
{
	return SNew(SDiversionSettings);
}
#endif

ECommandResult::Type FDiversionProvider::ExecuteSynchronousCommand(FDiversionCommand& InCommand, const FText& Task)
{
	ECommandResult::Type Result = ECommandResult::Failed;

	// Display the progress dialog if a string was provided
	{
		FScopedSourceControlProgress Progress(Task);

		// Issue the command asynchronously...
		IssueCommand(InCommand);

		// ... then wait for its completion (thus making it synchronous)
		while (!InCommand.bExecuteProcessed)
		{
			// Tick the command queue and update progress.
			Tick();

			Progress.Tick();

			// Sleep for a bit so we don't busy-wait so much.
			FPlatformProcess::Sleep(0.01f);
		}

		// always do one more Tick() to make sure the command queue is cleaned up.
		Tick();

		if (InCommand.bCommandSuccessful)
		{
			Result = ECommandResult::Succeeded;
		}
	}

	// Delete the command now (asynchronous commands are deleted in the Tick() method)
	if (!DiversionUtils::DiversionValidityCheck(!InCommand.bAutoDelete,
		"InCommand was configures to execute synchronously with auto delete on", WsInfo.Get().AccountID)) {
		return Result;
	}

	// ensure commands that are not auto deleted do not end up in the command queue
	if (CommandQueue.Contains(&InCommand))
	{
		CommandQueue.Remove(&InCommand);
	}
	delete &InCommand;

	return Result;
}

ECommandResult::Type FDiversionProvider::IssueCommand(FDiversionCommand& InCommand)
{
	if (GThreadPool != nullptr)
	{
		// Queue this to our worker thread(s) for resolving
		GThreadPool->AddQueuedWork(&InCommand);
		CommandQueue.Add(&InCommand);
		return ECommandResult::Succeeded;
	}
	else
	{
		FText Message(LOCTEXT("NoSCCThreads", "There are no threads available to process the revision control command."));

		FMessageLog("SourceControl").Error(Message);
		InCommand.bCommandSuccessful = false;
		InCommand.Operation->AddErrorMessge(Message);

		return InCommand.ReturnResults();
	}
}

void FDiversionProvider::OnPackageSave(const FString& PackageFileName, UObject* Outer)
{
	const FString FileName = FPaths::ConvertRelativePathToFull(PackageFileName);
	if (const auto CurrentResolveInfo = FilesToResolve.Find(FileName))
	{
		const auto MergeId = CurrentResolveInfo->MergeId;

		auto ResolveFileOperation = ISourceControlOperation::Create<FResolveFile>();
		ResolveFileOperation->SetMergeID(MergeId);
		ResolveFileOperation->SetConflictID(CurrentResolveInfo->ConflictId);
		Execute(ResolveFileOperation, nullptr, {FileName}, EConcurrency::Synchronous);

		FilesToResolve.Remove(FileName);
		// We will revert the change only after finalizing the merge to avoid losing the changes
		FilesToRevert.Add(FileName);

		if (ConflictedFiles.Num() + FilesToResolve.Num() == 0)
		{
			// Run finalize merge and wait for it to finish
			auto FinalizeMergeOperation = ISourceControlOperation::Create<FFinalizeMerge>();
			FinalizeMergeOperation->SetMergeID(MergeId);
			Execute(FinalizeMergeOperation, nullptr, {}, EConcurrency::Synchronous);
			
			// TODO: What should we do if the Finalize merge failed?

			// Reset the workspace to avoid showing wrong changes - We committed them in the merge already
			Execute(ISourceControlOperation::Create<FRevert>(), nullptr, 
				FilesToRevert, EConcurrency::Synchronous);
		}
	}
}

#undef LOCTEXT_NAMESPACE
