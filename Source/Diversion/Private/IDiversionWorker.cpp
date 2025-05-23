// Copyright 2024 Diversion Company, Inc. All Rights Reserved.
#include "IDiversionWorker.h"

#include "DiversionModule.h"
#include "DiversionProvider.h"
#include "PackageTools.h"
#include "ISourceControlModule.h"

bool IDiversionWorker::UpdateStates() const {
	ReleaseLockedPackages();
	return UpdateSyncStatus();
}

bool IDiversionWorker::UpdateSyncStatus() const {
	if (SyncStatus.IsSet()) {
		FDiversionProvider& Provider = FDiversionModule::Get().GetProvider();
		Provider.SetSyncStatus(SyncStatus.GetValue());
	}
	return true;
}

void IDiversionWorker::ReleaseLockedPackages() const
{
	bool RequiresReload = false;

	for (auto& LockdedPackage : LockedPackages) {
		FString PackagePath = LockdedPackage.Path;
		FString PackageName = FPackageName::FilenameToLongPackageName(PackagePath);
		UE_LOG(LogSourceControl, Warning,
			TEXT("'%s' possibly locked by UE, attempting to release to continue syncing"), *PackagePath);
		if (LockdedPackage.Package == nullptr) {
			UE_LOG(LogSourceControl, Warning, TEXT("'%s' is not a valid package, skipping unload"), *PackagePath);
			continue;
		}

		const bool PackageHasUnsavedChanges = LockdedPackage.Package->IsDirty();
		if (PackageHasUnsavedChanges)
		{
			UE_LOG(LogSourceControl, Warning, TEXT("'%s' has unsaved changes, skipping unload - Save changes in order to continue synching"), *PackageName);
			continue;
		}

		if (DiversionUtils::UPackageUtils::IsPackageOpenedInEditor(PackageName)) {
			UE_LOG(LogSourceControl, Warning, TEXT("'%s' is currently opened in the editor view - Close it in order to continue synching"), *PackageName);
			continue;
		}

		if (UPackageTools::UnloadPackages({ LockdedPackage.Package }))
		{
			UE_LOG(LogSourceControl, Display, TEXT("Successfully unloaded locked package: '%s'"), *PackagePath);
			RequiresReload = true;
		}
		else
		{
			UE_LOG(LogSourceControl, Warning, TEXT("Failed to unload the locked package: '%s', make sure the file is not being edited somewhere else, is not 'readonly' and you have the permissions to edit it."), *PackagePath);
		}
	}
	if (RequiresReload) {
		FDiversionModule::Get().GetProvider().SetReloadStatus();
	}
}
