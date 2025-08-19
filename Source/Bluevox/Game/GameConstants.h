#pragma once

namespace GameConstants::Shapes
{
	inline constexpr uint16 GShapeId_Void = 0;
	
	inline const FName GShape_Void(TEXT("core:shape:void"));
	inline const FName GShape_Layer(TEXT("core:shape:layer"));
	inline const FName GShape_Layer_Dirt(TEXT("core:shape:layer_dirt"));
	inline const FName GShape_Layer_Grass(TEXT("core:shape:layer_grass"));
	inline const FName GShape_Layer_Stone(TEXT("core:shape:layer_stone"));

	inline const FName GShape_Test_AlwaysTick(TEXT("core:shape:test_always_tick"));
	inline const FName GShape_Test_TickOnLoad(TEXT("core:shape:test_tick_on_load"));
	inline const FName GShape_Test_TickOnNeighborUpdate(TEXT("core:shape:test_tick_on_neighbor_update"));
}

namespace GameConstants::Chunk
{
	extern inline int32 Size = 48;
	static FAutoConsoleVariableRef CVarChunkSize(
		TEXT("game.rules.chunk.size"), Size,
		TEXT("The number of blocks in X and Y axis per chunk"), ECVF_Default);

	extern inline int32 Height = 1024;
	static FAutoConsoleVariableRef CVarWorldHeight(
		TEXT("game.rules.chunk.world_height"), Height,
		TEXT("The number of blocks in Z"), ECVF_Default);
}

namespace GameConstants::Chunk::File
{
	extern inline constexpr int32 FileVersion = 1;
}

namespace GameConstants::Scaling
{
	extern inline float XYWorldSize = 100;
	static FAutoConsoleVariableRef CVarBlockWorldSize(
		TEXT("game.rules.scaling.xy_world_size"), XYWorldSize,
		TEXT("The size of a block in the world"), ECVF_Default);

	extern inline float ZSize = 25;
	static FAutoConsoleVariableRef CVarLayerWorldSize(
		TEXT("game.rules.scaling.z_world_size"), ZSize,
		TEXT("The size of a layer in the world"), ECVF_Default);

	extern inline float PlayerHeight = 100;
	static FAutoConsoleVariableRef CVarPlayerHeight(
		TEXT("game.rules.scaling.player_height"), ZSize,
		TEXT(
			"The height of the player (Don't modify player height it's just used for some calculations)"),
		ECVF_Default);
}

namespace GameConstants::Distances
{
	extern inline int32 InteractionDistance = 500;
	static FAutoConsoleVariableRef CVarInteractionDistance(
		TEXT("game.rules.distances.interaction_distance"), InteractionDistance,
		TEXT("The distance for interaction"), ECVF_Default);

	extern inline int32 PlayerHeightInLayers = 8;
	static FAutoConsoleVariableRef CVarPlayerSizeInLayers(
		TEXT("game.rules.distances.player_height_in_layers"), PlayerHeightInLayers,
		TEXT("The height of the player in layers"), ECVF_Default);
}

namespace GameConstants::Region
{
	extern inline int32 Size = 24;
	static FAutoConsoleVariableRef CVarRegionSize(
		TEXT("game.rules.region.size"), Size,
		TEXT("The number of chunks in X and Y axis"), ECVF_Default);
}

namespace GameConstants::Region::File
{
	extern inline int32 SegmentSizeBytes = 10240; // 10 kb
	static FAutoConsoleVariableRef CVarSegmentSizeBytes(
		TEXT("game.rules.region.file.segment_size_bytes"), SegmentSizeBytes,
		TEXT("The size of a region file segment in bytes"), ECVF_Default);
}

namespace GameConstants::Tick
{
	extern inline int32 TicksPerSecond = 24;
	static FAutoConsoleVariableRef CVarTicksPerSecond(
			TEXT("game.ticks.per_second"), TicksPerSecond,
			TEXT("Ideal number of ticks per second"), ECVF_Default);

	extern inline float TickBudget = 1'500'000; // 1.5 ms
	static FAutoConsoleVariableRef CVarTickBudget(
		TEXT("game.ticks.budget_ns"), TickBudget,
		TEXT("Maximum time (in nanoseconds) before spreading tasks across frames"), ECVF_Default);
}
