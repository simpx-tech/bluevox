// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FaceVisibility.h"
#include "Bluevox/Utils/Face.h"
#include "UObject/Object.h"
#include "Shape.generated.h"

class UChunkData;
class AGameManager;
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
	virtual ~UShape() override;

	virtual void GenerateRenderGroups();

	FRenderGroup* NorthRenderGroup;
	FRenderGroup* SouthRenderGroup;
	FRenderGroup* WestRenderGroup;
	FRenderGroup* EastRenderGroup;
	FRenderGroup* TopRenderGroup;
	FRenderGroup* BottomRenderGroup;
	FRenderGroup* EverywhereRenderGroup;

	virtual uint16 GetMaterialId() const
	{
		return 0;
	}

	// DEV add constexpr values and also a dynamic path if needed
	// DEV add min/max values (?)
	
	/**
	 * With CanExpand <code>true</code> this will be rendered as a single entity, effectively using the Scalable properties (ex: ScalableVertices, etc.) 
	 */
	UPROPERTY()
	bool CanExpand = true;
	
public:
	virtual void GameTick(AGameManager* AGameManager, const FLocalPosition& Position, uint16 Height, UChunkData* WhereData, float DeltaTime) const;
	
	virtual FName GetNameId() const;
	
	// TODO cache result instead of call this every time (?)
	virtual void Render(UE::Geometry::FDynamicMesh3& Mesh, const EFace Face, const FLocalPosition& Position, int32 Size) const;

	UShape* InitializeData();

	virtual bool ShouldMerge() const;
	
	virtual bool ShouldTickOnPlace() const;
	
	virtual bool ShouldAlwaysTick() const;

	virtual bool ShouldTickOnLoad() const;

	virtual bool ShouldTickOnNeighborUpdate() const;

	FRenderGroup* GetRenderGroup(EFace Direction) const;
	
	virtual EFaceVisibility GetVisibility(EFace Face) const
	{
		return EFaceVisibility::Opaque;
	}
	
	virtual int32 GetMaterialCost() const
	{
		return 4;
	}
};
