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
 * PresignedUploadUrl_fields.h
 *
 * 
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {


/*
 * PresignedUploadUrl_fields
 *
 * 
 */
class COREAPI_API PresignedUploadUrl_fields
    : public Model
{
public:
    virtual ~PresignedUploadUrl_fields() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	TOptional<FString> mX_amz_checksum_algorithm;

	TOptional<FString> mKey;

	TOptional<FString> mX_amz_algorithm;

	TOptional<FString> mX_amz_credential;

	TOptional<FString> mX_amz_date;

	TOptional<FString> mPolicy;

	TOptional<FString> mX_amz_signature;

};


}
}
}

