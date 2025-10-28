// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Game/VoxelMaterial.h"
#include "Bluevox/Tick/GameTickable.h"
#include "Data/InstanceData.h"
#include "Data/Piece.h"
#include "Position/ChunkPosition.h"
#include "VirtualMap/ChunkState.h"
#include "Chunk.generated.h"

struct FRenderResult;
struct FRenderGroup;
struct FChunkColumn;
class UShape;
struct FPiece;
class AGameManager;
struct FRenderChunkPayload;

namespace UE::Geometry
{
	class FDynamicMesh3;
}

class UChunkData;
class UDynamicMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;

struct FRenderNeighbor {
	const FChunkColumn* Column = nullptr;
	const FPiece* Piece = nullptr;
	EMaterial MaterialId = EMaterial::Void;
	int32 Start = 0;
	int32 Size = 0;
	int32 Index = 0;
};

UCLASS()
class BLUEVOX_API AChunk : public AActor, public IGameTickable
{
	GENERATED_BODY()

	friend class UChunkRegistry;
	
protected:
	UPROPERTY(EditAnywhere)
	UDynamicMeshComponent* MeshComponent = nullptr;

	UPROPERTY()
	TMap<FPrimaryAssetId, UHierarchicalInstancedStaticMeshComponent*> ChunkInstanceComponents;

	FThreadSafeCounter RenderedAtDirtyChanges = -1;

	UPROPERTY()
	AGameManager* GameManager = nullptr;

	UPROPERTY(EditAnywhere)
	FChunkPosition Position;

public:
	virtual void BeginDestroy() override;
	
	// Sets default values for this actor's properties
	AChunk();
	
	AChunk* Init(const FChunkPosition InPosition, AGameManager* InGameManager, UChunkData* InData);
	
	UPROPERTY()
	UChunkData* ChunkData;

	void SetRenderState(EChunkState State) const;
	
	bool BeginRender(UE::Geometry::FDynamicMesh3& OutMesh, bool bForceRender = false);

	void CommitRender(FRenderResult&& RenderResult) const;

public:
	// Update instance visibility for clients (hides instances where entities exist)
	void RefreshInstanceVisibility();

	// Force update instance rendering
	void UpdateInstanceRendering();

private:
	// Track which instances are currently visible (client-side)
	// Key: AssetId, Value: Set of visible instance indices
	TMap<FPrimaryAssetId, TSet<int32>> VisibleInstanceIndices;

	// Track if instances have been initially rendered
	bool bInstancesInitialized = false;

	UHierarchicalInstancedStaticMeshComponent* GetOrCreateInstanceComponent(
		const FPrimaryAssetId& AssetId,
		class UInstanceTypeDataAsset* Asset);

	// Check if running as client-only
	bool IsClientOnly() const;
};
