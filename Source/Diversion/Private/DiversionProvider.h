// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#pragma once
#include "DiversionState.h"
#include "DiversionWorkspaceInfo.h"
#include "ISourceControlProvider.h"
#include "IDiversionWorker.h"
#include "DiversionVersion.h"
#include "CachedState.h"
#include "DiversionTimedDelegate.h"

class FDiversionState;

class FDiversionCommand;

DECLARE_DELEGATE_RetVal(FDiversionWorkerRef, FGetDiversionWorker)


class FDiversionProvider : public ISourceControlProvider
{
public:
	FDiversionProvider() : LastHttpTickTime(0),
	                       DvVersion(FDiversionVersion(""), FTimespan::FromSeconds(1)),
	                       WsInfo(WorkspaceInfo(), FTimespan::FromSeconds(1)),
	                       SyncStatus(DiversionUtils::EDiversionWsSyncStatus::Paused, FTimespan::FromSeconds(1))
	{
	}

	/* ISourceControlProvider implementation */
	virtual void Init(bool bForceConnection = true) override;
	virtual void Close() override;
	virtual FText GetStatusText() const override;
	virtual TMap<EStatus, FString> GetStatus() const override;
	virtual bool IsEnabled() const override;
	virtual bool IsAvailable() const override;
	virtual const FName& GetName(void) const override;
	virtual bool QueryStateBranchConfig(const FString& ConfigSrc, const FString& ConfigDest) override { return false; }
	virtual void RegisterStateBranches(const TArray<FString>& BranchNames, const FString& ContentRoot) override {}
	virtual int32 GetStateBranchIndex(const FString& InBranchName) const override { return INDEX_NONE; }
	virtual ECommandResult::Type GetState(const TArray<FString>& InFiles, TArray<FSourceControlStateRef>& OutState, EStateCacheUsage::Type InStateCacheUsage) override;
	virtual ECommandResult::Type GetState(const TArray<FSourceControlChangelistRef>& InChangelists, TArray<FSourceControlChangelistStateRef>& OutState, EStateCacheUsage::Type InStateCacheUsage) override;
	virtual TArray<FSourceControlStateRef> GetCachedStateByPredicate(TFunctionRef<bool(const FSourceControlStateRef&)> Predicate) const override;
	virtual FDelegateHandle RegisterSourceControlStateChanged_Handle(const FSourceControlStateChanged::FDelegate& SourceControlStateChanged) override;
	virtual void UnregisterSourceControlStateChanged_Handle(FDelegateHandle Handle) override;
	virtual ECommandResult::Type Execute(const FSourceControlOperationRef& InOperation, FSourceControlChangelistPtr InChangelist, const TArray<FString>& InFiles, EConcurrency::Type InConcurrency = EConcurrency::Synchronous, const FSourceControlOperationComplete& InOperationCompleteDelegate = FSourceControlOperationComplete()) override;
	virtual bool CanExecuteOperation(const FSourceControlOperationRef& InOperation) const override;
	virtual bool CanCancelOperation(const FSourceControlOperationRef& InOperation) const override;
	virtual void CancelOperation(const FSourceControlOperationRef& InOperation) override;
	virtual bool UsesLocalReadOnlyState() const override;
	virtual bool UsesChangelists() const override;
	virtual bool UsesUncontrolledChangelists() const override;
	virtual bool UsesCheckout() const override;
	virtual bool UsesFileRevisions() const override;
	virtual bool UsesSnapshots() const override;
	virtual bool AllowsDiffAgainstDepot() const override;
	virtual TOptional<bool> IsAtLatestRevision() const override;
	virtual TOptional<int> GetNumLocalChanges() const override;
	virtual void Tick() override;
	virtual TArray< TSharedRef<class ISourceControlLabel> > GetLabels(const FString& InMatchingSpec) const override;
	virtual TArray<FSourceControlChangelistRef> GetChangelists(EStateCacheUsage::Type InStateCacheUsage) override;
#if SOURCE_CONTROL_WITH_SLATE
	virtual TSharedRef<class SWidget> MakeSettingsWidget() const override;
	FString GetBranchId();
#endif

	using ISourceControlProvider::Execute;

	/**
	 * Check configuration, else standard paths, and run a Diversion "version" command to check the availability of the binary.
	 */
	bool CheckDiversionAvailability();

	/** Diversion version for feature checking */
	const FDiversionVersion& GetDiversionVersion(EConcurrency::Type Concurrency, bool InForceUpdate);
	const FDiversionVersion& GetDiversionVersion() const;

	bool IsAgentAlive(EConcurrency::Type Concurrency, bool InForceUpdate){
		return GetDiversionVersion(Concurrency, InForceUpdate).IsValid();
	}
	bool IsAgentAlive() const { return GetDiversionVersion().IsValid(); }


	WorkspaceInfo GetWsInfo(EConcurrency::Type Concurrency, bool InForceUpdate);
	WorkspaceInfo GetWsInfo() const;

	void SetDvVersion(const FDiversionVersion& InVersion);

	void SetWorkspaceInfo(const WorkspaceInfo& InWsInfo);

	DiversionUtils::EDiversionWsSyncStatus GetSyncStatus() const;
	void SetSyncStatus(const DiversionUtils::EDiversionWsSyncStatus& InSyncStatus);

	void SetReloadStatus() {
		ReloadStatusRequired = true;
	}

	const FString& GetPluginVersion() const
	{
		return PluginVersion;
	}

	bool IsRepoFound() const;

	/** Get the path to the root of the Diversion repository: can be the ProjectDir itself, or any parent directory */
	FString GetPathToWorkspaceRoot() const
	{
		return WsInfo.Get().GetPath();
	}

