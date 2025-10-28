// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemTypeDataAsset.h"

UStaticMesh* UItemTypeDataAsset::GetMeshForStackAmount(int32 StackAmount) const
{
	if (StackMeshes.Num() == 0)
	{
		return nullptr;
	}

	// Find the appropriate mesh by finding the highest threshold that doesn't exceed the stack amount
	UStaticMesh* SelectedMesh = nullptr;
	int32 HighestValidThreshold = 0;

	for (const FStackMeshThreshold& Threshold : StackMeshes)
	{
		if (Threshold.MinStackAmount <= StackAmount && Threshold.MinStackAmount > HighestValidThreshold)
		{
			HighestValidThreshold = Threshold.MinStackAmount;
			SelectedMesh = Threshold.Mesh;
		}
	}

	// If no valid threshold found, use the first mesh as fallback
	return SelectedMesh != nullptr ? SelectedMesh : StackMeshes[0].Mesh;
}
