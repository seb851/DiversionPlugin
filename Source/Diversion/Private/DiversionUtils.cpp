// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "DiversionState.h"
#include "DiversionSettings.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/Paths.h"
#include "ISourceControlModule.h"
#include "DiversionModule.h"
#include "TimerManager.h"
#include "PackageTools.h"

#include "HttpModule.h"
#include "HttpManager.h"

#include "AssetRegistry/AssetRegistryModule.h"

#pragma region Generated REST API headers
#include "OpenAPIDefaultApi.h"
#include "OpenAPIDefaultApiOperations.h"

#pragma endregion

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shlobj.h>
#endif

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformFileManager.h"


#if PLATFORM_LINUX
#endif
#include "DiversionOperations.h"


namespace DiversionConstants
{
}

namespace DiversionUtils
{

	namespace UPackageUtils {

		bool IsPackageOpenedInEditor(const FString& PackageName)
		{
			if (GEditor)
			{
				UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
				if (AssetEditorSubsystem)
				{
					TArray<UObject*> OpenedAssets = AssetEditorSubsystem->GetAllEditedAssets();
					for (UObject* Asset : OpenedAssets)
					{
						if (Asset && Asset->GetOutermost()->GetName() == PackageName)
						{
							return true;
						}
					}
				}
			}
			return false;
		}

		UPackage* PackageFromPath(const FString& InPath)
		{
			FString PackageName;
			UPackage* Package;
			FPackageName::TryConvertFilenameToLongPackageName(InPath, PackageName);

			if (UE::IsSavingPackage(nullptr) || IsGarbageCollectingAndLockingUObjectHashTables()) {
				return nullptr;
			}
			else {
				Package = PackageName.IsEmpty() ? nullptr : FindPackage(nullptr, *PackageName);
			}
			return Package;
		}

	}

void PrintSupportNeededLogLine(const FString& InMessage, bool IsWarning)
{
	// Not possible to pass verbosity as a parameter - using boolean to switch between warning and error
	const FString DocumentationStr = TEXT("Official documentation is available at: https://docs.diversion.dev/quickstart?utm_source=ue-plugin&utm_medium=plugin");
	const FString SupportStr = TEXT("For support please either open a ticket in the Diversion Desktop app (help icon) or join our Discord server: https://discord.com/invite/wSJgfsMwZr");


	if (IsWarning) {
		UE_LOG(LogSourceControl, Warning, TEXT("%s"), *InMessage);
		UE_LOG(LogSourceControl, Warning, TEXT("%s"), *DocumentationStr);
		UE_LOG(LogSourceControl, Warning, TEXT("%s"), *SupportStr)
	}
	else {
		UE_LOG(LogSourceControl, Error, TEXT("%s"), *InMessage);
		UE_LOG(LogSourceControl, Error, TEXT("%s"), *DocumentationStr);
		UE_LOG(LogSourceControl, Error, TEXT("%s"), *SupportStr);
	}
}

bool WaitForCondition(const WaitForConditionPredicate& Condition, float Timeout, float InWaitPollInterval)
{
	double StartTime = FPlatformTime::Seconds();
	while (true) {
		if (!Condition.IsBound()) {
			return false;
		}
		if(Condition.Execute()){
			return true;
		}
		// Sleep for a short time to avoid busy waiting
		FPlatformProcess::Sleep(InWaitPollInterval);
		if (FPlatformTime::Seconds() > Timeout + StartTime) {
			return false;
		}
	}
}

bool StartDiversionAgent(const FString& InPathToDiversionBinary)
{
	auto& Provider = FDiversionModule::Get().GetProvider();
	if (Provider.IsAgentAlive(EConcurrency::Synchronous, true))
	{
		UE_LOG(LogSourceControl, Display, TEXT("Diversion agent is already running"));
		return true;
	}
	
	// Call the agent process detached
	FString RunParameters = TEXT(" --agent --bkg");
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(*InPathToDiversionBinary, *RunParameters, true, true, true, nullptr, 0, nullptr, nullptr);
	if (!ProcHandle.IsValid())
	{
		PrintSupportNeededLogLine("Failed to start Diversion Sync-Agent process. Check Diversion tray icon or open Diversion Desktop to verify agent is up.");

		return false;
	}
	
	// Wait for the process to start
	WaitForConditionPredicate WaitForAgentProcessDelegate = WaitForConditionPredicate::CreateLambda([&] {
		return Provider.IsAgentAlive(EConcurrency::Synchronous, true);
	});

	if (!DiversionUtils::WaitForCondition(WaitForAgentProcessDelegate, 5.0f, 1.0f)) {
		PrintSupportNeededLogLine("Diversion Sync-Agent failed to respond. Check Diversion tray icon or open Diversion Desktop to verify agent is up.");

		return false;
	}

	if (!Provider.CheckDiversionAvailability()) {
		PrintSupportNeededLogLine("Failed checking Diversion availability. Check Diversion tray icon or open Diversion Desktop to verify agent is up.");

		return false;
	}

	UE_LOG(LogSourceControl, Display, TEXT("Diversion Sync-Agent started successfully!"));
	return true;
}

int GetWorkspaceRevisionByCommit(const FString& CommitID)
{
	return FCString::Atoi(*DiversionUtils::RefToOrdinalId(CommitID));
}

FString GetAgentAPIURL()
{
	return FString(DIVERSION_AGENT_URL);
}

FString GetUserHomeDirectory()
{
#if PLATFORM_WINDOWS
	TCHAR HomeDir[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, HomeDir)))
	{
		return FString(HomeDir);
	}
