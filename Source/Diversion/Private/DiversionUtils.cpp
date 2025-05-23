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
#include "FileHelpers.h"

#include "HttpModule.h"
#include "HttpManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

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

		bool SavePackage(UPackage* Package)
		{

			if (!Package)
			{
				return false;
			}

			FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(),
				FPackageName::GetAssetPackageExtension());

			// Save the package with optional flags, using `SavePackage` from `UPackageTools`
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			SaveArgs.SaveFlags = SAVE_NoError;
			bool bSuccess = UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
			
			if (bSuccess)
			{
				UE_LOG(LogSourceControl, Log, TEXT("Package saved successfully: %s"), *PackageFileName);
			}
			else
			{
				UE_LOG(LogSourceControl, Error, TEXT("Failed to save package: %s"), *PackageFileName);
			}

			return bSuccess;
		}

		void ClosePackageEditor(UPackage* Package)
		{
			if (!Package)
			{
				UE_LOG(LogSourceControl, Warning, TEXT("Invalid package provided to ClosePackageEditor."));
				return;
			}

			// Get the asset associated with the package
			UObject* Asset = Package->FindAssetInPackage();
			if (!Asset)
			{
				UE_LOG(LogSourceControl, Warning, TEXT("No asset found in package."));
				return;
			}

			// Get the Asset Editor Subsystem to close the editor
			UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			if (AssetEditorSubsystem->CloseAllEditorsForAsset(Asset))
			{
				UE_LOG(LogSourceControl, Display, TEXT("Closed editor tabs for asset: %s"), *Asset->GetName());
			}
			else
			{
				UE_LOG(LogSourceControl, Warning, TEXT("No open editor tabs found for asset: %s"), *Asset->GetName());
			}
		}

		void DiscardUnsavedChanges(UPackage* Package)
		{
			if(Package != nullptr)
			{
				if(!UPackageTools::ReloadPackages({Package}))
				{
					UE_LOG(LogSourceControl, Warning, TEXT("Failed to reload package: %s"), *Package->GetName());
				}
			}
			else
			{
				UE_LOG(LogSourceControl, Warning, TEXT("Invalid package provided to DiscardUnsavedChanges."));
			}
		}

		bool BackupUnsavedChanges(UPackage* Package)
		{
			if (!Package)
			{
				UE_LOG(LogSourceControl, Error, TEXT("Package is null, cannot backup unsaved changes"));
				return false;
			}

			// Find the primary asset. We assume that its name is the same as the package's short name.
			const FString PrimaryAssetName = FPackageName::GetShortName(Package->GetName());
			UObject* PrimaryAsset = FindObject<UObject>(Package, *PrimaryAssetName);
			if (!PrimaryAsset)
			{
				UE_LOG(LogSourceControl, Error, TEXT("Cannot find the primary asset in package %s"), *Package->GetName());
				// Better to copy the actual data aside and let the user deal with it manually
				return false;
			}

			// Determine the backup file name
			const FString Folder = FPackageName::GetLongPackagePath(Package->GetName());
			const FString BaseNewName = PrimaryAsset->GetName() + TEXT("_recovered");

			// Ensure the new asset name is unique.
			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
			FString NewLongName;
			FString NewAssetName;
			AssetTools.CreateUniqueAssetName(Folder / BaseNewName, TEXT(""), NewLongName, NewAssetName);

			// Duplicate the primary asset into the new package.
			UObject* NewAsset = AssetTools.DuplicateAsset(NewAssetName, Folder, PrimaryAsset);
			if (!NewAsset)
			{
				UE_LOG(LogSourceControl, Error, TEXT("Asset duplication failed for %s"), *PrimaryAsset->GetName());
				return false;
			}

			// Notify the Asset Registry so the new asset appears in the Content Browser.
			FAssetRegistryModule::AssetCreated(NewAsset);

			// Retrieve the new asset's package
			UPackage* NewPkg = NewAsset->GetPackage();

			// Convert the package name to the expected on disk file path.
			const FString AbsFilename = FPackageName::LongPackageNameToFilename(NewPkg->GetName(), FPackageName::GetAssetPackageExtension());
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(AbsFilename), true);

			// Save the new package to disk.
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			bool bSaved = UPackage::SavePackage(NewPkg, NewAsset, *AbsFilename, SaveArgs);
			if (bSaved)
			{
				UE_LOG(LogSourceControl, Display, TEXT("Backup saved successfully to %s"), *AbsFilename);
			}
			else
			{
				UE_LOG(LogSourceControl, Error, TEXT("Failed to save backup to %s"), *AbsFilename);
			}

			return bSaved;
		}

		bool MakeSimpleOSCopyPackageBackup(const FString& OriginalFileFullPath, const FString& TargetBackupFolderPath) {
			if (!FPaths::FileExists(OriginalFileFullPath))
			{
				UE_LOG(LogSourceControl, Error, TEXT("Original package file not found: %s"), *OriginalFileFullPath);
				return false;
			}

			const FString BackupFileName = FPaths::GetBaseFilename(OriginalFileFullPath) + TEXT("_recovered");
			int32 CopyResult = IFileManager::Get().Copy(*(TargetBackupFolderPath / BackupFileName), *OriginalFileFullPath);
			if (CopyResult == COPY_OK)
			{
				UE_LOG(LogSourceControl, Log, TEXT("OS backup copy created successfully: %s"), *TargetBackupFolderPath);
				return true;
			}
			else
			{
				UE_LOG(LogSourceControl, Error, TEXT("OS backup copy failed, error code: %d"), CopyResult);
				return false;
			}
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

	if (!DiversionUtils::WaitForCondition(WaitForAgentProcessDelegate, 10.0f, 1.0f)) {
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

TMap<FString, FString> GetDiversionHeaders()
{
	TMap<FString, FString> InHeaders;
	InHeaders.Add(APP_VERSION_HEADER_KEY, FDiversionModule::GetPluginVersion());
	InHeaders.Add(APP_NAME_HEADER_KEY, FDiversionModule::GetAppName());
	InHeaders.Add(CORRELATION_ID_HEADER_KEY, FGuid::NewGuid().ToString());
	return InHeaders;	
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

