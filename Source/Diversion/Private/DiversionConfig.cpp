// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionConfig.h"

UDiversionConfig::UDiversionConfig()
{
}

bool IsDiversionSoftLockEnabled()
{
        const UDiversionConfig* ConcertClientConfig = GetDefault<UDiversionConfig>();
        return ConcertClientConfig->bEnableSoftLock;
}