	/** Diversion config user.name */
	FString GetUserEmail() const
	{
		return WsInfo.Get().AccountID;
	}

	FString GetWorkspaceName() const
	{
		return WsInfo.Get().WorkspaceID;
	}

	FString GetRepositoryName() const
	{
		return WsInfo.Get().RepoName;
	}

	/**
	 * Register a worker with the provider.
	 * This is used internally so the provider can maintain a map of all available operations.
	 */
	void RegisterWorker(const FName& InName, const FGetDiversionWorker& InDelegate);

	/** Remove a named file from the state cache */
	bool RemoveFileFromCache(const FString& Filename);

	FString GetRepositoryId() const
	{
		return WsInfo.Get().RepoID;
	}

	FString GetWorkspacId() const
	{
		return WsInfo.Get().WorkspaceID;
	}



private:

	/** Is dv binary found and working. */
	bool bDiversionAvailable = false;

	/** Helper function for Execute() */
	TSharedPtr<class IDiversionWorker, ESPMode::ThreadSafe> CreateWorker(const FName& InOperationName) const;

	/** Helper function for running command synchronously. */
	ECommandResult::Type ExecuteSynchronousCommand(class FDiversionCommand& InCommand, const FText& Task);
	/** Issue a command asynchronously if possible. */
	ECommandResult::Type IssueCommand(class FDiversionCommand& InCommand);

	/** Output any messages this command holds */
	void OutputCommandMessages(const class FDiversionCommand& InCommand) const;

	/** Version of the Diversion plugin */
	FString PluginVersion;

	/** Diversion user email */
	FString UserEmail;

	bool ReloadStatusRequired = false;

	/** The currently registered version control operations */
	TMap<FName, FGetDiversionWorker> WorkersMap;

	/** Queue for commands given by the main thread */
	TArray<FDiversionCommand*> CommandQueue;

	/** For notifying when the version control states in the cache have changed */
	FSourceControlStateChanged OnSourceControlStateChanged;

	/** Time of the last HTTP manager tick call*/
	double LastHttpTickTime;

	TCached<FDiversionVersion> DvVersion;
	TCached<WorkspaceInfo> WsInfo;
	TCached<DiversionUtils::EDiversionWsSyncStatus> SyncStatus;
	
	const TArray<FString> CanExecuteBeforeEnabled = {
		"Connect",
		"AgentHealthCheck",
		"GetWsInfo",
		"InitRepo",
		"SendAnalytics"
	};

public:
	TMap<FString, FDiversionResolveInfo> ConflictedFiles;
	TMap<FString, FDiversionResolveInfo> FilesToResolve;
	TArray<FString> FilesToRevert;
	void OnPackageSave(const FString& PackageFileName, UObject* Outer);

#pragma region StatesCache
public:
	
	/**
	 * Helper function for various commands to update cached states.
	 * @param InNewStates - new states to update the cache with
	 * @param IsFullStatusUpdate - if true, this will clear the modified states cache
	 * @param InConflictedFiles - Conflicted paths to update the states of
	 * @returns true if any states were updated
	 */
	bool UpdateCachedStates(const TMap<FString, FDiversionState>& InNewStates, bool IsFullStatusUpdate,
	const TMap<FString, FDiversionResolveInfo>& InConflictedFiles);
	
	/**
	 * Helper function for various commands to update states with their potential clashes status.
	 * @param InPotentialClashes - Potential clashes to update the states with
	 *                             Will remove the potential clash info from local cache if its empty (Num==0).
	 * @returns true if any states were updated
	 */
	bool UpdatePotentialConflictStates(const TMap<FString, TArray<EDiversionPotentialClashInfo>>& InPotentialClashes,
		bool IsFullStatusUpdate);

	// This is currently being updated by the local conflicted files list
	void UpdateConflictedStates();
	
	void GetCurrentPotentialClashes(TArray<FString>& OutPotentialClashedPaths) const;
	
	/** Helper function used to update state cache */
	TSharedRef<FDiversionState, ESPMode::ThreadSafe> GetStateInternal(const FString& Filename);

	/** Adds a state to the synching states cache */
	void AddSynchingState(const FString& Path, const TSharedRef<class FDiversionState>& InState);

// Background status triggering functions
	// Use when want to mark BG status as called and wait for next interval
	void BackgroundStatusSkipToNextInterval()
	{
		BackgroundStatus->SkipToNextInterval();
	}
	// Force trigger calling BG status on the next tick
	void BackgroundStatusTriggerInstantCall()
	{
		BackgroundStatus->TriggerInstantCallAndReset();
	}
//
	
private:
	/** Helper function for UpdateCachedStates. Updates the modified states cache.
	 * @param InNewModifiedStates - new states to update the cache with
	 */
	void UpdateModifiedStates(const TMap<FString, TSharedRef<FDiversionState>>& InNewModifiedStates);

	/** Adds a state to the modified states cache */
	void AddModifiedState(const TSharedRef<class FDiversionState>& InState);
	
	/** State cache */
	TMap<FString, TSharedRef<FDiversionState>> StateCache;
	/** Tracking modified states - enables resetting the changes to apply external to UE changes too */
	TMap<FString, TSharedRef<FDiversionState>> ModifiedStates;
	/** Tracking potential clashes - this used to keep track of resolved potential clashes*/
	TMap<FString, TSharedRef<FDiversionState>> PotentialClashesCache;
	/** Tracking synching states - enables restting them once finished synching */
	TMap<FString, TSharedRef<FDiversionState>> SynchingStates;

// Background status triggering variables
	TUniquePtr<FTimedDelegateWrapper> BackgroundStatus = nullptr;
	TUniquePtr<FTimedDelegateWrapper> BackgroundPotentialConflicts = nullptr;
//
#pragma endregion

};
