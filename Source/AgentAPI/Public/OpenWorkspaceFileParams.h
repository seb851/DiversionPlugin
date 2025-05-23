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

/*
 * OpenWorkspaceFileParams.h
 *
 * Parameters for opening a file inside a locally cloned workspace
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace AgentAPI {
namespace Model {


/*
 * OpenWorkspaceFileParams
 *
 * Parameters for opening a file inside a locally cloned workspace
 */
class AGENTAPI_API OpenWorkspaceFileParams
    : public Model
{
public:
    virtual ~OpenWorkspaceFileParams() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	/* ID of the locally cloned workspace we want to open */
	FString mWorkspaceId;

	/* file path to item under the locally cloned workspace */
	FString mFilePath;

};


}
}
}

