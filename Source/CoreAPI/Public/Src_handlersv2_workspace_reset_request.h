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
 * Src_handlersv2_workspace_reset_request.h
 *
 * 
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {


/*
 * Src_handlersv2_workspace_reset_request
 *
 * 
 */
class COREAPI_API Src_handlersv2_workspace_reset_request
    : public Model
{
public:
    virtual ~Src_handlersv2_workspace_reset_request() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	TOptional<bool> mAll;

	/* Inclusion list of paths to include in the operation. If null, all paths will be included. */
	TOptional<TArray<FString>> mPaths;

	TOptional<bool> mDelete_added;

	TOptional<bool> mWrite_to_journal;

};


}
}
}

