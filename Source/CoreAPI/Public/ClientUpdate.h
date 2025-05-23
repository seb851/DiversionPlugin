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
 * ClientUpdate.h
 *
 * A single update sent by the client
 */
#pragma once


#include "ModelBase.h"
#include "FileEntry.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {

class FileEntry;

/*
 * ClientUpdate
 *
 * A single update sent by the client
 */
class COREAPI_API ClientUpdate
    : public Model
{
public:
    virtual ~ClientUpdate() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	FString mTransaction_id;

	FileEntry mFile_entry;

};


}
}
}

