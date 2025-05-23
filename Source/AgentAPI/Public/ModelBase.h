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
 * ModelBase.h
 *
 * This is the base class for all model classes
 */
#pragma once

#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"

typedef TSharedRef<TJsonWriter<>> JsonWriter;

namespace Diversion {
namespace AgentAPI {
namespace Model {

class AGENTAPI_API Model
{
public:
    virtual ~Model() {}

	virtual void WriteJson(JsonWriter& Writer) const = 0;
	virtual bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) = 0;
};

}
}
}
