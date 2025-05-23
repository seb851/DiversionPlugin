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
 * WorkspaceSyncProgress_FileStats.h
 *
 * 
 */
#pragma once


#include "ModelBase.h"
#include "FileStatusAggregation.h"

namespace Diversion {
namespace AgentAPI {
namespace Model {

class FileStatusAggregation;

/*
 * WorkspaceSyncProgress_FileStats
 *
 * 
 */
class AGENTAPI_API WorkspaceSyncProgress_FileStats
    : public Model
{
public:
    virtual ~WorkspaceSyncProgress_FileStats() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	TOptional<FileStatusAggregation> mInbound;

	TOptional<FileStatusAggregation> mOutbound;

};


}
}
}

