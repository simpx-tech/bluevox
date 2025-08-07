// Fill out your copyright notice in the Description page of Project Settings.


#include "Chunk.h"

#include "ChunkRegistry.h"
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
	RootComponent = MeshComponent;
}

void AChunk::BeginPlay()
{
	Super::BeginPlay();
}

void AChunk::Th_BeginRender(const EChunkState State, FDynamicMesh3& OutMesh)
{
	const auto Start = FPlatformTime::Cycles64();
	
	const auto ChunkRegistry = GameManager->ChunkRegistry;
	const auto ShapeRegistry = GameManager->ShapeRegistry;
	
	ChunkRegistry->LockForRender(Position);

	const bool HasCollision = EnumHasAnyFlags(State, EChunkState::Collision);
	MeshComponent->SetCollisionEnabled(HasCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	
	const auto Visible = EnumHasAnyFlags(State, EChunkState::Rendered);
	MeshComponent->SetVisibility(Visible);
	
	if (RenderedAtDirtyChanges == Data->Changes)
	{
		return;
	}

	int32 ColumnIndex;
	int32 PiecesCount;
	int32 CurZ;
	int32 InternalZ;
	int32 AccumulatedSize = 0;
	int32 AccumulatedZ = -1;
	FColumnPosition CurPosition;
	
	TStaticArray<FRenderNeighbor, 4> Neighbors;
	
	for (int32 X = 0; X < GameRules::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameRules::Chunk::Size; ++Y)
		{
			ColumnIndex = X + Y * GameRules::Chunk::Size;
			PiecesCount = Data->Columns[ColumnIndex].Pieces.Num();
			CurPosition.X = X;
			CurPosition.Y = Y;
			CurZ = 0;
			InternalZ = 0;

			for (const auto Face : FaceUtils::AllHorizontalFaces)
			{
				const auto& Column = ChunkRegistry->Th_GetColumn(CurPosition +
					FaceUtils::GetHorizontalOffsetByFace(Face));
				auto& Neighbor = Neighbors[static_cast<uint8>(Face)];
				Neighbor.Column = &Column;
				Neighbor.Piece = &Column.Pieces[0];
				// TODO in future we would have more complex logic here
				Neighbor.Shape = ShapeRegistry->GetShapeById(Neighbor.Piece->Id);
				Neighbor.Start = 0;
				Neighbor.Size = Column.Pieces[0].GetSize();
				Neighbor.Index = 0;
			}
		
			for (int32 PieceIdx = 0; PieceIdx < PiecesCount; ++PieceIdx)
			{
				const auto& Piece = Data->Columns[ColumnIndex].Pieces[PieceIdx];
				const auto PieceSize = Piece.GetSize();
				const auto Shape = ShapeRegistry->GetShapeById(Piece.Id);

				// Hot path, skip void
				if (Piece.Id == GameRules::Constants::GShapeId_Void)
				{
					continue;
				}

				for (const EFace Face : FaceUtils::AllHorizontalFaces)
				{
					if (!Shape->IsOpaque(Face))
					{
						continue;
					}
					
					const auto OppositeFace = FaceUtils::GetOppositeFace(Face);

					auto& Neighbor = Neighbors[static_cast<uint8>(Face)];
					do
					{
						const auto NeighborIsOpaque = Neighbor.Shape->IsOpaque(OppositeFace);

						if (NeighborIsOpaque)
						{
							if (AccumulatedSize > 0)
							{
								Shape->Render(OutMesh, Face, FLocalPosition(CurPosition.X, CurPosition.Y, AccumulatedZ), AccumulatedSize, Piece.Id);
								
								AccumulatedSize = 0;
								AccumulatedZ = -1;
							}
						} else
						{
							if (AccumulatedZ == -1)
							{
								AccumulatedZ = InternalZ;
							}

							const auto End = FMath::Min(CurZ + PieceSize, Neighbor.Start + Neighbor.Size);
							AccumulatedSize += End - InternalZ;
						}

						Neighbor.Start += Neighbor.Size;
						InternalZ += Neighbor.Size;
						Neighbor.Index += 1;
						// Advance neighbor if we have more neighbor pieces
						if (Neighbor.Index < Neighbor.Column->Pieces.Num())
						{
							Neighbor.Size = Neighbor.Column->Pieces[Neighbor.Index].GetSize();
							Neighbor.Piece = &Neighbor.Column->Pieces[Neighbor.Index];
							Neighbor.Shape = ShapeRegistry->GetShapeById(Neighbor.Piece->Id);
						}

						if (AccumulatedSize > 0)
						{
							Shape->Render(OutMesh, Face, FLocalPosition(CurPosition.X, CurPosition.Y, AccumulatedZ), AccumulatedSize, Piece.Id);

							AccumulatedSize = 0;
							AccumulatedZ = -1;
						}
					} while (
						CurZ + PieceSize > Neighbor.Index + Neighbor.Size && 
						Neighbor.Index < Neighbor.Column->Pieces.Num()
					);
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
					Shape->Render(OutMesh, EFace::Top, FLocalPosition(CurPosition.X, CurPosition.Y, CurZ), PieceSize, Piece.Id);
				}
				
				// Bottom, only render if we are not on the first piece
				if (PieceIdx > 0)
				{
					const auto NeighborShape = ShapeRegistry->GetShapeById(Data->Columns[ColumnIndex].Pieces[PieceIdx - 1].Id);
					if (!NeighborShape->IsOpaque(EFace::Top))
					{
						Shape->Render(OutMesh, EFace::Bottom, FLocalPosition(CurPosition.X, CurPosition.Y, CurZ), PieceSize, Piece.Id);
					}
				}
				
				CurZ += Piece.GetSize();
			}
		}
	}

	RenderedAtDirtyChanges = Data->Changes;

	GameManager->ChunkRegistry->ReleaseForRender(Position);

	const auto End = FPlatformTime::Cycles64();
	UE_LOG(LogTemp, Log, TEXT("Chunk %s rendered in %f ms"), *Position.ToString(),
		FPlatformTime::ToMilliseconds64(End - Start));
}

void AChunk::CommitRender(FDynamicMesh3&& Mesh) const
{
	MeshComponent->SetMesh(MoveTemp(Mesh));
}

