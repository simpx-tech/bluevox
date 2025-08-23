// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterShape.h"

#include "Algo/RandomShuffle.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/GlobalPosition.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Shape/ShapeRegistry.h"

void UWaterShape::GameTick(AGameManager* GameManager, const FLocalPosition& Position,
                           const uint16 Height, UChunkData* OriginChunkData, float DeltaTime) const
{
	const auto GlobalPos = FGlobalPosition::FromLocalPosition(OriginChunkData->Position, Position);
	const auto WaterId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Water);

	// We can only spread to the bottom if above 0
	if (Position.Z > 0)
	{
		const auto BottomLocalPos = Position - FLocalPosition(0, 0, 1);
	
		const auto Bottom = OriginChunkData->Th_GetPieceCopy(BottomLocalPos);
	
		if (Bottom.Id == GameConstants::Shapes::GShapeId_Void)
		{
			const auto BottomGlobalPos = GlobalPos - FGlobalPosition(0, 0, 1);
			
			// Automatically re-schedule game tick since the SetPiece schedules a new tick (on place)
			GameManager->ChunkRegistry->SetPiece(GlobalPos + FGlobalPosition(0, 0, Height - 1),
				FPiece{GameConstants::Shapes::GShapeId_Void, 1});
			GameManager->ChunkRegistry->SetPiece(BottomGlobalPos, FPiece{WaterId, Height});
			return;
		}
	}

	// Can't spread anymore
	if (Height <= 1)
	{
		return;
	}

	// FGlobalPosition CurrentPosition = GlobalPos;
	// const auto EndZ = Position.Z + Height;
	//
	// while (CurrentPosition.Z < EndZ)
	// {
	// 	TArray<FGlobalPosition> PossibleSpreadNeighbors;
	// 	for (const auto& Direction : FaceUtils::AllHorizontalFaces)
	// 	{
	// 		const auto NeighborPos = CurrentPosition + FaceUtils::GetOffsetByFace(Direction);
	// 		const auto ChunkPos = FChunkPosition::FromGlobalPosition(NeighborPos);
	// 		const auto NeighborLocalPos = FLocalPosition::FromGlobalPosition(NeighborPos);
	// 		const auto Neighbor = GameManager->ChunkRegistry->Th_GetChunkData(ChunkPos)->Th_GetPieceCopy(NeighborLocalPos);
	// 		if (Neighbor.Start <= CurrentPosition.Z && Neighbor.Id == GameConstants::Shapes::GShapeId_Void)
	// 		{
	// 			PossibleSpreadNeighbors.Add(NeighborPos);
	// 		}
	// 	}
	// 	
	// 	if (PossibleSpreadNeighbors.Num() > 0)
	// 	{
	// 		Algo::RandomShuffle(PossibleSpreadNeighbors);
	// 		const auto HowMuchCanSpread = EndZ - CurrentPosition.Z;
	// 		const int WaterToSpreadCount = FMath::Min(HowMuchCanSpread, PossibleSpreadNeighbors.Num());
	//
	// 		for (int i = 0; i < HowMuchCanSpread && i < PossibleSpreadNeighbors.Num(); i++)
	// 		{
	// 			GameManager->ChunkRegistry->SetPiece(PossibleSpreadNeighbors[i], FPiece{WaterId, 1});
	// 		}
	//
	// 		// Remove from the source
	// 		GameManager->ChunkRegistry->SetPiece(CurrentPosition + FGlobalPosition(0, 0, Height - WaterToSpreadCount),
	// 			FPiece{GameConstants::Shapes::GShapeId_Void, static_cast<unsigned short>(WaterToSpreadCount)});
	// 		
	// 		// Can't spread anymore, don't tick
	// 		if (PossibleSpreadNeighbors.Num() >= HowMuchCanSpread)
	// 		{
	// 			return;
	// 		}
	//
	// 		// We have more to spread and haven't reached the end, so schedule another tick
	// 		OriginChunkData->ScheduledToTick.Add(Position);
	// 		
	// 		return;
	// 	}
	// 	
	// 	CurrentPosition.Z++;
	// }
}
