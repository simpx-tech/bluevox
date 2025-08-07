// Fill out your copyright notice in the Description page of Project Settings.


#include "LayerShape.h"

#include "Bluevox/Chunk/CullingMask.h"
#include "Bluevox/Chunk/RenderGroup.h"
#include "Bluevox/Chunk/Position/LocalPosition.h"
#include "Bluevox/Game/GameRules.h"
#include "Bluevox/ShapeMaterial/ShapeMaterialRegistry.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

FName ULayerShape::GetNameId() const
{
	return GameRules::Constants::GShape_Layer;
}

void ULayerShape::InitializeAllowedMaterials(UMaterialRegistry* Registry)
{
	using namespace GameRules::Constants;
	AllowedMaterials.Add(Registry->GetMaterialByName(GMaterial_Glass));
	AllowedMaterials.Add(Registry->GetMaterialByName(GMaterial_Stone));
	AllowedMaterials.Add(Registry->GetMaterialByName(GMaterial_Glass));
}

void ULayerShape::GenerateRenderGroups()
{
	const auto BlockSize = GameRules::Scaling::XYWorldSize;
	const auto LayerSize = GameRules::Scaling::ZSize;
	const auto LayerPercent = LayerSize / BlockSize;

	NorthRenderGroup = new FRenderGroup();
	NorthRenderGroup->Direction = EFace::North;
	NorthRenderGroup->StaticVertices = {
		FVector(BlockSize, 0, 0),
		FVector(BlockSize, BlockSize, 0),
	};
	NorthRenderGroup->ScalableVertices = {
		FVector(BlockSize, BlockSize, LayerSize),
		FVector(BlockSize, 0, LayerSize),
	};
	NorthRenderGroup->Triangles = {0, 2, 1, 0, 3, 2};
	NorthRenderGroup->Normals = {
		FVector3f(1, 0, 0),
		FVector3f(1, 0, 0),
		FVector3f(1, 0, 0),
		FVector3f(1, 0, 0)
	};
	NorthRenderGroup->HideCulling = static_cast<uint32>(ECullingMask::Layer);
	NorthRenderGroup->UVs = {
		FVector2f(0, 0),
		FVector2f(1, 0),
		FVector2f(1, LayerPercent),
		FVector2f(0, LayerPercent)
	};
	NorthRenderGroup->UVResizableMask = {
		false, false, true, true
	};

	EastRenderGroup = new FRenderGroup();
	EastRenderGroup->Direction = EFace::East;
	EastRenderGroup->StaticVertices = {
		FVector(BlockSize, BlockSize, 0),
		FVector(0, BlockSize, 0),
	};
	EastRenderGroup->ScalableVertices = {
		FVector(0, BlockSize, LayerSize),
		FVector(BlockSize, BlockSize, LayerSize),
	};
	EastRenderGroup->Triangles = {0, 2, 1, 0, 3, 2};
	EastRenderGroup->Normals = {
		FVector3f(0, 1, 0),
		FVector3f(0, 1, 0),
		FVector3f(0, 1, 0),
		FVector3f(0, 1, 0)
	};
	EastRenderGroup->HideCulling = static_cast<uint32>(ECullingMask::Layer);
	EastRenderGroup->UVs = {
		FVector2f(0, 0),
		FVector2f(1, 0),
		FVector2f(1, LayerPercent),
		FVector2f(0, LayerPercent)
	};
	EastRenderGroup->UVResizableMask = {
		false, false, true, true
	};

	WestRenderGroup = new FRenderGroup();
	WestRenderGroup->Direction = EFace::West;
	WestRenderGroup->StaticVertices = {
		FVector(0, 0, 0),
		FVector(BlockSize, 0, 0),
	};
	WestRenderGroup->ScalableVertices = {
		FVector(BlockSize, 0, LayerSize),
		FVector(0, 0, LayerSize),
	};
	WestRenderGroup->Triangles = {0, 2, 1, 0, 3, 2};
	WestRenderGroup->Normals = {
		FVector3f(0, -1, 0),
		FVector3f(0, -1, 0),
		FVector3f(0, -1, 0),
		FVector3f(0, -1, 0)
	};
	WestRenderGroup->HideCulling = static_cast<uint32>(ECullingMask::Layer);
	WestRenderGroup->UVs = {
		FVector2f(0, 0),
		FVector2f(1, 0),
		FVector2f(1, LayerPercent),
		FVector2f(0, LayerPercent)
	};
	WestRenderGroup->UVResizableMask = {
		false, false, true, true
	};

	SouthRenderGroup = new FRenderGroup();
	SouthRenderGroup->Direction = EFace::South;
	SouthRenderGroup->StaticVertices = {
		FVector(0, BlockSize, 0),
		FVector(0, 0, 0),
	};
	SouthRenderGroup->ScalableVertices = {
		FVector(0, 0, LayerSize),
		FVector(0, BlockSize, LayerSize),
	};
	SouthRenderGroup->Triangles = {0, 2, 1, 0, 3, 2};
	SouthRenderGroup->Normals = {
		FVector3f(-1, 0, 0),
		FVector3f(-1, 0, 0),
		FVector3f(-1, 0, 0),
		FVector3f(-1, 0, 0)
	};
	SouthRenderGroup->HideCulling = static_cast<uint32>(ECullingMask::Layer);
	SouthRenderGroup->UVs = {
		FVector2f(0, 0),
		FVector2f(1, 0),
		FVector2f(1, LayerPercent),
		FVector2f(0, LayerPercent)
	};
	SouthRenderGroup->UVResizableMask = {
		false, false, true, true
	};

	TopRenderGroup = new FRenderGroup();
	TopRenderGroup->Direction = EFace::Top;
	TopRenderGroup->ScalableVertices = {
		FVector(0, 0, LayerSize),
		FVector(BlockSize, 0, LayerSize),
		FVector(BlockSize, BlockSize, LayerSize),
		FVector(0, BlockSize, LayerSize)
	};
	TopRenderGroup->Triangles = {0, 2, 1, 0, 3, 2};
	TopRenderGroup->Normals = {
		FVector3f(0, 0, 1),
		FVector3f(0, 0, 1),
		FVector3f(0, 0, 1),
		FVector3f(0, 0, 1)
	};
	TopRenderGroup->HideCulling = static_cast<uint32>(ECullingMask::Layer);
	TopRenderGroup->UVs = {
		FVector2f(0, 0),
		FVector2f(0, 1),
		FVector2f(1, 1),
		FVector2f(1, 0)
	};
	TopRenderGroup->UVResizableMask = {
		false, false, false, false
	};

	BottomRenderGroup = new FRenderGroup();
	BottomRenderGroup->Direction = EFace::Bottom;
	BottomRenderGroup->StaticVertices = {
		FVector(0, 0, 0),
		FVector(0, BlockSize, 0),
		FVector(BlockSize, BlockSize, 0),
		FVector(BlockSize, 0, 0)
	};
	BottomRenderGroup->Triangles = {0, 2, 1, 0, 3, 2};
	BottomRenderGroup->Normals = {
		FVector3f(0, 0, -1),
		FVector3f(0, 0, -1),
		FVector3f(0, 0, -1),
		FVector3f(0, 0, -1)
	};
	BottomRenderGroup->HideCulling = static_cast<uint32>(ECullingMask::Layer);
	BottomRenderGroup->UVs = {
		FVector2f(0, 0),
		FVector2f(0, 1),
		FVector2f(1, 1),
		FVector2f(1, 0)
	};
	BottomRenderGroup->UVResizableMask = {
		false, false, false, false
	};
}

