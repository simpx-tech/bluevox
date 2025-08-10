#pragma once

namespace GameRules::Constants
{
	inline constexpr uint16 GShapeId_Void = 0;
	
	inline const FName GShape_Void(TEXT("core:shape:void"));
	inline const FName GShape_Layer(TEXT("core:shape:layer"));
	inline const FName GShape_Layer_Dirt(TEXT("core:shape:layer_dirt"));
	inline const FName GShape_Layer_Grass(TEXT("core:shape:layer_grass"));
	inline const FName GShape_Layer_Stone(TEXT("core:shape:layer_stone"));
}

#if UE_BUILD_SHIPPING

namespace GameRules::Chunk
{
	extern inline constexpr int32 Size = 48;
	extern inline constexpr int32 Height = 1024;
}

namespace GameRules::Scaling
{
	extern inline constexpr float XYWorldSize = 100;
	extern inline constexpr float ZSize = 25;
	extern inline constexpr float PlayerHeight = 100;
}

namespace GameRules::Distances
{
	extern inline constexpr int32 InteractionDistance = 500;
}

namespace GameRules::Region
{
	extern inline constexpr int32 Size = 24;
}

namespace GameRules::Region::File
{
	extern inline constexpr int32 SegmentSizeBytes = 10240; // 10 kb
}

namespace GameRules::Tick
{
	extern inline constexpr int32 TicksPerSecond = 24;
	extern inline constexpr float TickBudget = 1'500'000; // 1.5 ms
}

#else
namespace GameRules::Chunk
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

namespace GameRules::Scaling
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

namespace GameRules::Distances
{
	extern inline int32 InteractionDistance = 500;
	static FAutoConsoleVariableRef CVarInteractionDistance(
		TEXT("game.rules.distances.interaction_distance"), InteractionDistance,
		TEXT("The distance for interaction"), ECVF_Default);
}

namespace GameRules::Region
{
	extern inline int32 Size = 24;
	static FAutoConsoleVariableRef CVarRegionSize(
		TEXT("game.rules.region.size"), Size,
		TEXT("The number of chunks in X and Y axis"), ECVF_Default);
}

namespace GameRules::Region::File
{
	extern inline int32 SegmentSizeBytes = 10240; // 10 kb
	static FAutoConsoleVariableRef CVarSegmentSizeBytes(
		TEXT("game.rules.region.file.segment_size_bytes"), SegmentSizeBytes,
		TEXT("The size of a region file segment in bytes"), ECVF_Default);
}

namespace GameRules::Tick
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
#endif

// Non constants variables

namespace GameRules::Distances
{
	extern inline int32 LiveDistance = 6;
	static FAutoConsoleVariableRef CVarSegmentSizeBytes(
		TEXT("game.distances.live_distance"), LiveDistance,
		TEXT("How far to have live chunks around the player"), ECVF_Default);

	extern inline int32 Sv_MaxLoadDistance = 24;
	static FAutoConsoleVariableRef CVarServerMaxLoadDistance(
		TEXT("game.distances.sv_max_load_distance"), LiveDistance,
		TEXT("How far the server allows the player to load chunks"), ECVF_Default);
}
