// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Position/ChunkPosition.h"
#include "Chunk.generated.h"

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
class BLUEVOX_API AChunk : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AChunk();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	UDynamicMeshComponent* MeshComponent = nullptr;

	FThreadSafeCounter RenderedAtDirtyChanges = -1;

	UPROPERTY()
	AGameManager* GameManager = nullptr;

	UPROPERTY()
	FChunkPosition Position;

public:
	AChunk* Init(const FChunkPosition InPosition, AGameManager* InGameManager, UChunkData* InData)
	{
		Position = InPosition;
		GameManager = InGameManager;
		Data = InData;
		return this;
	}
	
	UPROPERTY()
	UChunkData* Data;
	
	void Th_BeginRender(UE::Geometry::FDynamicMesh3& OutMesh);

	void CommitRender(UE::Geometry::FDynamicMesh3&& Mesh) const;
};
