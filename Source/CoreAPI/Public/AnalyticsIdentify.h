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
 * AnalyticsIdentify.h
 *
 * An analytics identification event
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {


/*
 * AnalyticsIdentify
 *
 * An analytics identification event
 */
class COREAPI_API AnalyticsIdentify
    : public Model
{
public:
    virtual ~AnalyticsIdentify() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	/* Event source */
	FString mSource;

	FDateTime mTime;

	TOptional<FString> mAnonymous_id;

};


}
}
}

