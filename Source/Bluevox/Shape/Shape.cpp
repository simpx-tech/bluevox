// Fill out your copyright notice in the Description page of Project Settings.


#include "Shape.h"

#include "Bluevox/Chunk/ChunkStats.h"
#include "Bluevox/Chunk/RenderGroup.h"
#include "Bluevox/Chunk/Position/LocalPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "Components/BaseDynamicMeshComponent.h"

FName UShape::GetNameId() const
{
	UE_LOG(LogTemp, Fatal, TEXT("UShape::GetNameId not implemented for %s"), *GetName());
	return NAME_None;
}

UShape* UShape::InitializeData()
{
	GenerateRenderGroups();

	if (NorthRenderGroup != nullptr)
	{
		RenderGroups.Emplace(NorthRenderGroup);
	}
	if (SouthRenderGroup != nullptr)
	{
		RenderGroups.Emplace(SouthRenderGroup);
	}
	if (WestRenderGroup != nullptr)
	{
		RenderGroups.Emplace(WestRenderGroup);
	}
	if (EastRenderGroup != nullptr)
	{
		RenderGroups.Emplace(EastRenderGroup);
	}
	if (TopRenderGroup != nullptr)
	{
		RenderGroups.Emplace(TopRenderGroup);
	}
	if (BottomRenderGroup != nullptr)
	{
		RenderGroups.Emplace(BottomRenderGroup);
	}
	if (EverywhereRenderGroup != nullptr)
	{
		RenderGroups.Emplace(EverywhereRenderGroup);
	}

	return this;
}

FRenderGroup* UShape::GetRenderGroup(const EFace Direction) const
{
	switch (Direction)
	{
	case EFace::North:
		return NorthRenderGroup;
	case EFace::South:
		return SouthRenderGroup;
	case EFace::East:
		return EastRenderGroup;
	case EFace::West:
		return WestRenderGroup;
	case EFace::Top:
		return TopRenderGroup;
	case EFace::Bottom:
		return BottomRenderGroup;
	default:
		return nullptr;
	}
}

UShape::~UShape()
{
	if (NorthRenderGroup != nullptr)
	{
		delete NorthRenderGroup;
	}
	if (SouthRenderGroup != nullptr)
	{
		delete SouthRenderGroup;
	}
	if (EastRenderGroup != nullptr)
	{
		delete EastRenderGroup;
	}
	if (WestRenderGroup != nullptr)
	{
		delete WestRenderGroup;
	}
	if (TopRenderGroup != nullptr)
	{
		delete TopRenderGroup;
	}
	if (BottomRenderGroup != nullptr)
	{
		delete BottomRenderGroup;
	}
	if (EverywhereRenderGroup != nullptr)
	{
		delete EverywhereRenderGroup;
	}
}

void UShape::GenerateRenderGroups()
{
}

void UShape::Render(FDynamicMesh3& Mesh, const EFace Face,
	const FLocalPosition& Position, const int32 Size) const
{
	SCOPE_CYCLE_COUNTER(STAT_Shape_Render);
	
	const auto RenderGroup = GetRenderGroup(Face);
	if (RenderGroup == nullptr) 
	{
		return;
	}

	const auto MaterialId = GetMaterialId();
	
	const auto SizeVector = FVector{1, 1, static_cast<double>(Size)};
	const auto PositionVector = FVector{
		Position.X * GameConstants::Scaling::XYWorldSize,
		Position.Y * GameConstants::Scaling::XYWorldSize,
		Position.Z * GameConstants::Scaling::ZSize
	};

	const auto NormalsOverlay = Mesh.Attributes()->PrimaryNormals();
	const auto UVOverlay = Mesh.Attributes()->PrimaryUV();
	const auto UV1Overlay = Mesh.Attributes()->GetUVLayer(1);
	const auto VertexOffset = Mesh.VertexCount();
	int VertexIndex = 0;
	for (int i = 0; i < RenderGroup->StaticVertices.Num(); i++)
	{
		Mesh.AppendVertex(RenderGroup->StaticVertices[i] + PositionVector);
		NormalsOverlay->AppendElement(RenderGroup->Normals[VertexIndex]);
		UVOverlay->AppendElement(RenderGroup->UVs[VertexIndex]);
		UV1Overlay->AppendElement(FVector2f{static_cast<float>(MaterialId), 0});
		VertexIndex++;
	}

	for (int i = 0; i < RenderGroup->ScalableVertices.Num(); i++)
	{
		Mesh.AppendVertex(RenderGroup->ScalableVertices[i] * SizeVector + PositionVector);
		NormalsOverlay->AppendElement(RenderGroup->Normals[VertexIndex]);
		UVOverlay->AppendElement(
			RenderGroup->UVs[VertexIndex] * (RenderGroup->UVResizableMask[VertexIndex]
				                           ? FVector2f{1, static_cast<float>(Size)}
				                           : FVector2f{1, 1}));
		UV1Overlay->AppendElement(FVector2f{static_cast<float>(MaterialId), 0});
		VertexIndex++;
	}

	for (int i = 0; i < RenderGroup->Triangles.Num(); i += 3)
	{
		const auto Tri = Mesh.AppendTriangle(RenderGroup->Triangles[i] + VertexOffset,
		                                     RenderGroup->Triangles[i + 1] + VertexOffset,
		                                     RenderGroup->Triangles[i + 2] + VertexOffset);
		NormalsOverlay->SetTriangle(Tri, UE::Geometry::FIndex3i(
			                            RenderGroup->Triangles[i] + VertexOffset,
			                            RenderGroup->Triangles[i + 1] + VertexOffset,
			                            RenderGroup->Triangles[i + 2] + VertexOffset));
		UVOverlay->SetTriangle(Tri, UE::Geometry::FIndex3i(
			                       RenderGroup->Triangles[i] + VertexOffset,
			                       RenderGroup->Triangles[i + 1] + VertexOffset,
			                       RenderGroup->Triangles[i + 2] + VertexOffset));
		UV1Overlay->SetTriangle(Tri, UE::Geometry::FIndex3i(
			                        RenderGroup->Triangles[i] + VertexOffset,
			                        RenderGroup->Triangles[i + 1] + VertexOffset,
			                        RenderGroup->Triangles[i + 2] + VertexOffset));
	}
}