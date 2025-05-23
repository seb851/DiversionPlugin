// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionUtils.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformFilemanager.h"

bool DiversionUtils::DownloadFileFromURL(const FString& Url, const FString& SavePath)
{
	FHttpModule* HttpModule = &FHttpModule::Get();
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = HttpModule->CreateRequest();
	HttpRequest->SetURL(Url);
	HttpRequest->SetVerb(TEXT("GET"));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* FileHandle = PlatformFile.OpenWrite(*SavePath);

	bool Success = false;

	if (FileHandle)
	{
		HttpRequest->OnRequestProgress().BindLambda([FileHandle, LastWritten = int32(0)](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived) mutable
			{
				const int32 NewChunkSize = BytesReceived - LastWritten;
				if (NewChunkSize > 0)
				{
					const TArray<uint8>& ResponseData = Request->GetResponse()->GetContent();
					FileHandle->Write(ResponseData.GetData() + LastWritten, NewChunkSize);
					LastWritten = BytesReceived;
				}
			});
		HttpRequest->OnProcessRequestComplete().BindLambda([&Success, FileHandle](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
			{
				// Close the file stream
				delete FileHandle;
				Success = bWasSuccessful;

				if (!bWasSuccessful || !Response.IsValid())
				{
					// TODO: Handle error
				}
			});

		HttpRequest->ProcessRequest();
	}
	else
	{
		// TODO: Handle file opening error
	}

	// Wait for the request completion
	double VeryLongWaitForFileDownload = 60 * 60;
	double AvoidLogSynchronizationSpammig = 5.0;
	DiversionUtils::WaitForHttpRequest(VeryLongWaitForFileDownload, HttpRequest, "BlobDownload", AvoidLogSynchronizationSpammig);

	return Success;
}


