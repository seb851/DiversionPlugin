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

/*
 * OrganizationSubscription.h
 *
 * 
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {


/*
 * OrganizationSubscription
 *
 * 
 */
class COREAPI_API OrganizationSubscription
    : public Model
{
public:
    virtual ~OrganizationSubscription() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	/* Unique identifier for the organization subscription */
	FString mId;

	/* ID of the organization */
	FString mOrganization_id;

	enum class Subscription_typeEnum
	{
		STORAGE_GB,
		USERS,
  	};

	static FString EnumToString(const Subscription_typeEnum& EnumValue);
	static bool EnumFromString(const FString& EnumAsString, Subscription_typeEnum& EnumValue);
	/* Type of the subscription */
	Subscription_typeEnum mSubscription_type;

	/* Amount of the subscription */
	int32_t mAmount = 0;

	/* Start of the grace period */
	TOptional<FDateTime> mGrace_period_start;

	/* End of the grace period */
	TOptional<FDateTime> mGrace_period_end;

};


}
}
}