#elif PLATFORM_LINUX || PLATFORM_MAC
	return FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
#else
#error Unsupported platform
#endif

	return FString();
}

FString GetWsSyncStatusString(EDiversionWsSyncStatus InStatus)
{
	switch(InStatus)
	{
		case EDiversionWsSyncStatus::Paused:
			return TEXT("Sync is paused.");
		case EDiversionWsSyncStatus::InProgress:
			return TEXT("Sync in Progress...");
		case EDiversionWsSyncStatus::Completed:
			return TEXT("Sync Complete!");
		case EDiversionWsSyncStatus::PathError:
			return TEXT("Sync temporarily stopped due to an inaccessible path.");
		default:
			return TEXT("Error! Unknown sync status.");
	}
}

void LoadWorkspaceInfo(FString InWorkingDir, WorkspaceInfo& OutInfo)
{
	//Info.Path = InWorkingDir;

	// Parse the .diversion/ workspace info file
	// TODO: Use endpoint instead of file
	FString JsonString;
	FString ConfigPath = FPaths::Combine(InWorkingDir, TEXT(".diversion"));
	// Get list of files in ConfigPath
	FJsonSerializableArray Files;
	IFileManager::Get().FindFiles(Files, *ConfigPath);

	// Search the first file that starts with "dv.ws"
	Files = Files.FilterByPredicate([](const FString& File) {
		return File.StartsWith(TEXT("dv.ws"));
		});

	if (Files.Num() == 0) {
		UE_LOG(LogSourceControl, Warning, TEXT("Failed to find workspace info file."));
		return;
	}
	const FString JsonFilePath = FPaths::Combine(ConfigPath, Files[0]);


	if (FFileHelper::LoadFileToString(JsonString, *JsonFilePath)) {
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			JsonObject->TryGetStringField(TEXT("WorkspaceID"), OutInfo.WorkspaceID);
			// TODO: use name instead of id
			JsonObject->TryGetStringField(TEXT("WorkspaceID"), OutInfo.WorkspaceName);
			JsonObject->TryGetStringField(TEXT("RepoID"), OutInfo.RepoID);
			// TODO: Add also name
			JsonObject->TryGetStringField(TEXT("AccountID"), OutInfo.AccountID);
			JsonObject->TryGetStringField(TEXT("BranchID"), OutInfo.BranchID);
			JsonObject->TryGetStringField(TEXT("BranchName"), OutInfo.BranchName);
			JsonObject->TryGetStringField(TEXT("CommitID"), OutInfo.CommitID);
			JsonObject->TryGetStringField(TEXT("RepoName"), OutInfo.RepoName);
			OutInfo.SetPath(InWorkingDir);
		}
		else {
			UE_LOG(LogSourceControl, Error, TEXT("Failed to deserialize workspace info file."));
		}
	}
}

FString ConvertFullPathToRelative(const FString& InPath, FString RootDir)
{
	if (RootDir == "") 
	{
		RootDir = FDiversionModule::Get().GetProvider().GetPathToWorkspaceRoot() + "/";
	}

	// Make sure RootDir ends with a '/'
	if (!RootDir.EndsWith("/"))
	{
		RootDir += "/";
	}

	if (InPath.StartsWith(RootDir))
	{
		return InPath.RightChop(RootDir.Len());
	}
	else if (RootDir.StartsWith(InPath)) 
	{
		// We passed a parent to the RootDir as InPath
		return "";
	}
	else
	{
		UE_LOG(LogSourceControl, Error, TEXT("Failed to convert path to relative: %s"), *InPath);
		return InPath;
	}
}

