// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Utils/Face.h"
#include "UObject/Object.h"
#include "Shape.generated.h"

class UMaterialRegistry;

namespace UE::Geometry
{
	class FDynamicMesh3;
}

struct FLocalPosition;
/**
 * 
 */
UCLASS()
class BLUEVOX_API UShape : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	TArray<int32> AllowedMaterials;

public:
	virtual FName GetNameId() const;
	
	// TODO cache result instead of call this every time (?)
	void Render(UE::Geometry::FDynamicMesh3& Mesh, const EFace Face, const FLocalPosition& Position, int32 Size, int32 MaterialId) const;

	virtual void InitializeAllowedMaterials(UMaterialRegistry* Registry);

	bool IsMaterialAllowed(const int32 MaterialId) const
	{
		return AllowedMaterials.Contains(MaterialId);
	}

	virtual bool IsOpaque(EFace Face) const
	{
		return true;
	}
	
	virtual int32 GetMaterialCost() const
	{
		return 4;
	}
};
