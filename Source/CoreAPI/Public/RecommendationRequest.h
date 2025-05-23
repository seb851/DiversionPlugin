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
 * RecommendationRequest.h
 *
 * The parameters needed for sending recommendations to friends
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {


/*
 * RecommendationRequest
 *
 * The parameters needed for sending recommendations to friends
 */
class COREAPI_API RecommendationRequest
    : public Model
{
public:
    virtual ~RecommendationRequest() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	/* The display name of the user */
	TOptional<FString> mUser_display_name;

	/* an optional user message to the recommendation email */
	TOptional<FString> mUser_message;

	/* A list of emails to send the recommendation to */
	TArray<FString> mEmails;

};


}
}
}