FString ConvertRelativePathToDiversionFull(const FString& InPath, FString RootDir)
{
	if (RootDir == "") {
		RootDir = FDiversionModule::Get().GetProvider().GetPathToWorkspaceRoot() + "/";
	}

	// Make sure RootDir ends with a '/'
	if (!RootDir.EndsWith("/"))
	{
		RootDir += "/";
	}

	FString FullPath;
	if (InPath.StartsWith(RootDir))
	{
		FullPath = InPath;
	}
	else
	{
		FullPath = RootDir / InPath;
	}
	return FullPath;
}

FString RefToOrdinalId(const FString& RefId)
{
	return RefId.RightChop(RefId.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd) + 1);
}

bool HttpTick(double& LastHttpTickTime)
{
	if (!FModuleManager::Get().IsModuleLoaded("HTTP"))
	{
		return false;
	}
	const double AppTime = FPlatformTime::Seconds();
	auto TickResult = FHttpModule::Get().GetHttpManager().Tick(float(AppTime - LastHttpTickTime));
	LastHttpTickTime = AppTime;
	return TickResult;
}

void ShowErrorNotification(const FText& ErrorMessage)
{
	FNotificationInfo Info(ErrorMessage);
	Info.Image = FAppStyle::GetBrush(TEXT("MessageLog.Error"));
	Info.bFireAndForget = true;
	Info.FadeOutDuration = 2.0f;
	Info.ExpireDuration = 5.0f;
	Info.bUseSuccessFailIcons = true;
	FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
}

bool IsDiversionInstalled()
{
#if PLATFORM_WINDOWS
	// Try reading the RegKey diversion installer sets during installation
	HKEY hKey;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Diversion"), 0, KEY_READ, &hKey);
	return result == ERROR_SUCCESS;
#else
	auto BinDir = FDiversionModule::Get().AccessSettings().GetBinaryPath();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        
	if (PlatformFile.FileExists(*BinDir))
	{
		return true;
   }
	else
	{
		return false;
   }
#endif
}

FString GetFilePathFromAssetData(const FAssetData& AssetData)
{
	FString PackageName = AssetData.PackageName.ToString();
	if(PackageName.IsEmpty())
	{
		return FString();
	}
		
	FString AssetPath;
	FPackageName::TryConvertLongPackageNameToFilename(PackageName, AssetPath);

	if (!AssetPath.EndsWith(TEXT(".uasset"))) {
		AssetPath += TEXT(".uasset");
	}
	return AssetPath;
}

bool SleepAndTick(double& LastHttpTickTime) {
	FPlatformProcess::Sleep(0.01f);
	if ((ENGINE_MINOR_VERSION == 4) || IsInGameThread()) {
		return DiversionUtils::HttpTick(LastHttpTickTime);
	}
	return false;
}

TSet<FString> GetPathsCommonPrefixes(const TArray<FString>& InPaths, const FString& InBaseRepoPath, int32 InLimit)
{
	TSet<FString> PathPrefixes;
	for (auto& FileOrDir : InPaths) {
		bool IsPathADirectory = FPaths::DirectoryExists(FileOrDir);
		FString PathAsDirectory = IsPathADirectory ? FileOrDir : FPaths::GetPath(FileOrDir);

		PathPrefixes.Add(PathAsDirectory);

		if ((InLimit > 0) && PathPrefixes.Num() >= InLimit) {
			// Limit the number of prefixes we check
			break;
		}
	}
	return PathPrefixes;
}

FString FindCommonAncestorDirectory(const TArray<FString>& InPaths)
{
	// Assuming the paths are either all absolute or all relative, no mixes!

	// Given list of files, find the directory which is the shared ancestor for all paths
	// and return it as relative path to the repo root.
	if (InPaths.Num() == 0)
	{
		return FString();
	}

	TArray<FString> CommonDirs;
	FString FirstPath = FPaths::GetPath(InPaths[0]);
	FPaths::NormalizeDirectoryName(FirstPath);
	FirstPath.ParseIntoArray(CommonDirs, TEXT("/"), true);

	for (int32 PathIndex = 1; PathIndex < InPaths.Num(); PathIndex++) {
		FString Path = FPaths::GetPath(InPaths[PathIndex]);
		FPaths::NormalizeDirectoryName(Path);
		TArray<FString> CurrentDirs;
		Path.ParseIntoArray(CurrentDirs, TEXT("/"), true);

		int32 CommonLength = FMath::Min(CommonDirs.Num(), CurrentDirs.Num());
		CommonDirs.SetNum(CommonLength);
		for (int32 ComponentIndex = 0; ComponentIndex < CommonLength; ++ComponentIndex) {
			if (CommonDirs[ComponentIndex] != CurrentDirs[ComponentIndex]) {
				CommonDirs.SetNum(ComponentIndex);
				break;
			}
		}
	}
	auto Result = FString::Join(CommonDirs, TEXT("/"));
	if (FirstPath.StartsWith(TEXT("/"))) {
		Result = TEXT("/") + Result;
	}
	return Result;
}

