// Copyright 2024 Diversion Company, Inc. All Rights Reserved.
/**
 * Agent API
 * API of Diversion sync agent
 *
 * The version of the OpenAPI document: 1.0
 *
 * NOTE: This class is auto generated by OpenAPI-Generator 7.10.0.
 * https://openapi-generator.tech
 * Do not edit the class manually.
 */



#include "JsonBody.h"
#include "OpenWorkspaceFileParams.h"

namespace Diversion {
namespace AgentAPI {
namespace Model {



void OpenWorkspaceFileParams::WriteJson(JsonWriter& Writer) const
{
	Writer->WriteObjectStart();
	Writer->WriteIdentifierPrefix(TEXT("workspaceId")); WriteJsonValue(Writer, mWorkspaceId);
	Writer->WriteIdentifierPrefix(TEXT("filePath")); WriteJsonValue(Writer, mFilePath);
	Writer->WriteObjectEnd();
}

bool OpenWorkspaceFileParams::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{

	const TSharedPtr<FJsonObject>* InnerGeneratorOpenAPIObject;
	if (!JsonValue->TryGetObject(InnerGeneratorOpenAPIObject))
		return false;

	bool ParseSuccess = true;

    

	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("workspaceId"), mWorkspaceId);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("filePath"), mFilePath);


	return ParseSuccess;
}


}
}
}

