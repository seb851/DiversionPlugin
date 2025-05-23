// Copyright 2024 Diversion Company, Inc. All Rights Reserved.
/**
 * Diversion Core API
 * Definition of the Core API used to access low-level functionality of Diversion
 *
 * The version of the OpenAPI document: 0.2.0
 *
 * NOTE: This class is auto generated by OpenAPI-Generator 7.10.0.
 * https://openapi-generator.tech
 * Do not edit the class manually.
 */



#include "JsonBody.h"
#include "OrganizationSubscription.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {



void OrganizationSubscription::WriteJson(JsonWriter& Writer) const
{
	Writer->WriteObjectStart();
	Writer->WriteIdentifierPrefix(TEXT("id")); WriteJsonValue(Writer, mId);
	Writer->WriteIdentifierPrefix(TEXT("organization_id")); WriteJsonValue(Writer, mOrganization_id);
	Writer->WriteIdentifierPrefix(TEXT("subscription_type")); WriteJsonValue(Writer, OrganizationSubscription::EnumToString(mSubscription_type));
	Writer->WriteIdentifierPrefix(TEXT("amount")); WriteJsonValue(Writer, mAmount);
	if (mGrace_period_start.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("grace_period_start")); WriteJsonValue(Writer, mGrace_period_start.GetValue());
	}
	if (mGrace_period_end.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("grace_period_end")); WriteJsonValue(Writer, mGrace_period_end.GetValue());
	}
	Writer->WriteObjectEnd();
}

bool OrganizationSubscription::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{

	const TSharedPtr<FJsonObject>* InnerGeneratorOpenAPIObject;
	if (!JsonValue->TryGetObject(InnerGeneratorOpenAPIObject))
		return false;

	bool ParseSuccess = true;

    

	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("id"), mId);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("organization_id"), mOrganization_id);
    // Reading the value into a string enum first
    FString Subscription_typeString;
    bool ParseEnumSubscription_typeStringSuccess = TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("subscription_type"), Subscription_typeString);
    if (ParseEnumSubscription_typeStringSuccess) {
        ParseSuccess &= OrganizationSubscription::EnumFromString(Subscription_typeString, mSubscription_type);
        
    }
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("amount"), mAmount);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("grace_period_start"), mGrace_period_start);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("grace_period_end"), mGrace_period_end);


	return ParseSuccess;
}


FString OrganizationSubscription::EnumToString(const Subscription_typeEnum& EnumValue) {
    switch (EnumValue)
    {
    case Subscription_typeEnum::STORAGE_GB:
        return TEXT("STORAGE_GB");
    case Subscription_typeEnum::USERS:
        return TEXT("USERS");
    default:
        return TEXT("");
    }
}

bool OrganizationSubscription::EnumFromString(const FString& EnumAsString, Subscription_typeEnum& EnumValue) {
    if(EnumAsString.IsEmpty()) return false;
    if(EnumAsString == TEXT("STORAGE_GB")) {
        EnumValue = Subscription_typeEnum::STORAGE_GB;
        return true;
    }
    if(EnumAsString == TEXT("USERS")) {
        EnumValue = Subscription_typeEnum::USERS;
        return true;
    }

    return false;
}


}
}
}

