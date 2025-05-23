// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "ISourceControlModule.h"

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "IDiversionStatusWorker.h"
#include "DiversionCommand.h"
#include "DiversionState.h"



#include "OpenAPIRepositoryManipulationApi.h"
#include "OpenAPIRepositoryManipulationApiOperations.h"


using namespace CoreAPI;

using FHistory = OpenAPIRepositoryManipulationApi;

class FSyncFileHistory;
using FHistorySyncAPI = TSyncApiCall<
	FSyncFileHistory,
	FHistory::SrcHandlersv2CommitGetObjectHistoryRequest,
	FHistory::SrcHandlersv2CommitGetObjectHistoryResponse,
	FHistory::FSrcHandlersv2CommitGetObjectHistoryDelegate,
	FHistory,
	&FHistory::SrcHandlersv2CommitGetObjectHistory,
	AuthorizedCall,
	// Output parameters
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*InfoMessages*/
	TArray<FString>& /*ErrorMessages*/>;


class FSyncFileHistory final : public FHistorySyncAPI {
	friend FHistorySyncAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FHistory::SrcHandlersv2CommitGetObjectHistoryResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncFileHistory);


static void PopulateSCCRevision(TSharedRef<FDiversionRevision, ESPMode::ThreadSafe> InOutSCCRev,
	const FString& InCommitId, const int64 InCreatedTs, const FString& InPath,
	const int InStatus, TOptional<OpenAPIFileEntryBlob> InBlob,
	const FString& InUserName, const FString& InCommitMessage, const WorkspaceInfo& InWsInfo)
{
	auto OrdinalCommitId = DiversionUtils::RefToOrdinalId(InCommitId);
	auto OrdinalCommitNumber = FCString::Atoi(*OrdinalCommitId);

	InOutSCCRev->RevisionNumber = OrdinalCommitNumber;
	InOutSCCRev->ShortCommitId = OrdinalCommitId;
	InOutSCCRev->CommitId = InCommitId;
	InOutSCCRev->CommitIdNumber = OrdinalCommitNumber;
	InOutSCCRev->Date = FDateTime::FromUnixTimestamp(InCreatedTs);
	InOutSCCRev->Filename = InPath;

	switch (InStatus)
	{
	case 1:
		InOutSCCRev->Action = "intact";
		break;
	case 2:
		InOutSCCRev->Action = "add";
		break;
	case 3:
		InOutSCCRev->Action = "modified";
		break;
	case 4:
		InOutSCCRev->Action = "delete";
		break;
	}

	if (InBlob.IsSet())
	{
		InOutSCCRev->FileHash = InBlob.GetValue().Sha;
		InOutSCCRev->FileSize = static_cast<int32>(InBlob.GetValue().Size);
	}

	InOutSCCRev->UserName = InUserName;
	InOutSCCRev->Description = InCommitMessage;

	InOutSCCRev->WsInfo = InWsInfo;
}

FString ExtractFileNameFromCommitEntry(const OpenAPICommit& InCommitEntry)
{
	if(InCommitEntry.Author.FullName.IsSet() && !InCommitEntry.Author.FullName->IsEmpty())
	{
		return InCommitEntry.Author.FullName.GetValue();
	}
	else if(InCommitEntry.Author.Email.IsSet() && !InCommitEntry.Author.Email->IsEmpty())
	{
		return InCommitEntry.Author.Email.GetValue();
	}
	else
	{
		return InCommitEntry.Author.Id;
	}
}

bool FSyncFileHistory::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FHistory::SrcHandlersv2CommitGetObjectHistoryResponse& Response) {
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	IDiversionStatusWorker& Worker = static_cast<IDiversionStatusWorker&>(InCommand.Worker.Get());

	FString FilePath;
	auto Revisions = Response.Content.Entries;
	if(Revisions.Num() == 0)
	{
		OutInfoMessages.Add("No history found for the file");
		return true;
	}
	
	FilePath = DiversionUtils::ConvertRelativePathToDiversionFull(Response.Content.Entries[0].Entry.Path, InCommand.WsInfo.GetPath());

	TDiversionHistory History;

	for (auto& Revision : Revisions) {
		TSharedRef<FDiversionRevision, ESPMode::ThreadSafe> SourceControlRevision = MakeShareable(new FDiversionRevision);

		FString UserName = ExtractFileNameFromCommitEntry(Revision.Commit);
		FString CommitMessage = Revision.Commit.CommitMessage.IsSet() ? Revision.Commit.CommitMessage.GetValue() : "";

		PopulateSCCRevision(SourceControlRevision, Revision.Commit.CommitId, Revision.Commit.CreatedTs, Revision.Entry.Path,
		                                    Revision.Entry.Status, Revision.Entry.Blob, UserName, CommitMessage, InCommand.WsInfo);
		History.Add(MoveTemp(SourceControlRevision));
	}

	if (TDiversionHistory* Existing = Worker.Histories.Find(FilePath)) {
		History.Append(*Existing);
	}

	Worker.Histories.Add(FilePath, MoveTemp(History));
	return true;
}


bool DiversionUtils::RunGetHistory(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, 
	const FString& InFile, const FString* MergeFromRef)
{
	auto Request = FHistory::SrcHandlersv2CommitGetObjectHistoryRequest();
	Request.Path = ConvertFullPathToRelative(InFile, InCommand.WsInfo.GetPath());
	Request.RepoId = InCommand.WsInfo.RepoID;

	if (MergeFromRef)
	{
		Request.RefId = *MergeFromRef;
		Request.Limit = 1;
	}
	else
	{
		// TODO: handle limit/pagination of file history?
		Request.RefId = InCommand.WsInfo.WorkspaceID;
	}

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncFileHistory>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages);
}
