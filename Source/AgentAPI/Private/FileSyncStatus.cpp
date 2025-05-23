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
#include "FileSyncStatus.h"

namespace Diversion {
namespace AgentAPI {
namespace Model {



void FileSyncStatus::WriteJson(JsonWriter& Writer) const
{
	Writer->WriteObjectStart();
	Writer->WriteIdentifierPrefix(TEXT("Path")); WriteJsonValue(Writer, mPath);
	Writer->WriteIdentifierPrefix(TEXT("IsSynced")); WriteJsonValue(Writer, mIsSynced);
	if (mStatusDescription.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("StatusDescription")); WriteJsonValue(Writer, mStatusDescription.GetValue());
	}
	Writer->WriteObjectEnd();
}

bool FileSyncStatus::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{

	const TSharedPtr<FJsonObject>* InnerGeneratorOpenAPIObject;
	if (!JsonValue->TryGetObject(InnerGeneratorOpenAPIObject))
		return false;

	bool ParseSuccess = true;

    

	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("Path"), mPath);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("IsSynced"), mIsSynced);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("StatusDescription"), mStatusDescription);


	return ParseSuccess;
}


}
}
}

