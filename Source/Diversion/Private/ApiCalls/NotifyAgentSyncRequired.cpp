// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "DiversionModule.h"

using namespace Diversion::AgentAPI;

bool DiversionUtils::NotifyAgentSyncRequired(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages)
{
    // Triggers an immediate sync on the agent (Pull updates etc. )
    // Should be used for commands that changes the BE state and requires immediate sync
    auto ErrorResponse = DefaultApi::FnotifySyncRequiredDelegate::Bind([&]() {
        return false;
    });

    auto VariantResponse = DefaultApi::FnotifySyncRequiredDelegate::Bind([&]() {
        return true;
    });

    return FDiversionModule::Get().AgentAPIRequestManager->NotifySyncRequired(InCommand.WsInfo.RepoID, 
        InCommand.WsInfo.WorkspaceID, FString(), {}, 5, 5).HandleApiResponse(ErrorResponse, VariantResponse, OutErrorMessages);
}

