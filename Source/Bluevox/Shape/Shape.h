// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Utils/Face.h"
#include "UObject/Object.h"
#include "Shape.generated.h"

struct FRenderGroup;
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

	friend class UShapeRegistry;

protected:
	UPROPERTY()
	TArray<int32> AllowedMaterials;

	virtual ~UShape() override;

	virtual void GenerateRenderGroups();

	FRenderGroup* NorthRenderGroup;
	FRenderGroup* SouthRenderGroup;
	FRenderGroup* WestRenderGroup;
	FRenderGroup* EastRenderGroup;
	FRenderGroup* TopRenderGroup;
	FRenderGroup* BottomRenderGroup;
	FRenderGroup* EverywhereRenderGroup;

	TArray<FRenderGroup*> RenderGroups;

	static inline void AddRenderToMesh(const FRenderGroup& Render, UE::Geometry::FDynamicMesh3& Mesh,
								const FLocalPosition& InPosition, uint16 Size, uint16 MaterialId);

	// DEV add constexpr values and also a dynamic path if needed
	// DEV add min/max values (?)
	
	/**
	 * With CanExpand <code>true</code> this will be rendered as a single entity, effectively using the Scalable properties (ex: ScalableVertices, etc.) 
	 */
	UPROPERTY()
	bool CanExpand = true;
	
public:
	virtual FName GetNameId() const;
	
	// TODO cache result instead of call this every time (?)
	virtual void Render(UE::Geometry::FDynamicMesh3& Mesh, const EFace Face, const FLocalPosition& Position, int32 Size, int32 MaterialId) const;

	UShape* InitializeData();

	FRenderGroup* GetRenderGroup(EFace Direction) const;

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
