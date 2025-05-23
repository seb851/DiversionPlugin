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
 * WorkspaceSyncStatus.h
 *
 * 
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace AgentAPI {
namespace Model {


/*
 * WorkspaceSyncStatus
 *
 * 
 */
class AGENTAPI_API WorkspaceSyncStatus
    : public Model
{
public:
    virtual ~WorkspaceSyncStatus() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	bool mIsSyncComplete = false;

	TOptional<bool> mIsPaused;

};


}
}
}