void WaitForHttpRequest(const double InMaxWaitingTimeoutSeconds, const FHttpRequestPtr InRequest, const FString& InAPICallName, const double InLogIntervalSeconds) {
	double AppTime = FPlatformTime::Seconds();
	double StartTime = FPlatformTime::Seconds();
	double LastLogTime = FPlatformTime::Seconds();

	while (EHttpRequestStatus::Processing == InRequest->GetStatus())
	{
		SleepAndTick(AppTime);
		if (FPlatformTime::Seconds() > InMaxWaitingTimeoutSeconds + StartTime) {
			// TODO: Remove this, probably introduced since Tick was not called outside game thread
			UE_LOG(LogSourceControl, Error, TEXT("Aborting API call due to a long wait."));
			break;
		}

		if (FPlatformTime::Seconds() > LastLogTime + InLogIntervalSeconds) {
			UE_LOG(LogSourceControl, Display, TEXT("Waiting for API call: %s, to complete..."), *InAPICallName);
			LastLogTime = FPlatformTime::Seconds();
		}
	}
	
	SleepAndTick(AppTime);
}

FString GetStackTrace()
{
	try {
		// Define the maximum number of stack frames to capture
		const int32 MaxStackDepth = 100;
		// Array to hold the program counters
		uint64 StackTrace[MaxStackDepth];
		// Capture the stack trace
		int32 StackDepth = FPlatformStackWalk::CaptureStackBackTrace(StackTrace, MaxStackDepth);

		// String to hold the human-readable stack trace
		FString StackTraceString;
		for (int32 i = 0; i < StackDepth; ++i)
		{
			// Convert each program counter to a human-readable string
			ANSICHAR HumanReadableString[1024];
			FPlatformStackWalk::ProgramCounterToHumanReadableString(i, StackTrace[i], HumanReadableString, sizeof(HumanReadableString));
			// Remove unicode characters
			StackTraceString += FString(ANSI_TO_TCHAR(HumanReadableString)) + TEXT("\n");
		}
		return StackTraceString;
	}
	catch (...)
	{
		return "Exception during collecting stack trace";
	}
}

bool DiversionValidityCheck(bool InCondition, FString InErrorDescription, const FString& AccountId, bool InSilent) {
	
	if (InCondition) {
		// Condition is met, no need to do anything
		return true;
	}

	if (AccountId.IsEmpty() || AccountId == "N/a") {
		UE_LOG(LogSourceControl, Error, TEXT("Diversion: Account ID is empty, cannot send error to BE"));
		return false;
	}

	if (!InSilent) {
		PrintSupportNeededLogLine(InErrorDescription, false);
	}

	// Collect stack trace
	FString StackTrace = GetStackTrace();
	const FString ErrorDescription = "Assertion failed, Error: " + InErrorDescription;
	SendErrorToBE(AccountId, ErrorDescription, StackTrace);

	return false;
}

bool WaitForAgentSync(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, float InSecondsToTimeout) {
	auto& SyncStatus = InCommand.Worker->SyncStatus;
	auto WaitForAgentSyncDelegate = WaitForConditionPredicate::CreateLambda([&] {
		return AgentInSync(InCommand, OutInfoMessages, OutErrorMessages) && 
			(SyncStatus == EDiversionWsSyncStatus::Completed);
	});
	if (!DiversionUtils::WaitForCondition(WaitForAgentSyncDelegate, InSecondsToTimeout)) {
		UE_LOG(LogSourceControl, Warning, TEXT("Diversion: UpdateStatus: Agent still syncing, please try again in a few moments. If the problem persists, try the following: 1. If there are many pending changes this could take some time, check for progress in the Diversion desktop app, 2. Verify there are no open files in the Editor (UE locks files), this could block the sync process. 3. If the sync is still not progressing, restart the Editor, it locks some files even when they are not open"));
		return false;
	}
	return true;
}

}




#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

