// Fill out your copyright notice in the Description page of Project Settings.


#include "TestWorldGenerator.h"

#include "FastNoiseWrapper.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Shape/ShapeRegistry.h"

void UTestWorldGenerator::GenerateThreeColumns(const FChunkPosition& Position,
	TArray<FChunkColumn>& OutColumns) const
{
	for (int X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const int Index = UChunkData::GetIndex(X, Y);
			OutColumns[Index] = FChunkColumn{
							{
								FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height)},
							}
			};
		}
	}

	if (Position.X == 0 && Position.Y == 0)
	{
		OutColumns[UChunkData::GetIndex(0,0)] = FChunkColumn{
					{
						FPiece{1, 10},
						FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height - 10)}
					}
		};
	
		OutColumns[UChunkData::GetIndex(1,0)] = FChunkColumn{
						{
							FPiece{1, 5},
							FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height - 5)}
						}
		};
	
		OutColumns[UChunkData::GetIndex(0,1)] = FChunkColumn{
				{
					FPiece{1, 1024},
				}
		};
	}
}

void UTestWorldGenerator::GenerateOneColumnTick(const FChunkPosition& Position,
	TArray<FChunkColumn>& OutColumns) const
{
	for (int X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const int Index = UChunkData::GetIndex(X, Y);
			OutColumns[Index] = FChunkColumn{
								{
									FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height)},
								}
			};
		}
	}

	if (Position.X == 0 && Position.Y == 0)
	{
		OutColumns[UChunkData::GetIndex(0,0)] = FChunkColumn{
						{
							FPiece{1, 100},
							FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height - 100)}
						}
		};
	}
}

void UTestWorldGenerator::GenerateNoise4X4(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);

	const auto DirtId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Layer_Dirt);
	const auto GrassId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Layer_Grass);
	const auto StoneId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Layer_Stone);

	const auto MaxHeight = FMath::FloorToInt(GameConstants::Chunk::Height * 0.75f);
	const FRandomStream RandomStream(123);
	
	if (Position.X == 0 && Position.Y == 0)
	{
		for (int X = 0; X < GameConstants::Chunk::Size; ++X)
		{
			for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
			{
				if (X < 4 && Y < 4)
				{
					const auto Index = UChunkData::GetIndex(X, Y);
					auto& Column = OutColumns[Index];

					const auto WorldX = X + Position.X * GameConstants::Chunk::Size;
					const auto WorldY = Y + Position.Y * GameConstants::Chunk::Size;

					const float NoiseValue = Noise->GetNoise2D(WorldX * 0.5f, WorldY * 0.5f);

					const unsigned short Height = FMath::RoundToInt(
						(NoiseValue + 1) * (MaxHeight / 2));

					const auto GrassHeight = RandomStream.RandRange(1, 2);
					const auto DirtHeight = RandomStream.RandRange(1, 6);
					const auto StoneHeight = Height - GrassHeight - DirtHeight;
					const auto VoidHeight = GameConstants::Chunk::Height - Height;
			
					Column.Pieces.Emplace(StoneId, StoneHeight);
					Column.Pieces.Emplace(DirtId, DirtHeight);
					Column.Pieces.Emplace(GrassId, GrassHeight);
					Column.Pieces.Emplace(GameConstants::Shapes::GShapeId_Void, VoidHeight);
				} else
				{
					const int Index = UChunkData::GetIndex(X, Y);
					OutColumns[Index] = FChunkColumn{
						{
							FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height)},
						}
					};
				}
			}
		}
	} else
	{
		for (int X = 0; X < GameConstants::Chunk::Size; ++X)
		{
			for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
			{
				const int Index = UChunkData::GetIndex(X, Y);
				OutColumns[Index] = FChunkColumn{
					{
						FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height)},
					}
				};
			}
		}
	}
}

void UTestWorldGenerator::GenerateTickAlwaysShape(const FChunkPosition& Position,
	TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);
	
	for (int X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const int Index = UChunkData::GetIndex(X, Y);
			OutColumns[Index] = FChunkColumn{
								{
									FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height)},
								}
			};
		}
	}

	if (Position.X == 0 && Position.Y == 0)
	{
		const auto TickAlwaysShape = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Test_AlwaysTick);
		
		OutColumns[UChunkData::GetIndex(0,0)] = FChunkColumn{
						{
							FPiece{TickAlwaysShape, 1},
							FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height - 1)}
						}
		};
	}
}

void UTestWorldGenerator::GenerateTickOnLoadShape(const FChunkPosition& Position,
	TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);
	
	for (int X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const int Index = UChunkData::GetIndex(X, Y);
			OutColumns[Index] = FChunkColumn{
									{
										FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height)},
									}
			};
		}
	}

	if (Position.X == 0 && Position.Y == 0)
	{
		const auto TickOnLoadShape = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Test_TickOnLoad);
		
		OutColumns[UChunkData::GetIndex(0,0)] = FChunkColumn{
							{
								FPiece{TickOnLoadShape, 1},
								FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height - 1)}
							}
		};
	}
}

void UTestWorldGenerator::GenerateTickOnNeighborUpdateShape(const FChunkPosition& Position,
	TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);
	
	for (int X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const int Index = UChunkData::GetIndex(X, Y);
			OutColumns[Index] = FChunkColumn{
									{
										FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height)},
									}
			};
		}
	}

	if (Position.X == 0 && Position.Y == 0)
	{
		const auto Dirt = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Layer_Dirt);
		const auto TickOnNeighborUpdate = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Test_TickOnNeighborUpdate);
		
		OutColumns[UChunkData::GetIndex(0,0)] = FChunkColumn{
							{
								FPiece{TickOnNeighborUpdate, 1},
								FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height - 1)}
							}
		};
		OutColumns[UChunkData::GetIndex(0,1)] = FChunkColumn{
									{
										FPiece{Dirt, 1},
										FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height - 1)}
									}};
		OutColumns[UChunkData::GetIndex(1,1)] = FChunkColumn{
								{
									FPiece{Dirt, 1},
									FPiece{0, static_cast<unsigned short>(GameConstants::Chunk::Height - 1)}
								}
		};
	}
}

UTestWorldGenerator::UTestWorldGenerator()
{
	Noise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("Noise"));
	Noise->SetupFastNoise(EFastNoise_NoiseType::Perlin, 123, 0.03f);
}

void UTestWorldGenerator::GenerateChunk(const FChunkPosition& Position,
                                        TArray<FChunkColumn>& OutColumns) const
{
	// GenerateTickAlwaysShape(Position, OutColumns);
	// GenerateTickOnLoadShape(Position, OutColumns);
	GenerateTickOnNeighborUpdateShape(Position, OutColumns);
}
