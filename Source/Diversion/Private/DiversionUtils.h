// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#pragma once

// TODO: Use env variables or external configuration file?
#ifndef DIVERSION_BINARY_NAME 
	#define DIVERSION_BINARY_NAME TEXT("dv")
	#define DIVERSION_AGENT_URL TEXT("http://localhost:8797")
		#define DIVERSION_CREDENTIALS_FOLDER TEXT("credentials")
#endif

#include "HttpModule.h"
#include "DiversionState.h"
#include "CoreAPI/Public/Merge.h"



const FString DIVERSION_API_HOST = TEXT("api.diversion.dev");
const FString DIVERSION_API_PORT = TEXT("443");

const FString AGENT_API_HOST = TEXT("127.0.0.1");
const FString AGENT_API_PORT = TEXT("8797");


class FDiversionState;
class FDiversionCommand;
struct FDiversionVersion;
struct WorkspaceInfo;

DECLARE_DELEGATE_RetVal(bool, WaitForConditionPredicate)

namespace DiversionUtils
{
	namespace UPackageUtils {
		/**
		* Get the package object from a file absolute path
		* @param InPath The absolute path to get the package name from
		* @returns the package object or nullptr if the package was not loaded
		*/
		UPackage* PackageFromPath(const FString& InPath);

		/**
		* Check if a package is opened in the editor
		* @param PackageName The name of the package to check
		* @returns true if the package is opened in the editor
		*/
		bool IsPackageOpenedInEditor(const FString& PackageName);

		/**
		 * Wrap UE package saving mechanism
		 * @param Package 
		 * @return true if the package was saved successfully
		 */
		bool SavePackage(UPackage* Package);

		/**
		 * Closing all editor tabs for a package
		 * @param Package 
		 */
		void ClosePackageEditor(UPackage* Package);

		/**
		 * Dicard all unsaved changes for a package
		 * @param Package 
		 */
		void DiscardUnsavedChanges(UPackage* Package);

		bool BackupUnsavedChanges(UPackage* Package);
		
		bool MakeSimpleOSCopyPackageBackup(const FString& OriginalFileFullPath, const FString& TargetBackupFolderPath);
		
	}

enum class EDiversionWsSyncStatus {
	Unknown,
	PathError,
	Paused,
	InProgress,
	Completed
};

FString GetWsSyncStatusString(EDiversionWsSyncStatus InStatus);

/**
* Printing common support referencing log line
* @param InMessage The message to print in addition to the support links
* @param IsWarning If true, the log line will be a warning
*/
void PrintSupportNeededLogLine(const FString& InMessage, bool IsWarning = false);

void LoadWorkspaceInfo(FString InWorkingDir, WorkspaceInfo& OutInfo);

/**
* Starts the diversion agent process
* @param InPathToDiversionBinary The path to the diversion binary
* @returns true if the agent was started successfully
*/
bool StartDiversionAgent(const FString& InPathToDiversionBinary);

/**
 * Get the workspace revision number from a commit ID
 * @param CommitID The commit ID to parse
 * @returns the workspace revision number
 */
int GetWorkspaceRevisionByCommit(const FString& CommitID);

/**
 * Helper function to wait for a condition to be true.
 * @note This function will block the calling thread until the condition is true or the timeout is reached.
 * @param Condition The condition to wait for
 * @param Timeout The maximum time to wait for the condition to be true
 * @param InWaitPollInterval The interval to wait between condition checks
 * @returns true if the condition was true before the timeout
 */
bool WaitForCondition(const WaitForConditionPredicate& Condition, float Timeout, float InWaitPollInterval = 0.1f);

FString GetUserHomeDirectory();

FString ConvertFullPathToRelative(const FString& InPath, FString RootDir = "");

FString ConvertRelativePathToDiversionFull(const FString& InPath, FString RootDir = "");

FString RefToOrdinalId(const FString& RefId);

void ShowErrorNotification(const FText& ErrorMessage);

bool IsDiversionInstalled();

FString GetFilePathFromAssetData(const FAssetData& AssetData);

/**
 * Extract common prefixes from the given paths list and removes duplicates.
 * Inclusive for folders path as well.
 * @param InPaths The paths to extract common prefixes from. Pass 0 for no limit.
 * @param InBaseRepoPath The base path to extract the common prefixes from.
 * @param InLimit The maximum number of prefixes to return.
 * @returns Set containing the common prefixes as relative paths to the repo root provided.
 */
TSet<FString> GetPathsCommonPrefixes(const TArray<FString>& InPaths, const FString& InBaseRepoPath, int32 InLimit = 0);

FString FindCommonAncestorDirectory(const TArray<FString>& InPaths);

FString GetStackTrace();

bool DiversionValidityCheck(bool InCondition, FString InErrorDescription, const FString& AccountId, bool InSilent = false);

#define AUTHORIZATION_HEADER_KEY "Authorization"
#define APP_VERSION_HEADER_KEY "X-DV-App-Version"
#define APP_NAME_HEADER_KEY "X-DV-App-Name"
#define CORRELATION_ID_HEADER_KEY "X-Sentry-Correlation-ID"
	
TMap<FString, FString> GetDiversionHeaders();
	
// API calls 
bool GetWorkspaceConfigByPath(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InPath);
bool SendAnalyticsEvent(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const TMap<
                        FString, FString>& InProperties);
bool RunAgentHealthCheck(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool AgentInSync(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool GetConflictedFiles(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages,
	TArray<FString>& OutErrorMessages, TMap<FString, FDiversionResolveInfo>& OutConflicts,
	TArray<Diversion::CoreAPI::Model::Merge>& OutWorkspaceMergesList, TArray<Diversion::CoreAPI::Model::Merge>& OutBranchMergesList);
bool RunGetMerges(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages,
	TArray<FString>& OutErrorMessages, TArray<Diversion::CoreAPI::Model::Merge>& OutMerges);
bool RunUpdateStatus(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, bool WaitForSync = true);
bool RunCommit(FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InDescription, bool WaitForSync = true);
bool RunReset(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool WaitForAgentSync(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, float InSecondsToTimeout = 10.f);
bool RunGetHistory(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InFile, 
	const FString* MergeFromRef);
bool GetWsBlobInfo(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InFile);
bool RunResolvePath(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, 
	const FString& InMergeId, const FString& InConflictId, bool WaitForSync = true);
bool RunFinalizeMerge(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InMergeId);
// Exceptions to the API calls due to being executed irregularily by the engine
bool DownloadFileFromURL(const FString& Url, const FString& SavePath);
bool DownloadBlob(TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InRefId, const FString& InOutputFilePath, const FString& InFilePath, WorkspaceInfo InWsInfo);
bool RunRepoInit(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InRepoRootPath, const FString& InRepoName);
bool GetPotentialFileClashes(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, TMap<FString, TArray<EDiversionPotentialClashInfo>>& OutPotentialClashes, bool& OutRecurseCall);
bool GetWorkspaceSyncProgress(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool NotifyAgentSyncRequired(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool UpdateWorkspace(FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool GetRemoteRepos(FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, bool InOwnedOnly, TArray<FString>& OutReposList);
bool GetAllLocalWorkspaces(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, TArray<WorkspaceInfo>& OutLocalWorkspaces);

// Since this API doesn't affect the state of the Provider and doesn't return any value, it can live without the context of a FDiversionCommand
bool SendErrorToBE(const FString& AccountID, const FString& InErrorMessageToReport, const FString& InStackTrace);
} // namespace DiversionUtils
