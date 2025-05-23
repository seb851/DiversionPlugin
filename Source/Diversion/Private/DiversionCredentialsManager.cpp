// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionCredentialsManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

#include "DiversionHttpManager.h"

#if PLATFORM_WINDOWS
#include <shlobj.h>
#endif

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "DiversionUtils.h"


#if PLATFORM_LINUX
#endif


TSharedPtr<FJsonObject> FCredentialsManager::GetCredFileForUser(FString& InUserId)
{
	auto ExistingCredFilePtr = UserCredsMap.Find(InUserId);

	if (ExistingCredFilePtr != nullptr) {
		return *ExistingCredFilePtr;
	}

	// Get diversion installation folder
	FString FilePath = "";
	if (InUserId == "N/a") {
		auto CredDir = DiversionUtils::GetUserHomeDirectory() / TEXT(".diversion") / DIVERSION_CREDENTIALS_FOLDER;
		TArray<FString> FileNames;
		FFileManagerGeneric FileMgr;
		FileMgr.SetSandboxEnabled(true);
		FString wildcard("*");
		FString search_path(FPaths::Combine(CredDir, *wildcard));

		FileMgr.FindFiles(FileNames, *search_path, true, false);
		auto TargetUserId = InUserId;
		if (FileNames.Num() == 1) {
			TargetUserId = FileNames[0];
		}
		FilePath = CredDir / TargetUserId;
	}
	else {
		FilePath = DiversionUtils::GetUserHomeDirectory() / TEXT(".diversion") / DIVERSION_CREDENTIALS_FOLDER / InUserId;
	}

	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *FilePath)) {
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			if (JsonObject->HasField(TEXT("token"))) {
				UserCredsMap.Add(InUserId, JsonObject);
			}
			else {
				UE_LOG(LogTemp, Error, TEXT("Token field not found."));
			}
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("Failed to deserialize credentials file."));
		}
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Failed to load cerdentials file."));
	}
	ExistingCredFilePtr = UserCredsMap.Find(InUserId);
	if (ExistingCredFilePtr != nullptr) {
		return *ExistingCredFilePtr;
	}
	return TSharedPtr<FJsonObject>();
}

FString FCredentialsManager::GetUserAccessToken(FString& InUserId)
{
	FString AccessToken;
	TSharedPtr<FJsonObject> ExistingCredFile = GetCredFileForUser(InUserId);

	if (!ExistingCredFile.IsValid()) {
		return "";
	}

	auto TokenObject = ExistingCredFile->GetObjectField(TEXT("token"));
	if (TokenObject->HasField(TEXT("access_token"))) {
		TokenObject->TryGetStringField(TEXT("access_token"), AccessToken);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Access token field not found."));
		AccessToken = RefreshAndCacheAccessToken(ExistingCredFile);
		if (AccessToken == "") {
			return "";
		}
	}

	if (AccessTokenExpired(AccessToken)) {
		AccessToken = RefreshAndCacheAccessToken(ExistingCredFile);
	}

	return AccessToken;
}

bool FCredentialsManager::AccessTokenExpired(FString AccessToken)
{
	TArray<FString> JwtComponents;
	AccessToken.ParseIntoArray(JwtComponents, TEXT("."), true);

	if (JwtComponents.Num() != 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid JWT"));
		return true;
	}

	FString EncodedPayload = JwtComponents[1];
	FString DecodedPayload;
	FBase64::Decode(EncodedPayload, DecodedPayload);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DecodedPayload);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		int64 ExpiryTime = static_cast<int64>(JsonObject->GetNumberField("exp"));
		const int64 ExpiryMarginSec = 60;
		FDateTime ExpiryDateTime = FDateTime::FromUnixTimestamp(ExpiryTime - ExpiryMarginSec);
		FDateTime Now = FDateTime::UtcNow();

		if (Now > ExpiryDateTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("JWT has expired"));
		}
		else
		{
			return false;
		}
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Invalid JWT body"));
	}
	return true;
}

FString FCredentialsManager::RefreshAndCacheAccessToken(TSharedPtr<FJsonObject> ExistingCredFile) {
	const FString OauthURL = "https://auth.diversion.dev/oauth2/token";
	const FString ClientId = "nmm65ta2r48pvj1lsjcmoeb7l";
	FString RefreshToken;
	FString AccessToken;
	volatile bool DelegateDone = false;
	auto TokenObject = ExistingCredFile->GetObjectField(TEXT("token"));
	if (TokenObject->HasField(TEXT("refresh_token"))) {
		TokenObject->TryGetStringField(TEXT("refresh_token"), RefreshToken);

		TMap<FString, FString> Headers;
		FString ContentType = TEXT("application/x-www-form-urlencoded");
		//Headers.Add("Content-Type", ContentType);
		FString Body = FString::Printf(TEXT("grant_type=refresh_token&refresh_token=%s&client_id=%s"),
			*FString(RefreshToken), *FString(ClientId));
		DiversionHttp::FHttpRequestManager CredentialsRequestManager(OauthURL);
		FString RequestPath = DiversionHttp::GetPathFromUrl(OauthURL);

		auto Response = CredentialsRequestManager.SendRequest(RequestPath, DiversionHttp::HttpMethod::POST, FString(), ContentType, Body, Headers, 5, 120);
		if(Response.Error.IsSet()) {
			UE_LOG(LogTemp, Error, TEXT("Failed using the refresh token auth flow: %s"), *Response.Error.GetValue());
			return "";
		}

		if(Response.ResponseCode >= 400) {
			UE_LOG(LogTemp, Error, TEXT("Failed using the refresh token auth flow: %s"), *Response.Error.GetValue());
			return "";
		}

		FString ResponseStr = Response.Contents;

		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			AccessToken = JsonObject->GetStringField(TEXT("access_token"));
			FString TokenType = JsonObject->GetStringField(TEXT("token_type"));
			int32 ExpiresIn = JsonObject->GetIntegerField(TEXT("expires_in"));
			FString IdToken = JsonObject->GetStringField(TEXT("id_token"));
			if (TokenType != "Bearer") {
				UE_LOG(LogTemp, Warning, TEXT("Got a token of type != Bearer"));
			}
			TokenObject->SetStringField(TEXT("access_token"), AccessToken);
			ExistingCredFile->SetObjectField(TEXT("token"), TokenObject);
		}
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Refresh token field not found."));
	}
	return AccessToken;
}
