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
 * FileHistoryEntry.h
 *
 * A change in the history of a versioned item along with the relevant commit
 */
#pragma once


#include "ModelBase.h"
#include "Commit.h"
#include "FileEntry.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {

class FileEntry;
class Commit;

/*
 * FileHistoryEntry
 *
 * A change in the history of a versioned item along with the relevant commit
 */
class COREAPI_API FileHistoryEntry
    : public Model
{
public:
    virtual ~FileHistoryEntry() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	FileEntry mEntry;

	Commit mCommit;

};


}
}
}

