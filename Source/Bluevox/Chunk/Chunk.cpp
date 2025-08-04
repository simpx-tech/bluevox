// Fill out your copyright notice in the Description page of Project Settings.


#include "Chunk.h"

#include "Runtime/GeometryFramework/Public/Components/DynamicMeshComponent.h"


// Sets default values
AChunk::AChunk()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
}

// Called when the game starts or when spawned
void AChunk::BeginPlay()
{
	Super::BeginPlay();
}

void AChunk::BeginRender(FRenderChunkPayload&& Payload, FDynamicMesh3& OutMesh)
{
	// DEV check if state changed, do something
	// DEV also check if data is dirty and/or if the chunk is rendered
}

void AChunk::CommitRender(FDynamicMesh3&& Mesh)
{
	// DEV
}

