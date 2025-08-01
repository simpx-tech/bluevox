// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VirtualMap/ChunkState.h"
#include "Chunk.generated.h"

class UChunkData;
class UDynamicMeshComponent;

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

	UPROPERTY()
	bool bRendered = false;

	UPROPERTY()
	UChunkData* Data;

	UPROPERTY()
	uint32 RenderedAtDirtyChanges = 0;

	UFUNCTION()
	void Render(EChunkState State);
};
