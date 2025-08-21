// Fill out your copyright notice in the Description page of Project Settings.


#include "Chunk.h"

#include "ChunkRegistry.h"
#include "LogChunk.h"
#include "ChunkStats.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Shape/Shape.h"
#include "Bluevox/Shape/ShapeRegistry.h"
#include "Bluevox/Tick/TickManager.h"
#include "Bluevox/Utils/Face.h"
#include "Data/ChunkData.h"
#include "Position/LocalPosition.h"
#include "Runtime/GeometryFramework/Public/Components/DynamicMeshComponent.h"
#include "VirtualMap/ChunkTaskManager.h"

void AChunk::BeginDestroy()
{
	if (GameManager)
	{
		GameManager->TickManager->UnregisterUObjectTickable(this);	
	}
	
	Super::BeginDestroy();
}

// Sets default values
AChunk::AChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->bEnableComplexCollision = true;
	MeshComponent->bCastShadowAsTwoSided = true;
	MeshComponent->CollisionType = CTF_UseComplexAsSimple;
	MeshComponent->bUseAsyncCooking = true;
	MeshComponent->SetMobility(EComponentMobility::Type::Static);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	RootComponent = MeshComponent;

	WaterMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("WaterMeshComponent"));
	WaterMeshComponent->bCastShadowAsTwoSided = true;
	WaterMeshComponent->SetGenerateOverlapEvents(false);
	WaterMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	WaterMeshComponent->SetMobility(EComponentMobility::Type::Static);
	WaterMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	WaterMeshComponent->CollisionType = CTF_UseDefault;
	
	WaterMeshComponent->SetupAttachment(RootComponent);
}

// TODO also pass render state, so know if should tick or not (in case of live x lod chunks)
AChunk* AChunk::Init(const FChunkPosition InPosition, AGameManager* InGameManager,
	UChunkData* InData)
{
	Position = InPosition;
	GameManager = InGameManager;
	ChunkData = InData;

	const auto ShapeRegistry = GameManager->ShapeRegistry;

	for (int32 LocalX = 0; LocalX < GameConstants::Chunk::Size; ++LocalX)
	{
		for (int32 LocalY = 0; LocalY < GameConstants::Chunk::Size; ++LocalY)
		{
			const auto ColumnPos = FLocalColumnPosition(LocalX, LocalY);
			const auto& Column = ChunkData->GetColumn(ColumnPos);

			int32 CurZ = 0;
			for (const auto& Piece : Column.Pieces)
			{
				const auto Shape = ShapeRegistry->GetShapeById(Piece.Id);
				if (Shape->ShouldAlwaysTick())
				{
					AlwaysTick.Add(FLocalPosition(LocalX, LocalY, CurZ));
				} else if (Shape->ShouldTickOnLoad())
				{
					ChunkData->ScheduledToTick.Add(FLocalPosition(LocalX, LocalY, CurZ));
				}

				CurZ += Piece.Size;
			}
		}
	}

	GameManager->TickManager->RegisterUObjectTickable(this);
	
	return this;
}

void AChunk::GameTick(float DeltaTime)
{
	// DEV error here, can't modify the ScheduledToTick while iterating over it
	for (const auto& ScheduledToTickPos : ChunkData->ScheduledToTick)
	{
		const auto Piece = ChunkData->Th_GetPieceCopy(ScheduledToTickPos);
		const auto Shape = GameManager->ShapeRegistry->GetShapeById(Piece.Id);
		if (Shape)
		{
			Shape->GameTick(GameManager, ScheduledToTickPos, Piece.Size, ChunkData, DeltaTime);
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Shape %d not found for piece at %s in chunk %s"), Piece.Id, *ScheduledToTickPos.ToString(), *Position.ToString());
		}
	}
	ChunkData->ScheduledToTick.Reset();
	
	for (const auto& AlwaysTickPos : AlwaysTick)
	{
		const auto Piece = ChunkData->Th_GetPieceCopy(AlwaysTickPos);
		const auto Shape = GameManager->ShapeRegistry->GetShapeById(Piece.Id);
		if (Shape)
		{
			Shape->GameTick(GameManager, AlwaysTickPos, Piece.Size, ChunkData, DeltaTime);
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Shape %d not found for piece at %s in chunk %s"), Piece.Id, *AlwaysTickPos.ToString(), *Position.ToString());
		}
	}
}

