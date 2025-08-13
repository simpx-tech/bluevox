// Fill out your copyright notice in the Description page of Project Settings.


#include "Chunk.h"

#include "ChunkRegistry.h"
#include "LogChunk.h"
#include "ChunkStats.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Shape/Shape.h"
#include "Bluevox/Shape/ShapeRegistry.h"
#include "Bluevox/Utils/Face.h"
#include "Data/ChunkData.h"
#include "Position/LocalPosition.h"
#include "Runtime/GeometryFramework/Public/Components/DynamicMeshComponent.h"


// Sets default values
AChunk::AChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->bEnableComplexCollision = true;
	MeshComponent->bCastShadowAsTwoSided = true;
	MeshComponent->CollisionType = CTF_UseComplexAsSimple;

	RootComponent = MeshComponent;
}

void AChunk::SetRenderState(const EChunkState State) const
{
	UE_LOG(LogChunk, Verbose, TEXT("SetRenderState for chunk %s to %s"), *Position.ToString(), *UEnum::GetValueAsString(State));
	const auto Visible = EnumHasAllFlags(State, EChunkState::Visible);
	MeshComponent->SetVisibility(Visible);

	const auto Collision = EnumHasAllFlags(State, EChunkState::Collision);
	MeshComponent->SetCollisionEnabled(Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

bool AChunk::Th_BeginRender(FDynamicMesh3& OutMesh)
{
	SCOPE_CYCLE_COUNTER(STAT_Chunk_BeginRender)
	UE_LOG(LogChunk, Verbose, TEXT("Th_BeginRender for chunk %s"), *Position.ToString());
	
	const auto Start = FPlatformTime::Cycles64();
	
	const auto ChunkRegistry = GameManager->ChunkRegistry;
	const auto ShapeRegistry = GameManager->ShapeRegistry;
	
	if (RenderedAtDirtyChanges.GetValue() == Data->Changes)
	{
		return false;
	}
	
	OutMesh.EnableAttributes();
	OutMesh.Attributes()->SetNumUVLayers(2);
	OutMesh.Attributes()->EnablePrimaryColors();
	
	FColumnPosition GlobalPos;
	
	TStaticArray<FRenderNeighbor, 4> Neighbors;

	const auto BaseChunkPosX = Position.X * GameRules::Chunk::Size;
	const auto BaseChunkPosY = Position.Y * GameRules::Chunk::Size;
	
	for (int32 LocalX = 0; LocalX < GameRules::Chunk::Size; ++LocalX)
	{
		for (int32 LocalY = 0; LocalY < GameRules::Chunk::Size; ++LocalY)
		{
			if (Position.X == 0 && Position.Y == 0 && LocalX == 2 && LocalY == 2)
			{
				UE_LOG(LogTemp, Warning, TEXT("breakpoint"));
			}
			
			SCOPE_CYCLE_COUNTER(STAT_Chunk_BeginRender_ProcessColumn)

			const int32 ColumnIndex = UChunkData::GetIndex(LocalX, LocalY);
			const int32 PiecesCount = Data->Columns[ColumnIndex].Pieces.Num();
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
				
				const auto& Piece = Data->Columns[ColumnIndex].Pieces[PieceIdx];
				const int32 PieceSize = Piece.Size;
				const auto Shape = ShapeRegistry->GetShapeById(Piece.Id);

				// Quick path, skip void
				if (Piece.Id == GameRules::Constants::GShapeId_Void)
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
					
					if (!Shape->IsOpaque(Face))
					{
						continue;
					}
					
					const auto OppositeFace = FaceUtils::GetOppositeFace(Face);

					bool bContinue;
					auto& Neighbor = Neighbors[static_cast<uint8>(Face)];

					do
					{
						const auto NeighborIsOpaque = Neighbor.Shape->IsOpaque(OppositeFace);

						if (NeighborIsOpaque)
						{
							if (AccumulatedSize > 0)
							{
								Shape->Render(OutMesh, Face, FLocalPosition(LocalX, LocalY, AccumulatedZ), AccumulatedSize);
								
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
						Shape->Render(OutMesh, Face, FLocalPosition(LocalX, LocalY, AccumulatedZ), AccumulatedSize);
					}
				}

				// Top
				bool bShouldRenderTop = true;
				if (PieceIdx < PiecesCount - 1)
				{
					const auto NeighborShape = ShapeRegistry->GetShapeById(Data->Columns[ColumnIndex].Pieces[PieceIdx + 1].Id);
					bShouldRenderTop = !NeighborShape->IsOpaque(EFace::Bottom);
				}
				if (bShouldRenderTop)
				{
					Shape->Render(OutMesh, EFace::Top, FLocalPosition(LocalX, LocalY, CurZ), PieceSize);
				}
				
				// Bottom, only render if we are not on the first piece
				if (PieceIdx > 0)
				{
					const auto NeighborShape = ShapeRegistry->GetShapeById(Data->Columns[ColumnIndex].Pieces[PieceIdx - 1].Id);
					if (!NeighborShape->IsOpaque(EFace::Top))
					{
						Shape->Render(OutMesh, EFace::Bottom, FLocalPosition(LocalX, LocalY, CurZ), PieceSize);
					}
				}
				
				CurZ += PieceSize;
			}
		}
	}

	RenderedAtDirtyChanges.Set(Data->Changes);
	
	const auto End = FPlatformTime::Cycles64();
	UE_LOG(LogChunk, Verbose, TEXT("Chunk %s rendered in %f ms"), *Position.ToString(),
		FPlatformTime::ToMilliseconds64(End - Start));

	return true;
}

void AChunk::CommitRender(FDynamicMesh3&& Mesh) const
{
	UE_LOG(LogChunk, Verbose, TEXT("Committing render for chunk %s"), *Position.ToString());
	MeshComponent->SetMaterial(0, GameManager->ChunkMaterial);
	MeshComponent->SetMesh(MoveTemp(Mesh));
}
