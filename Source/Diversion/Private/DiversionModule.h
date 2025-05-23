// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "DiversionSettings.h"
#include "DiversionProvider.h"
#include "DiversionUtils.h"

#include "ISourceControlModule.h"
#include "DiversionCredentialsManager.h"
#include "DiversionHttpManager.h"

// API headers
#include "DefaultApi.h"
#include "SupportApi.h"
#include "AnalyticsApi.h"
#include "RepositoryManipulationApi.h"
#include "RepositoryMergeManipulationApi.h"
#include "RepositoryCommitManipulationApi.h"
#include "RepositoryWorkspaceManipulationApi.h"
#include "RepositoryManagementApi.h"

/** Diversion Version Control Plugin for Unreal Engine */
class FDiversionModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Access the Diversion version control settings */
	FDiversionSettings& AccessSettings()
	{
		return DiversionSettings;
	}

	/** Save the Diversion version control settings */
	void SaveSettings();

	/** Access the Diversion version control provider */
	FDiversionProvider& GetProvider()
	{
		// Other threads MUST NOT interact with the provider directly
		check(IsInGameThread());
		return DiversionProvider;
	}

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, asserts if the module is not loaded yet or unloaded already.
	 */
	static FDiversionModule& Get();

	/**
	 * Checks whether the module is currently loaded.
	 */
	static bool IsLoaded();

	/**
	 * Finds information of the plugin.
	 *
	 * @return	 Pointer to the plugin's information, or nullptr.
	 */
	static const TSharedPtr<class IPlugin> GetPlugin();

	inline static FString GetAppName()
	{
		return "Plugin-Unreal";
	}

	static FString GetPluginVersion();

	FString GetAccessToken(FString InUserID);


	FString GetOriginalAccountID() const
	{
		return OriginalAccountID;
	}

	void SetOriginalAccountID(const FString& InAccountID)
	{
		OriginalAccountID = InAccountID;
	}

private:
	/** Event handler for asset editor window opening */
	void HandleAssetOpenedInEditor(UObject* Asset);
	bool PotentialConflictExistForAsset(UObject* Asset);
	void ShowPotentialConflictConfirmationDialog(UObject* Asset);
	
	/* Stores list of assets paths that are scheduled for re-opening as a readonly window. 
	 * Used to filter out the "re-opening" asset editor window events.
	**/
	TSet<FString> OpenedEditorAssets;

private:
	/** The Diversion version control provider */
	FDiversionProvider DiversionProvider;

	/** The settings for Diversion version control */
	FDiversionSettings DiversionSettings;

	FCredentialsManager CredManager;

	// Storing long-lived APICalls
	TMap<FString, TSharedPtr<void>> ApiCallCache;
	mutable TUniquePtr<FRWLock> ServiceStatusLock;

	/** The account ID with which the current plugin session was initiated */
	FString OriginalAccountID;

	/** Saved global state of the extra confirmation checkbox when trying to open a potentially conflicted file */
	ECheckBoxState bConflictedFileOpenConfirmCheckboxState = ECheckBoxState::Unchecked;

public:

// API Request Managers
	TSharedPtr<DiversionHttp::FHttpRequestManager> CoreAPIClient; // Used to perform download file from URL

	TUniquePtr<Diversion::AgentAPI::DefaultApi> AgentAPIRequestManager;
	TUniquePtr<Diversion::CoreAPI::SupportApi> SupportAPIRequestManager;
	TUniquePtr<Diversion::CoreAPI::AnalyticsApi> AnalyticsAPIRequestManager;
	TUniquePtr<Diversion::CoreAPI::RepositoryManagementApi> RepositoryManagementAPIRequestManager;
	TUniquePtr<Diversion::CoreAPI::RepositoryManipulationApi> RepositoryManipulationAPIRequestManager;
	TUniquePtr<Diversion::CoreAPI::RepositoryMergeManipulationApi> RepositoryMergeManipulationAPIRequestManager;
	TUniquePtr<Diversion::CoreAPI::RepositoryCommitManipulationApi> RepositoryCommitManipulationAPIRequestManager;
	TUniquePtr<Diversion::CoreAPI::RepositoryWorkspaceManipulationApi> RepositoryWorkspaceManipulationAPIRequestManager;
};
