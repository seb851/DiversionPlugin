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
 * Inline_object_7.h
 *
 * 
 */
#pragma once


#include "ModelBase.h"
#include "CollaborationInvite.h"
#include "Collaborator.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {

class Collaborator;
class CollaborationInvite;

/*
 * Inline_object_7
 *
 * 
 */
class COREAPI_API Inline_object_7
    : public Model
{
public:
    virtual ~Inline_object_7() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	TOptional<TArray<Collaborator>> mCollaborators;

	TOptional<TArray<CollaborationInvite>> mInvites;

};


}
}
}

