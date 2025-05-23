// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"

#include "OpenAPIDefaultApi.h"
#include "OpenAPIDefaultApiOperations.h"

using namespace AgentAPI;

using FWsSync = OpenAPIDefaultApi;

class FSyncGetSyncProgress;

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


using FGetSyncProgressAPI = TSyncApiCall<
	FSyncGetSyncProgress,
	FWsSync::GetSyncProgressRequest,
	FWsSync::GetSyncProgressResponse,
	FWsSync::FGetSyncProgressDelegate,
	FWsSync,
	&FWsSync::GetSyncProgress,
	UnauthorizedCall,
	// Output parameters
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/
	>;

REGISTER_PARSE_TYPE(FSyncGetSyncProgress);

class FSyncGetSyncProgress final : public FGetSyncProgressAPI {
	friend FGetSyncProgressAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FWsSync::GetSyncProgressResponse& Response);
};

bool IsPathWriteAccessible(const FString& Path)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*Path)) {
		return false;
	}

	return !PlatformFile.IsReadOnly(*Path);
}


bool FSyncGetSyncProgress::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FWsSync::GetSyncProgressResponse& Response) {
	IDiversionWorker& Worker = static_cast<IDiversionWorker&>(InCommand.Worker.Get());

	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	// Handle Locked Files
	bool LockHandleFeatureFlag = Response.Content.EnableLockRelease.IsSet() ? Response.Content.EnableLockRelease.GetValue() : false;
	if (LockHandleFeatureFlag) {
		TArray<OpenAPIWorkspaceSyncProgressErrorPathsInner> ErrorPaths = 
			Response.Content.ErrorPaths.IsSet() ? Response.Content.ErrorPaths.GetValue() : 
			TArray<OpenAPIWorkspaceSyncProgressErrorPathsInner>();

		auto OriginalWorkerSyncStatus = Worker.SyncStatus;
		if (!ErrorPaths.IsEmpty()) {
			// Tag the sync status as PathError 
			Worker.SyncStatus = DiversionUtils::EDiversionWsSyncStatus::PathError;
		}

		// Reset packages array 
		Worker.LockedPackages.Empty();

		for (const auto& ErrorPath : ErrorPaths) {
			if (!ErrorPath.ErrorCode.IsSet() || !ErrorPath.Path.IsSet()) {
				// Skip if no error code or path available
				continue;
			}

			EPathErrorCode ErrorCode = static_cast<EPathErrorCode>(ErrorPath.ErrorCode.GetValue());
			FString ErrorFilePath = ErrorPath.Path.GetValue();

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
					Worker.LockedPackages.Add({ Package, ErrorFilePath });
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
				FString ErrStr = ErrorPath.ErrorString.IsSet() ? ErrorPath.ErrorString.GetValue() : PathErrorCodeToString(ErrorCode);
				UE_LOG(LogSourceControl, Warning,
					TEXT("Sync received error: %s on the path: '%s'."), *ErrStr, *ErrorFilePath);
			}
		}
	}
	else {
		if (Response.Content.LastErr.IsSet()) {
			AddErrorMessage(Response.Content.LastErr.GetValue(), OutErrorMessages);
		}
	}
	
	return true;
}

bool DiversionUtils::WorkspaceSyncProgress(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages)
{
	auto Request = FWsSync::GetSyncProgressRequest();
	Request.RepoID = InCommand.WsInfo.RepoID;
	Request.WorkspaceID = InCommand.WsInfo.WorkspaceID;

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncGetSyncProgress>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages);
}