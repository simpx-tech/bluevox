// Fill out your copyright notice in the Description page of Project Settings.


#include "InstanceTypeDataAsset.h"

#include "Bluevox/Game/GameConstants.h"

FPrimaryAssetId UInstanceTypeDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId("InstanceType", GetFName());
}

void UInstanceTypeDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.GetPropertyName();
	const bool bAffectsSizing = PropName == GET_MEMBER_NAME_CHECKED(UInstanceTypeDataAsset, StaticMesh);

	if (bAffectsSizing && StaticMesh)
	{
		float MinSpacingBlocks = Radius;
		int32 VoidBlocks = Height;

		EstimateInstanceSpacingFromMesh(
			MinSpacingBlocks,
			VoidBlocks
		);

		Radius = MinSpacingBlocks;
		Height = VoidBlocks;
		MarkPackageDirty();
	}
}
