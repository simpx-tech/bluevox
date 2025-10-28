# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Bluevox is a voxel-based game built with Unreal Engine 5.6, featuring a chunk-based world generation system with dynamic instance-to-entity conversion, multiplayer networking, and efficient rendering through HISM components.

## Build Commands

### Building the Project
```bash
# Build the project (Windows, Development configuration)
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" Bluevox Win64 Development -Project="C:\Users\vinic\RiderProjects\bluevox\Bluevox.uproject"

# Clean and rebuild
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Clean.bat" Bluevox Win64 Development -Project="C:\Users\vinic\RiderProjects\bluevox\Bluevox.uproject"
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" Bluevox Win64 Development -Project="C:\Users\vinic\RiderProjects\bluevox\Bluevox.uproject"

# Generate project files (for IDE integration)
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\GenerateProjectFiles.bat" "C:\Users\vinic\RiderProjects\bluevox\Bluevox.uproject"
```

### Running the Game
```bash
# Run editor
"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\vinic\RiderProjects\bluevox\Bluevox.uproject"

# Run standalone game
"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\vinic\RiderProjects\bluevox\Bluevox.uproject" -game

# Run server
"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\vinic\RiderProjects\bluevox\Bluevox.uproject" -server -log

# Run client
"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\vinic\RiderProjects\bluevox\Bluevox.uproject" -game -windowed -ResX=1280 -ResY=720
```

## Architecture Overview

### Core Systems

#### Chunk System (`Source/Bluevox/Chunk/`)
- **AChunk**: Actor representing a single chunk in the world. Handles mesh rendering and instance management via HISM components.
- **UChunkData**: Thread-safe data container for chunk columns and instances. Uses FRWLock for concurrent access.
- **UChunkRegistry**: Central registry managing all chunks, regions, and their lifecycle. Coordinates between disk storage and in-memory chunks.
- **FRegionFile**: Handles persistent storage of chunks in region files (similar to Minecraft's region format).

Key relationships:
- ChunkRegistry owns ChunkData objects and spawns Chunk actors
- ChunkData contains both terrain (Columns) and instances (InstanceCollections)
- Chunks use ChunkData to render terrain mesh and instances

#### Instance System (`Source/Bluevox/Data/`, `Source/Bluevox/Entity/`)
Uses a data-driven primary asset system:

- **UInstanceTypeDataAsset**: Data asset defining instance properties (mesh, spawn rules, entity conversion settings)
- **FInstanceData**: Individual instance with transform and asset reference
- **FInstanceCollection**: Collection of instances of the same type with spatial indexing

Entity Conversion Flow:
1. Instances spawn as HISM instances (inexpensive rendering)
2. When players approach within conversion distance, the server converts to entities
3. Entities are full actors (AEntityFacade) with optional server-side logic (UEntityNode)
4. Clients hide instances when replicated entities spawn

#### Networking (`Source/Bluevox/Network/`)
- **ChunkDataNetworkPacket**: Sends chunk terrain AND instances from server to client
- Server generates all chunks and instances
- Clients receive pre-generated data via network packets
- Entity replication handled by Unreal's built-in system

#### Virtual Map System (`Source/Bluevox/Chunk/VirtualMap/`)
- **UVirtualMap**: Manages chunk loading/unloading based on player positions
- **UChunkTaskManager**: Handles async chunk operations (load, save, generate, network send)
- Implements view distance and automatic chunk streaming

#### Tick Management (`Source/Bluevox/Tick/`)
- **UTickManager**: Central tick system with budget-based execution
- **IGameTickable**: Interface for tickable objects
- **UEntityConversionTickable**: Manages instance-to-entity conversion per tick

### Threading Model
- Main game thread: Rendering, actor spawning, component registration
- Background threads: Chunk generation, mesh building, file I/O
- Critical sections: ChunkData access (FRWLock), region file operations

### Key Constants (`Source/Bluevox/Game/GameConstants.h`)
```cpp
namespace GameConstants {
    namespace Chunk {
        constexpr int32 Size = 32;      // Chunk width/depth
        constexpr int32 Height = 256;   // Chunk height
    }
    namespace Scaling {
        constexpr float XYWorldSize = 100.0f;  // Voxel size in XY
        constexpr float ZSize = 100.0f;        // Voxel size in Z
    }
    namespace Region {
        constexpr int32 Size = 32;  // Chunks per region
    }
}
```

### Common Patterns

#### Thread-Safe Chunk Data Access
```cpp
// Reading
{
    FReadScopeLock ReadLock(ChunkData->Lock);
    // Read operations
}

// Writing
{
    FWriteScopeLock WriteLock(ChunkData->Lock);
    // Write operations
}
```

#### Async Operations
```cpp
GameManager->TickManager->RunAsyncThen(
    [/* capture */] { /* async work */ return result; },
    [/* capture */](auto result) { /* game thread callback */ }
);
```

#### Position Conversions
- Global position: World coordinates
- Chunk position: Which chunk (integer coordinates)
- Local position: Position within chunk (0–31 for X, Y)
- Scaled position: Local position * scaling factors

### Critical Files to Understand

1. **GameManager** (`Source/Bluevox/Game/GameManager.h/cpp`): Central game coordinator, initializes all subsystems
2. **ChunkRegistry** (`Source/Bluevox/Chunk/ChunkRegistry.h/cpp`): Manages chunk lifecycle and data
3. **ChunkData** (`Source/Bluevox/Chunk/Data/ChunkData.h/cpp`): Thread-safe chunk data container
4. **NoiseWorldGenerator** (`Source/Bluevox/Chunk/Generator/NoiseWorldGenerator.h/cpp`): Generates terrain and instances
5. **EntityConversionTickable** (`Source/Bluevox/Entity/EntityConversionTickable.h/cpp`): Manages instance-to-entity conversion

# Design Patterns

- Since voxel games are really demanding games, always make the most efficient code possible performance wise
- Use the Unreal Engine's built-in systems whenever possible
- The code should be event-driven to allow easy extensibility for the developer and in future for mods
- The code should also prefer composition to inheritance, to also allow for modularity and extensibility
- Use Cl_, Sv_ and ClOnly_ prefixes for methods which should only be run on specific network authority
- When using RPC, include an R on the start, so if it's a server Rpc, it should have the prefix RSv_
- For event listeners methods, always prefix it with Handle_, where the rest is the exact delegate name, for example, Handle_OnAllRenderTasksFinishedForChunk
- Unreal is server-side first, so everything that can be processed on the server should be processed on the server, that's to reduce client overhead but also because things are replicated from the server to clients
