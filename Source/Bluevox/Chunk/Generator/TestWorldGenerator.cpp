// Fill out your copyright notice in the Description page of Project Settings.


#include "TestWorldGenerator.h"

#include "FastNoiseWrapper.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/GameRules.h"
#include "Bluevox/Shape/ShapeRegistry.h"

UTestWorldGenerator::UTestWorldGenerator()
{
	Noise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("Noise"));
	Noise->SetupFastNoise(EFastNoise_NoiseType::Perlin, 123, 0.03f);
}

void UTestWorldGenerator::GenerateChunk(const FChunkPosition& Position,
                                        TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameRules::Chunk::Size * GameRules::Chunk::Size);

	const auto DirtId = GameManager->ShapeRegistry->GetShapeIdByName(GameRules::Constants::GShape_Layer_Dirt);
	const auto GrassId = GameManager->ShapeRegistry->GetShapeIdByName(GameRules::Constants::GShape_Layer_Grass);
	const auto StoneId = GameManager->ShapeRegistry->GetShapeIdByName(GameRules::Constants::GShape_Layer_Stone);

	const auto MaxHeight = FMath::FloorToInt(GameRules::Chunk::Height * 0.75f);
	FRandomStream RandomStream(123);
	
	if (Position.X == 0 && Position.Y == 0)
	{
		for (int X = 0; X < GameRules::Chunk::Size; ++X)
		{
			for (int Y = 0; Y < GameRules::Chunk::Size; ++Y)
			{
				if (X < 4 && Y < 4)
				{
					const auto Index = UChunkData::GetIndex(X, Y);
					auto& Column = OutColumns[Index];

					const auto WorldX = X + Position.X * GameRules::Chunk::Size;
					const auto WorldY = Y + Position.Y * GameRules::Chunk::Size;

					const float NoiseValue = Noise->GetNoise2D(WorldX * 0.5f, WorldY * 0.5f);

					const unsigned short Height = FMath::RoundToInt(
						(NoiseValue + 1) * (MaxHeight / 2));

					const auto GrassHeight = RandomStream.RandRange(1, 2);
					const auto DirtHeight = RandomStream.RandRange(1, 6);
					const auto StoneHeight = Height - GrassHeight - DirtHeight;
					const auto VoidHeight = GameRules::Chunk::Height - Height;
			
					Column.Pieces.Emplace(StoneId, StoneHeight);
					Column.Pieces.Emplace(DirtId, DirtHeight);
					Column.Pieces.Emplace(GrassId, GrassHeight);
					Column.Pieces.Emplace(GameRules::Constants::GShapeId_Void, VoidHeight);
				} else
				{
					const int Index = UChunkData::GetIndex(X, Y);
					OutColumns[Index] = FChunkColumn{
						{
							FPiece{0, static_cast<unsigned short>(GameRules::Chunk::Height)},
						}
					};
				}
			}
		}
	} else
	{
		for (int X = 0; X < GameRules::Chunk::Size; ++X)
		{
			for (int Y = 0; Y < GameRules::Chunk::Size; ++Y)
			{
				const int Index = UChunkData::GetIndex(X, Y);
				OutColumns[Index] = FChunkColumn{
					{
						FPiece{0, static_cast<unsigned short>(GameRules::Chunk::Height)},
					}
				};
			}
		}
	}

	// if (Position.X == 0 && Position.Y == 0)
	// {
	// 	OutColumns[UChunkData::GetIndex(0,0)] = FChunkColumn{
	// 			{
	// 				FPiece{1, 10},
	// 				FPiece{0, static_cast<unsigned short>(GameRules::Chunk::Height - 10)}
	// 			}
	// 	};
	//
	// 	OutColumns[UChunkData::GetIndex(1,0)] = FChunkColumn{
	// 				{
	// 					FPiece{1, 5},
	// 					FPiece{0, static_cast<unsigned short>(GameRules::Chunk::Height - 5)}
	// 				}
	// 	};
	//
	// 	OutColumns[UChunkData::GetIndex(0,1)] = FChunkColumn{
	// 		{
	// 			FPiece{1, 1024},
	// 		}
	// 	};
	// }

	// if (Position.X == -1 && Position.Y == -1)
	// {
	// 	OutColumns[UChunkData::GetIndex(0,0)] = FChunkColumn{
	// 				{
	// 					FPiece{1, 10},
	// 					FPiece{0, static_cast<unsigned short>(GameRules::Chunk::Height - 10)}
	// 				}
	// 	};
	//
	// 	OutColumns[UChunkData::GetIndex(1,0)] = FChunkColumn{
	// 					{
	// 						FPiece{1, 5},
	// 						FPiece{0, static_cast<unsigned short>(GameRules::Chunk::Height - 5)}
	// 					}
	// 	};
	//
	// 	OutColumns[UChunkData::GetIndex(0,1)] = FChunkColumn{
	// 			{
	// 				FPiece{1, 1024},
	// 			}
	// 	};
	// }
}
