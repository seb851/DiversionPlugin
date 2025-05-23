// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct WorkspaceInfo {
	WorkspaceInfo() {
		WorkspaceID = "N/a";
		WorkspaceName = "N/a";
		RepoID = "N/a";
		RepoName = "N/a";
		Path = "N/a";
		AccountID = "N/a";
		BranchID = "N/a";
		BranchName = "N/a";
		CommitID = "N/a";
	}
	FString WorkspaceID;
	FString WorkspaceName;
	FString RepoID;
	FString RepoName;
	FString AccountID;
	FString BranchID;
	FString BranchName;
	FString CommitID;

	void SetPath(const FString& In){ 
#if PLATFORM_WINDOWS
		Path = In.Replace(TEXT("\\"), TEXT("/"));
#elif PLATFORM_MAC || PLATFORM_LINUX
		Path = In;
#endif
	}
	const FString& GetPath() const { return Path;}

	bool IsValid() const {
		return WorkspaceID != "N/a" && RepoID != "N/a" && AccountID != "N/a" && BranchID != "N/a" && CommitID != "N/a";
	}

private:
	FString Path;
};