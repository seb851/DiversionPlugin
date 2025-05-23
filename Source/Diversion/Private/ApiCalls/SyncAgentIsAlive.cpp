// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "ISourceControlModule.h"
#include "DiversionCommand.h"

#include "OpenAPIDefaultApi.h"
#include "OpenAPIDefaultApiOperations.h"

#include "DiversionOperations.h"

using namespace AgentAPI;

using AgentIsAliveSync = OpenAPIDefaultApi;

class FSyncFIsAlive;
using FIsAliveAPI = TSyncApiCall<
	FSyncFIsAlive,
	AgentIsAliveSync::IsAliveRequest,
	AgentIsAliveSync::IsAliveResponse,
	AgentIsAliveSync::FIsAliveDelegate,
	AgentIsAliveSync,
	&AgentIsAliveSync::IsAlive,
	UnauthorizedCall,
	FDiversionWorkerRef, /*Worker*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/>;


class FSyncFIsAlive final : public FIsAliveAPI {
	friend FIsAliveAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(FDiversionWorkerRef InOutWorker, TArray<FString>& OutInfoMessages, 
		TArray<FString>& OutErrorMessages, const AgentIsAliveSync::IsAliveResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncFIsAlive);



bool FSyncFIsAlive::ResponseImplementation(FDiversionWorkerRef InOutWorker, TArray<FString>& OutInfoMessages,
	TArray<FString>& OutErrorMessages, const AgentIsAliveSync::IsAliveResponse& Response) {
	if (!Response.IsSuccessful()) {
		FString BaseErr;
		switch (Response.GetHttpResponseCode()) {
		case EHttpResponseCodes::RequestTimeout:
			BaseErr = "Diversion Sync Agent is not running";
			break;
		default:
			BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
			break;
		}
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	// TODO: For a safer plugin - we should pass the worker type to the template arguments instead of using casting here
	FDiversionAgentHealthCheckWorker& Worker = static_cast<FDiversionAgentHealthCheckWorker&>(InOutWorker.Get());
	if (const FString* VersionPtr = Response.Content.Version.GetPtrOrNull()) {
		Worker.AgentVersion = *VersionPtr;
	}
	else {
		AddErrorMessage("Failed parsing the agent version", OutErrorMessages);
		return false;
	}

	// If we got a valid version response it means the agent is alive
	Worker.IsAgentAlive = true;
	return true;
}


bool DiversionUtils::RunAgentHealthCheck(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages)
{
	const auto Request = AgentIsAliveSync::IsAliveRequest();

	auto& Module = FDiversionModule::Get();
	if (&Module == nullptr) {
		return false;
	}

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncFIsAlive>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand.Worker, OutInfoMessages, OutErrorMessages);
}