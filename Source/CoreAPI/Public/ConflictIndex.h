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
 * ConflictIndex.h
 *
 * A index in a conflict, could be representing either \&quot;base\&quot;, \&quot;other\&quot; or \&quot;result\&quot;. If the index does not represent \&quot;result\&quot;, then all properties besides \&quot;prev_path\&quot; can be considered &#39;required&#39;. 
 */
#pragma once


#include "ModelBase.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {


/*
 * ConflictIndex
 *
 * A index in a conflict, could be representing either \&quot;base\&quot;, \&quot;other\&quot; or \&quot;result\&quot;. If the index does not represent \&quot;result\&quot;, then all properties besides \&quot;prev_path\&quot; can be considered &#39;required&#39;. 
 */
class COREAPI_API ConflictIndex
    : public Model
{
public:
    virtual ~ConflictIndex() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	enum class Conflict_index_idEnum
	{
		RESULT,
		BASE,
		OTHER,
  	};

	static FString EnumToString(const Conflict_index_idEnum& EnumValue);
	static bool EnumFromString(const FString& EnumAsString, Conflict_index_idEnum& EnumValue);
	Conflict_index_idEnum mConflict_index_id;

	int32_t mFile_mode = 0;

	FString mPath;

	TOptional<FString> mPrev_path;

	int32_t mType = 0;

	TOptional<int32_t> mStorage_backend;

	/* Coupled with storage_backend, a uri to storage location */
	TOptional<FString> mStorage_uri;

	/* Size in bytes */
	TOptional<int64_t> mSize;

};


}
}
}

