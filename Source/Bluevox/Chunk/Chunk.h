// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Tick/GameTickable.h"
#include "Data/Piece.h"
#include "Position/ChunkPosition.h"
#include "Position/LocalPosition.h"
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

struct FRenderNeighbor {
	const FChunkColumn* Column = nullptr;
	const FPiece* Piece = nullptr;
	const UShape* Shape = nullptr;
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

	UPROPERTY(EditAnywhere)
	UDynamicMeshComponent* WaterMeshComponent = nullptr;

	FThreadSafeCounter RenderedAtDirtyChanges = -1;

	UPROPERTY()
	AGameManager* GameManager = nullptr;

	UPROPERTY(EditAnywhere)
	FChunkPosition Position;

	UPROPERTY()
	TArray<FLocalPosition> AlwaysTick;

public:
	virtual void BeginDestroy() override;
	
	// Sets default values for this actor's properties
	AChunk();
	
	AChunk* Init(const FChunkPosition InPosition, AGameManager* InGameManager, UChunkData* InData);
	
	UPROPERTY()
	UChunkData* ChunkData;

	virtual void GameTick(float DeltaTime) override;

	void SetRenderState(EChunkState State) const;
	
	bool BeginRender(UE::Geometry::FDynamicMesh3& OutMesh, UE::Geometry::FDynamicMesh3& OutWaterMesh);

	void CommitRender(FRenderResult&& RenderResult) const;
};
