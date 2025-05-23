// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "DiversionModule.h"
#include "DiversionOperations.h"

using namespace Diversion::CoreAPI;

bool DiversionUtils::GetWsBlobInfo(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InFile)
{
	auto ErrorResponse = RepositoryManipulationApi::Fsrc_handlersv2_files_getFileEntryDelegate::Bind(
		[&]() {
			return false;
		}
	);
	auto VariantResponse = RepositoryManipulationApi::Fsrc_handlersv2_files_getFileEntryDelegate::Bind(
		[&](const TVariant<TSharedPtr<FileEntry>, TSharedPtr<Diversion::CoreAPI::Error>>& Variant) {

			if (Variant.IsType<TSharedPtr<Diversion::CoreAPI::Model::Error>>()) {
				auto Value = Variant.Get<TSharedPtr<Diversion::CoreAPI::Model::Error>>();
				OutErrorMessages.Add(FString::Printf(TEXT("Received error for reset call: %s"), *Value->mDetail));
				return false;
			}

			if (!Variant.IsType<TSharedPtr<FileEntry>>()) {
				// Unexpected response type
				OutErrorMessages.Add("Unexpected response type");
				return false;
			}

			auto Value = Variant.Get<TSharedPtr<FileEntry>>();

			FDiversionResolveFileWorker& Worker = static_cast<FDiversionResolveFileWorker&>(InCommand.Worker.Get());
			Worker.FileEntry = *Value;

			OutInfoMessages.Add("Successfuly retrieved file entry");
			return true;
		}
	);

	return FDiversionModule::Get().RepositoryManipulationAPIRequestManager->SrcHandlersv2FilesGetFileEntry(
		InCommand.WsInfo.RepoID, InCommand.WsInfo.WorkspaceID, ConvertFullPathToRelative(InFile, InCommand.WsInfo.GetPath()), 
		FDiversionModule::Get().GetAccessToken(InCommand.WsInfo.AccountID), {}, 5, 120).HandleApiResponse(ErrorResponse, VariantResponse, OutErrorMessages);
}