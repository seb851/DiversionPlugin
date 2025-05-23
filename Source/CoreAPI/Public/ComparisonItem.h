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
 * ComparisonItem.h
 *
 * Describes a comparison result at a single path. At least one of base/other items must be present.
 */
#pragma once


#include "ModelBase.h"
#include "FileEntry.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {

class FileEntry;

/*
 * ComparisonItem
 *
 * Describes a comparison result at a single path. At least one of base/other items must be present.
 */
class COREAPI_API ComparisonItem
    : public Model
{
public:
    virtual ~ComparisonItem() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	TOptional<FileEntry> mBase_item;

	TOptional<FileEntry> mOther_item;

	int32_t mStatus = 0;

};


}
}
}

