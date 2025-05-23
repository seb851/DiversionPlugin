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
 * TierWrapper.h
 *
 * A wrapper for the tier of the user
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {


/*
 * TierWrapper
 *
 * A wrapper for the tier of the user
 */
class COREAPI_API TierWrapper
    : public Model
{
public:
    virtual ~TierWrapper() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	/* The tier of the user */
	FString mTier;

};


}
}
}

