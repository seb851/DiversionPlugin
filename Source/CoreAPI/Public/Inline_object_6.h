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
 * Inline_object_6.h
 *
 * 
 */
#pragma once


#include "ModelBase.h"
#include "Shelf.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {

class Shelf;

/*
 * Inline_object_6
 *
 * 
 */
class COREAPI_API Inline_object_6
    : public Model
{
public:
    virtual ~Inline_object_6() {}

	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	void WriteJson(JsonWriter& Writer) const override;


	enum class ObjectEnum
	{
		SHELF,
  	};

	static FString EnumToString(const ObjectEnum& EnumValue);
	static bool EnumFromString(const FString& EnumAsString, ObjectEnum& EnumValue);
	ObjectEnum mobject;

	TArray<Shelf> mItems;

};


}
}
}

