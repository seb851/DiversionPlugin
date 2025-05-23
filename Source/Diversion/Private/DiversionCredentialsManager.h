// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#pragma once


class FCredentialsManager
{

public:
	FString GetUserAccessToken(FString& InUserId);

private:

	TMap<FString, TSharedPtr<FJsonObject>> UserCredsMap;

	FString RefreshAndCacheAccessToken(TSharedPtr<FJsonObject> ExistingCredFile);

	TSharedPtr<FJsonObject> GetCredFileForUser(FString& InUserId);

	bool AccessTokenExpired(FString AccessToken);
};
