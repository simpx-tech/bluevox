#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Utils/Face.h"
#include "RenderGroup.generated.h"

/**
 * A render group is a group of vertices that are rendered together and specially <b>being
 * culled together</b>
 *
 * You can consider a render group as vertices which should be hided together,
 * generally a face touching other layers
 */
USTRUCT(BlueprintType)
struct FRenderGroup
{
	GENERATED_BODY()

	FRenderGroup()
	{
	}

	FRenderGroup(const uint32 InHideMask, const TArray<FVector>& InStaticVertices,
				 const TArray<FVector>& InScalableVertices, const TArray<uint16>& InTriangles)
		: HideCulling(InHideMask), StaticVertices(InStaticVertices),
		  ScalableVertices(InScalableVertices),
		  Triangles(InTriangles)
	{
	}

	UPROPERTY()
	EFace Direction = EFace::Anywhere;

	/**
	 * The culling pattern that hides this group
	 */
	UPROPERTY()
	uint32 HideCulling = 0;

	/**
	 * Vertices that stay the same independently of the size
	 */
	UPROPERTY()
	TArray<FVector> StaticVertices;

	/**
	 * Normals for each vertex, consider that static vertices are defined first
	 */
	UPROPERTY()
	TArray<FVector3f> Normals;

	/**
	 * UV's for each vertex, consider that static vertices are defined first
	 */
	UPROPERTY()
	TArray<FVector2f> UVs;

	/**
	 * Mark which UVs scale with the size
	 */
	UPROPERTY()
	TArray<bool> UVResizableMask;

	/**
	 * Vertices that change depending on the size
	 */
	UPROPERTY()
	TArray<FVector> ScalableVertices;

	/**
	 * The triangles that make the mesh, static vertices are defined first, then the scalable vertices
	 */
	UPROPERTY()
	TArray<uint16> Triangles;
};
