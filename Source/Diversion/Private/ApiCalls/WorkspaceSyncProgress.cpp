// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "DiversionModule.h"


const FString GenericInaccessiblePathMessgae = "Make sure the file is not being edited outside UE, is not 'readonly' or checked-out in other SCMs and you have the permissions to edit it";

enum class EPathErrorCode : int {
	GeneralPathError = 1,
	PathAccessDeniedError = 2
};

FString PathErrorCodeToString(EPathErrorCode InErrorCode) {
	switch (InErrorCode) {
	case EPathErrorCode::GeneralPathError:
		return TEXT("GeneralPathError");
	case EPathErrorCode::PathAccessDeniedError:
		return TEXT("PathAccessDeniedError");
	default:
		return TEXT("Unknown");
	}
}

bool IsPathWriteAccessible(const FString& Path)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*Path)) {
		return false;
	}

	return !PlatformFile.IsReadOnly(*Path);
}

using namespace Diversion::AgentAPI;

bool DiversionUtils::GetWorkspaceSyncProgress(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages)
{
	IDiversionWorker& Worker = static_cast<IDiversionWorker&>(InCommand.Worker.Get());

	auto ErrorResponse = DefaultApi::FgetSyncProgressDelegate::Bind([&]() {
		return false;
	});

	auto VariantResponse = DefaultApi::FgetSyncProgressDelegate::Bind([&](const TVariant<TSharedPtr<WorkspaceSyncProgress>, void*>& Variant) {
		if (!Variant.IsType<TSharedPtr<WorkspaceSyncProgress>>()) {
			// Unexpected response type
			OutErrorMessages.Add("Unexpected response type");
			return false;
		}

		auto Value = Variant.Get<TSharedPtr<WorkspaceSyncProgress>>();
		// Handle Locked Files
		bool LockHandleFeatureFlag = Value->mEnableLockRelease.IsSet() ? Value->mEnableLockRelease.GetValue() : false;

		if (LockHandleFeatureFlag) {
			auto ErrorPaths = 
				Value->mErrorPaths.IsSet() ? Value->mErrorPaths.GetValue() :
				TArray<WorkspaceSyncProgress_ErrorPaths_inner>();

			auto OriginalWorkerSyncStatus = Worker.SyncStatus;
			if (!ErrorPaths.IsEmpty()) {
				// Tag the sync status as PathError 
				Worker.SyncStatus = DiversionUtils::EDiversionWsSyncStatus::PathError;
			}

			// Reset packages array 
			Worker.OpenedPackagePaths.Empty();

			for (const auto& ErrorPath : ErrorPaths) {
				if (!ErrorPath.mErrorCode.IsSet() || !ErrorPath.mPath.IsSet()) {
					// Skip if no error code or path available
					continue;
				}

				EPathErrorCode ErrorCode = static_cast<EPathErrorCode>(ErrorPath.mErrorCode.GetValue());
				FString ErrorFilePath = ErrorPath.mPath.GetValue();

				if (ErrorCode == EPathErrorCode::PathAccessDeniedError) {
					if(ErrorFilePath.IsEmpty()) {
						UE_LOG(LogSourceControl, Warning, TEXT("Sync got access denied error for an empty path, ignoring."));
					}
				
					FString PackageName;
					UPackage* Package;
					FPackageName::TryConvertFilenameToLongPackageName(ErrorFilePath, PackageName);
				
					if (UE::IsSavingPackage(nullptr) || IsGarbageCollectingAndLockingUObjectHashTables()) {
						// We mustn't access packages during serialization or GC
						UE_LOG(LogSourceControl, Display,
							TEXT("UE is currently saving or performing GC, skipping."), *ErrorFilePath);
						break;
					}
					else {
						Package = PackageName.IsEmpty() ? nullptr : FindPackage(nullptr, *PackageName);
					}

					if (Package != nullptr) {
						Worker.OpenedPackagePaths.Add(ErrorFilePath);
					}
					else if (PackageName.IsEmpty()) {
						UE_LOG(LogSourceControl, Warning,
							TEXT("Sync got access denied error for path: '%s'. Paths is not an UE package. %s"), *ErrorFilePath, *GenericInaccessiblePathMessgae);
					}
					else {
						if (IsPathWriteAccessible(ErrorFilePath)) {
							// Sync is still in progress in the agent but the file was already released
							// Safe to reset to the original worker sync status
							Worker.SyncStatus = OriginalWorkerSyncStatus;
						}
						else {
							UE_LOG(LogSourceControl, Warning, TEXT("Sync got access denied error for UE package in path: '%s'. %s."), *ErrorFilePath, *GenericInaccessiblePathMessgae);
						}
					}
				}
				else {
					FString ErrStr = ErrorPath.mErrorString.IsSet() ? ErrorPath.mErrorString.GetValue() : PathErrorCodeToString(ErrorCode);
					UE_LOG(LogSourceControl, Warning,
						TEXT("Sync received error: %s on the path: '%s'."), *ErrStr, *ErrorFilePath);
				}
			}
		}
		else {
			if (Value->mLastErr.IsSet()) {
				const FString ErrMsg = FString::Printf(TEXT("call to GetSyncProgress Failed: %s"), *Value->mLastErr.GetValue());
				UE_LOG(LogSourceControl, Error, TEXT("%s"), *ErrMsg);
				OutErrorMessages.Add(ErrMsg);

				return false;
			}
		}

		return true;
	});

	return FDiversionModule::Get().AgentAPIRequestManager->GetSyncProgress(InCommand.WsInfo.RepoID, InCommand.WsInfo.WorkspaceID, FString(), {}, 5, 5)
		.HandleApiResponse(ErrorResponse, VariantResponse, OutErrorMessages);
}