// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionPotentialClashUI.h"

#include "DiversionState.h"
#include "ISourceControlState.h"
#include "ISourceControlModule.h"
#include "RevisionControlStyle/RevisionControlStyle.h"


TSharedPtr<FDiversionState, ESPMode::ThreadSafe> GetDiversionStateForAsset(FString AssetPath)
{
	try {
		auto& Provider = ISourceControlModule::Get().GetProvider();
		if (!Provider.GetName().ToString().Equals("Diversion"))
		{
			return nullptr;
		}

		FSourceControlStatePtr SourceControlState = Provider.GetState(AssetPath, EStateCacheUsage::Use);
		if(!SourceControlState.IsValid())
		{
			return nullptr;
		}
		return StaticCastSharedPtr<FDiversionState>(SourceControlState);
	}
	catch (...)
	{
		return nullptr;
	}
}


const FSlateBrush* SPotentialClashIndicator::PotentialClashIcon = nullptr;

/**
* Construct this widget.
* @param InArgs Slate arguments
*/

void SPotentialClashIndicator::Construct(const FArguments& InArgs)
{
	AssetPath = InArgs._AssetPath;
	SetVisibility(MakeAttributeSP(this, &SPotentialClashIndicator::GetVisibility));

	ChildSlot
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 4
				.Padding(2.0f, 3.0f)
				[
					SNew(SBox)
						.WidthOverride(16.0f)
						.HeightOverride(16.0f)
#else
				.Padding(0.0f, 0.0f, 16.0f, 0.f)
				[
					SNew(SBox)
						.WidthOverride(16.0f)
						.HeightOverride(32.0f)
#endif
						[
							SNew(SImage).Image(this, &SPotentialClashIndicator::GetImageBrush)
						]
				]
		];
}

/** Caches the indicator brushes for access. */

void SPotentialClashIndicator::CacheIndicatorBrush()
{
	if (PotentialClashIcon == nullptr)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 4
		const FString RevisionControlClashIcon = FString(TEXT("RevisionControl.NotAtHeadRevision"));
#else
		const FString RevisionControlClashIcon = FString(TEXT("RevisionControl.ModifiedBadge"));
#endif
		PotentialClashIcon = FSlateIcon(FRevisionControlStyleManager::GetStyleSetName(), *RevisionControlClashIcon).GetIcon();
	}
}

bool SPotentialClashIndicator::IsAssetHasPotentialClashes(FString Path)
{
	TSharedPtr<FDiversionState, ESPMode::ThreadSafe> DiversionState = GetDiversionStateForAsset(Path);
	if (!DiversionState)
	{
		return false;
	}
	return DiversionState->PotentialClashes.GetPotentialClashesCount() > 0;
}

/**
* Construct this widget.
* @param InArgs Slate arguments
*/

void SPotentialClashTooltip::Construct(const FArguments& InArgs)
{
	AssetPath = InArgs._AssetPath;
	SetVisibility(MakeAttributeSP(this, &SPotentialClashTooltip::GetVisibility));

	ChildSlot
		[
			SNew(STextBlock)
				.Text(this, &SPotentialClashTooltip::GetTooltip)
		];
}

bool SPotentialClashTooltip::IsAssetHasPotentialClashes(FString Path)
{
	TSharedPtr<FDiversionState, ESPMode::ThreadSafe> DiversionState = GetDiversionStateForAsset(Path);
	if (!DiversionState)
	{
		return false;
	}
	return DiversionState->PotentialClashes.GetPotentialClashesCount() > 0;
}

FText SPotentialClashTooltip::GetTooltip() const
{
	TSharedPtr<FDiversionState, ESPMode::ThreadSafe> DiversionState = GetDiversionStateForAsset(AssetPath);
	if (!DiversionState)
	{
		return FText();
	}
	return FText::FromString(DiversionState->GetOtherEditorsList());
}