void AChunk::SetRenderState(const EChunkState State) const
{
	UE_LOG(LogChunk, Verbose, TEXT("SetRenderState for chunk %s to %s"), *Position.ToString(), *UEnum::GetValueAsString(State));
	const auto Visible = EnumHasAllFlags(State, EChunkState::Visible);
	MeshComponent->SetVisibility(Visible);
	WaterMeshComponent->SetVisibility(Visible);

	// const auto Collision = EnumHasAllFlags(State, EChunkState::Collision);
	// MeshComponent->SetCollisionEnabled(Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

bool AChunk::BeginRender(FDynamicMesh3& OutMesh, FDynamicMesh3& OutWaterMesh)
{
	SCOPE_CYCLE_COUNTER(STAT_Chunk_BeginRender)
	UE_LOG(LogChunk, Verbose, TEXT("Th_BeginRender for chunk %s"), *Position.ToString());
	
	const auto Start = FPlatformTime::Cycles64();
	
	const auto ChunkRegistry = GameManager->ChunkRegistry;
	const auto ShapeRegistry = GameManager->ShapeRegistry;
	
	if (RenderedAtDirtyChanges.GetValue() == ChunkData->Changes)
	{
		return false;
	}
	
	OutMesh.EnableAttributes();
	OutMesh.Attributes()->SetNumUVLayers(2);
	OutMesh.Attributes()->DisablePrimaryColors();

	OutWaterMesh.EnableAttributes();
	OutWaterMesh.Attributes()->SetNumUVLayers(2);
	OutMesh.Attributes()->DisablePrimaryColors();
	
	FColumnPosition GlobalPos;
	
	TStaticArray<FRenderNeighbor, 4> Neighbors;

	const auto BaseChunkPosX = Position.X * GameConstants::Chunk::Size;
	const auto BaseChunkPosY = Position.Y * GameConstants::Chunk::Size;
	
	for (int32 LocalX = 0; LocalX < GameConstants::Chunk::Size; ++LocalX)
	{
		for (int32 LocalY = 0; LocalY < GameConstants::Chunk::Size; ++LocalY)
		{
			if (Position.X == 0 && Position.Y == 0 && LocalX == 2 && LocalY == 2)
			{
				UE_LOG(LogTemp, Warning, TEXT("breakpoint"));
			}
			
			SCOPE_CYCLE_COUNTER(STAT_Chunk_BeginRender_ProcessColumn)

			const int32 ColumnIndex = UChunkData::GetIndex(LocalX, LocalY);
			const int32 PiecesCount = ChunkData->Columns[ColumnIndex].Pieces.Num();
			GlobalPos.X = BaseChunkPosX + LocalX;
			GlobalPos.Y = BaseChunkPosY + LocalY;
			int32 CurZ = 0;

			for (const auto Face : FaceUtils::AllHorizontalFaces)
			{
				const auto& Column = ChunkRegistry->Th_GetColumn(GlobalPos +
					FaceUtils::GetHorizontalOffsetByFace(Face));
				auto& Neighbor = Neighbors[static_cast<uint8>(Face)];
				Neighbor.Column = &Column;
				Neighbor.Piece = &Column.Pieces[0];
				// TODO in future we would have more complex logic here
				Neighbor.Shape = ShapeRegistry->GetShapeById(Neighbor.Piece->Id);
				Neighbor.Start = 0;
				Neighbor.Size = Column.Pieces[0].Size;
				Neighbor.Index = 0;
			}
		
			for (int32 PieceIdx = 0; PieceIdx < PiecesCount; ++PieceIdx)
			{
				SCOPE_CYCLE_COUNTER(STAT_Chunk_BeginRender_ProcessPiece)
				
				const auto& Piece = ChunkData->Columns[ColumnIndex].Pieces[PieceIdx];
				const int32 PieceSize = Piece.Size;
				const auto Shape = ShapeRegistry->GetShapeById(Piece.Id);

				// Quick path, skip void
				if (Piece.Id == GameConstants::Shapes::GShapeId_Void)
				{
					for (const EFace Face : FaceUtils::AllHorizontalFaces)
					{
						auto& Neighbor = Neighbors[static_cast<uint8>(Face)];
						while (Neighbor.Index + 1 < Neighbor.Column->Pieces.Num() &&
							CurZ + PieceSize >= Neighbor.Start + Neighbor.Size)
						{
							Neighbor.Start += Neighbor.Size;
							Neighbor.Index += 1;
							Neighbor.Size = Neighbor.Column->Pieces[Neighbor.Index].Size;
							Neighbor.Piece = &Neighbor.Column->Pieces[Neighbor.Index];
							Neighbor.Shape = ShapeRegistry->GetShapeById(Neighbor.Piece->Id);
						}
					}
					CurZ += PieceSize;
					continue;
				}

				for (const EFace Face : FaceUtils::AllHorizontalFaces)
				{
					int32 AccumulatedSize = 0;
					int32 AccumulatedZ = -1;
					int32 InternalZ = 0;
					SCOPE_CYCLE_COUNTER(STAT_Chunk_BeginRender_ProcessPiece_AllHorizontalFaces)

					const auto ShapeVisibility = Shape->GetVisibility(Face);
					
					// Nothing to render on this face
					if (ShapeVisibility == EFaceVisibility::None)
					{
						continue;
					}
					
					const auto OppositeFace = FaceUtils::GetOppositeFace(Face);

					bool bContinue;
					auto& Neighbor = Neighbors[static_cast<uint8>(Face)];

					do
					{
						const auto NeighborIsOpaque = Neighbor.Shape->GetVisibility(OppositeFace) == EFaceVisibility::Opaque;

						const auto ShouldNotAccumulate = NeighborIsOpaque || (Piece.Id == Neighbor.Piece->Id && ShapeVisibility == EFaceVisibility::Transparent); 

						if (ShouldNotAccumulate)
						{
							if (AccumulatedSize > 0)
							{
								Shape->Render(ShapeVisibility == EFaceVisibility::Transparent ? OutWaterMesh : OutMesh, Face, FLocalPosition(LocalX, LocalY, AccumulatedZ), AccumulatedSize);
								
								AccumulatedSize = 0;
								AccumulatedZ = -1;
							}
						} else
						{
							if (AccumulatedZ == -1)
							{
								AccumulatedZ = CurZ + InternalZ;
							}

							const int32 End = FMath::Min(CurZ + PieceSize, Neighbor.Start + Neighbor.Size);
							AccumulatedSize += End - CurZ - InternalZ;
						}

						if (
							CurZ + PieceSize >= Neighbor.Start + Neighbor.Size &&
							Neighbor.Index + 1 < Neighbor.Column->Pieces.Num())
						{
							const auto CurIsAheadNeighborBy = CurZ > Neighbor.Start ? CurZ - Neighbor.Start : 0;
							InternalZ += Neighbor.Size - CurIsAheadNeighborBy;
							Neighbor.Start += Neighbor.Size;
							
							Neighbor.Index += 1;
							Neighbor.Size = Neighbor.Column->Pieces[Neighbor.Index].Size;
							Neighbor.Piece = &Neighbor.Column->Pieces[Neighbor.Index];
							Neighbor.Shape = ShapeRegistry->GetShapeById(Neighbor.Piece->Id);
							bContinue = true;
						} else
						{
							bContinue = false;
						}
					} while (bContinue);

					if (AccumulatedSize > 0)
					{
						Shape->Render(ShapeVisibility == EFaceVisibility::Transparent ? OutWaterMesh : OutMesh, Face, FLocalPosition(LocalX, LocalY, AccumulatedZ), AccumulatedSize);
					}
				}

				// Top
				bool bShouldRenderTop = true;
				if (PieceIdx < PiecesCount - 1)
				{
					const auto NeighborShape = ShapeRegistry->GetShapeById(ChunkData->Columns[ColumnIndex].Pieces[PieceIdx + 1].Id);
					bShouldRenderTop = NeighborShape->GetVisibility(EFace::Bottom) != EFaceVisibility::Opaque;
				}
				if (bShouldRenderTop)
				{
					Shape->Render( Shape->GetVisibility(EFace::Bottom) == EFaceVisibility::Transparent ? OutWaterMesh : OutMesh, EFace::Top, FLocalPosition(LocalX, LocalY, CurZ), PieceSize);
				}
				
				// Bottom, only render if we are not on the first piece
				if (PieceIdx > 0)
				{
					const auto NeighborShape = ShapeRegistry->GetShapeById(ChunkData->Columns[ColumnIndex].Pieces[PieceIdx - 1].Id);
					if (NeighborShape->GetVisibility(EFace::Top) != EFaceVisibility::Opaque)
					{
						Shape->Render(Shape->GetVisibility(EFace::Bottom) == EFaceVisibility::Transparent ? OutWaterMesh : OutMesh, EFace::Bottom, FLocalPosition(LocalX, LocalY, CurZ), PieceSize);
					}
				}
				
				CurZ += PieceSize;
			}
		}
	}

	RenderedAtDirtyChanges.Set(ChunkData->Changes);
	
	const auto End = FPlatformTime::Cycles64();
	UE_LOG(LogChunk, Verbose, TEXT("Chunk %s rendered in %f ms"), *Position.ToString(),
		FPlatformTime::ToMilliseconds64(End - Start));

	return true;
}

void AChunk::CommitRender(FRenderResult&& RenderResult) const
{
	UE_LOG(LogChunk, Verbose, TEXT("Committing render for chunk %s"), *Position.ToString());
	MeshComponent->SetMaterial(0, GameManager->ChunkMaterial);
	MeshComponent->SetMesh(MoveTemp(RenderResult.Mesh));

	WaterMeshComponent->SetMaterial(0, GameManager->ChunkMaterialTransparent);
	WaterMeshComponent->SetMesh(MoveTemp(RenderResult.WaterMesh));
}
