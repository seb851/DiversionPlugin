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

template<typename T>
struct TypeParseTraits;

#define REGISTER_PARSE_TYPE(X) template <> struct TypeParseTraits<X> \
{ static const char* name; } ; const char* TypeParseTraits<X>::name = #X


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

bool HttpTick(double& LastHttpTickTime);

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

void WaitForHttpRequest(const double InMaxWaitingTimeoutSeconds, const FHttpRequestPtr InRequest, const FString& InAPICallName, const double InLogIntervalSeconds);

FString GetStackTrace();

bool DiversionValidityCheck(bool InCondition, FString InErrorDescription, const FString& AccountId, bool InSilent = false);

// API calls 
bool GetWorkspaceConfigByPath(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InPath);
bool SendAnalyticsEvent(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const TMap<
                        FString, FString>& InProperties);
bool RunAgentHealthCheck(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool AgentInSync(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool GetConflictedFiles(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool RunGetMerges(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
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
bool WorkspaceSyncProgress(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);
bool NotifyAgentSyncRequired(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages);

// Since this API doesn't affect the state of the Provider and doesn't return any value, it can live without the context of a FDiversionCommand
bool SendErrorToBE(const FString& AccountID, const FString& InErrorMessageToReport, const FString& InStackTrace);
} // namespace DiversionUtils
