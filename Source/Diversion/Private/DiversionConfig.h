// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/SoftObjectPath.h"

#include "DiversionConfig.generated.h"

UCLASS(config = DiversionConfig)
class UDiversionConfig : public UObject
{
        GENERATED_BODY()
public:
        UDiversionConfig();

        /*
         * Mark this setting object as editor only.
         * This so soft object path reference made by this setting object won't be automatically grabbed by the cooker.
         * @see UPackage::Save, FSoftObjectPathThreadContext::GetSerializationOptions, FSoftObjectPath::ImportTextItem
         */
        virtual bool IsEditorOnly() const override
        {
                return true;
        }
        /**
         * True if this client should be "headless"? (ie, not display any UI).
         */
        UPROPERTY(config, EditAnywhere, Category="General Settings", Meta=(ConfigRestartRequired=false,
                DisplayName="Enable Diversion Auto Soft Lock Confirmations",
                Tooltip="If unchecked, Diversion will not warn for potential conflicts before opening and saving files that have been modified in another branch or workspace"))
        bool bEnableSoftLock = true;
};

bool IsDiversionSoftLockEnabled();

