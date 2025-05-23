// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionPotentialClashUI.h"

#include "DiversionState.h"
#include "ISourceControlState.h"
#include "ISourceControlModule.h"
#include "RevisionControlStyle/RevisionControlStyle.h"


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
				.Padding(2.0f, 3.0f)
				[
					SNew(SBox)
						.WidthOverride(16.0f)
						.HeightOverride(16.0f)
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
		PotentialClashIcon = FSlateIcon(FRevisionControlStyleManager::GetStyleSetName(), "RevisionControl.NotAtHeadRevision").GetIcon();
	}
}

bool SPotentialClashIndicator::IsAssetHasPotentialClashes(FString Path)
{
	FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(Path, EStateCacheUsage::Use);
	TSharedPtr<FDiversionState, ESPMode::ThreadSafe> DiversionState = StaticCastSharedPtr<FDiversionState>(SourceControlState);
	return DiversionState && DiversionState->PotentialClashes.GetPotentialClashesCount() > 0;
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
	FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(Path, EStateCacheUsage::Use);
	TSharedPtr<FDiversionState, ESPMode::ThreadSafe> DiversionState = StaticCastSharedPtr<FDiversionState>(SourceControlState);
	return DiversionState && DiversionState->PotentialClashes.GetPotentialClashesCount() > 0;
}

FText SPotentialClashTooltip::GetTooltip() const
{
	FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(AssetPath, EStateCacheUsage::Use);
	TSharedPtr<FDiversionState, ESPMode::ThreadSafe> DiversionState = StaticCastSharedPtr<FDiversionState>(SourceControlState);
	return DiversionState ? FText::FromString(DiversionState->GetOtherEditorsList()) : FText();
}
