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
 * CreateTag.h
 *
 * 
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {


/*
 * CreateTag
 *
 * 
 */
class COREAPI_API CreateTag
    : public Model
{
public:
    virtual ~CreateTag() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	/* Display name of the tag */
	FString mName;

	/* Commit ID that the tag references */
	FString mCommit_id;

	/* More information about the tag */
	TOptional<FString> mDescription;

};


}
}
}

