// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"

#include "OpenAPIDefaultApi.h"
#include "OpenAPIDefaultApiOperations.h"

using namespace AgentAPI;

using FAgentApi = OpenAPIDefaultApi;

class FNotifyAgentSyncRequired;

using FNotifyAgentSyncAPI = TSyncApiCall<
        FNotifyAgentSyncRequired,
        FAgentApi::NotifySyncRequiredRequest,
        FAgentApi::NotifySyncRequiredResponse,
        FAgentApi::FNotifySyncRequiredDelegate,
        FAgentApi,
        &FAgentApi::NotifySyncRequired,
        UnauthorizedCall,
        // Output parameters
        const FDiversionCommand&, /*InCommand*/
        TArray<FString>&, /*ErrorMessages*/
        TArray<FString>& /*InfoMessages*/
        >;

REGISTER_PARSE_TYPE(FNotifyAgentSyncRequired);

class FNotifyAgentSyncRequired final : public FNotifyAgentSyncAPI {
        friend FNotifyAgentSyncAPI;     // To support Static Polymorphism and keep encapsulation

        static bool ResponseImplementation(const FDiversionCommand& InCommand,
                TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FAgentApi::NotifySyncRequiredResponse& Response);
};

bool FNotifyAgentSyncRequired::ResponseImplementation(const FDiversionCommand& InCommand,
        TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FAgentApi::NotifySyncRequiredResponse& Response) {
        if (!Response.IsSuccessful()) {
                const FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
                AddErrorMessage(BaseErr, OutErrorMessages);
                return false;
        }
        return true;
}

bool DiversionUtils::NotifyAgentSyncRequired(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages)
{
        // Triggers an immediate sync on the agent (Pull updates etc. )
        // Should be used for commands that changes the BE state and requires immediate sync
        auto Request = FAgentApi::NotifySyncRequiredRequest();
        Request.RepoID = InCommand.WsInfo.RepoID;
        Request.WorkspaceID = InCommand.WsInfo.WorkspaceID;

        auto& ApiCall = FDiversionModule::Get().GetApiCall<FNotifyAgentSyncRequired>();
        return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages);
}

