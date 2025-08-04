// Fill out your copyright notice in the Description page of Project Settings.


#include "Chunk.h"

#include "ChunkRegistry.h"
#include "Bluevox/Game/GameManager.h"
#include "Data/ChunkData.h"
#include "Runtime/GeometryFramework/Public/Components/DynamicMeshComponent.h"


// Sets default values
AChunk::AChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
}

void AChunk::BeginPlay()
{
	Super::BeginPlay();
}

void AChunk::Th_BeginRender(const EChunkState State, FDynamicMesh3& OutMesh)
{
	GameManager->ChunkRegistry->LockForRender(Position);

	const bool HasCollision = EnumHasAnyFlags(State, EChunkState::Collision);
	MeshComponent->SetCollisionEnabled(HasCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	
	const auto Visible = EnumHasAnyFlags(State, EChunkState::Rendered);
	MeshComponent->SetVisibility(Visible);
	
	if (RenderedAtDirtyChanges == Data->Changes)
	{
		return;
	}

	// DEV do the rendering itself

	RenderedAtDirtyChanges = Data->Changes;

	GameManager->ChunkRegistry->ReleaseForRender(Position);
}

void AChunk::CommitRender(FDynamicMesh3&& Mesh) const
{
	MeshComponent->SetMesh(MoveTemp(Mesh));
}

