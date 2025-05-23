// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "DiversionModule.h"

using namespace Diversion::AgentAPI;

bool DiversionUtils::RunRepoInit(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InRepoRootPath, const FString& InRepoName)
{
	auto ErrorResponse = DefaultApi::FrepoInitDelegate::Bind([&]() {
		OutErrorMessages.Add("Failed initializing repo in the provided path.");
		return false;
	});

	auto VariantResponse = DefaultApi::FrepoInitDelegate::Bind([&]() {
		OutInfoMessages.Add("Repo initialized successfully");
		return true;
	});

	auto initRepoRequestData = MakeShared<InitRepo>();
	initRepoRequestData->mName = InRepoName;
	initRepoRequestData->mPath = InRepoRootPath;
	return FDiversionModule::Get().AgentAPIRequestManager->RepoInit(initRepoRequestData, FString(), {}, 5, 5).
		HandleApiResponse(ErrorResponse, VariantResponse, OutErrorMessages);
}