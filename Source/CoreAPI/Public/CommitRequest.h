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
 * CommitRequest.h
 *
 * 
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {


/*
 * CommitRequest
 *
 * 
 */
class COREAPI_API CommitRequest
    : public Model
{
public:
    virtual ~CommitRequest() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	FString mCommit_message;

	/* Inclusion list of paths to include in the operation. If null, all paths will be included. */
	TOptional<TArray<FString>> mInclude_paths;

};


}
}
}

