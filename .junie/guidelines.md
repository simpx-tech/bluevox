### Bluevox – Developer Guidelines (Project‑Specific)

This document captures build, configuration, and development conventions that are specific to this project (UE 5.6.1, C++).

---

### Build and Environment
- Engine/IDE
  - Unreal Engine: 5.6.1.
  - IDE: JetBrains Rider for Unreal recommended (project already Rider‑configured).
- Opening the project
  - Double‑click `Bluevox.uproject` (or right‑click → Switch Unreal Engine version to 5.6.1 if prompted).
  - If project files are missing after an engine upgrade: right‑click `Bluevox.uproject` → Generate Visual Studio project files (Rider picks them up automatically).
- Building
  - From Rider: select the `BluevoxEditor` target (Development Editor) and build; run from Rider or Unreal Editor.
  - From Unreal Editor: open the project and let UE compile the C++ modules; enable “Compile” toolbar as needed.
- Packaging
  - Windows target; ensure D3D12 is enabled (project includes `Binaries\Win64\D3D12` and `DML` but verify RHI settings if you change rendering).

---

### Runtime Configuration (Console Variables)
Many gameplay/world parameters are exposed via CVars (registered in `Source/Bluevox/Game/GameConstants.h`). You can set them at runtime via console or persist them in config (e.g., `DefaultEngine.ini` under `[/Script/Engine.Console]` → `ConsoleVariables=...`). Key CVars:

- Chunk
  - `game.rules.chunk.size` → int (default 48): blocks in X/Y per chunk.
  - `game.rules.chunk.world_height` → int (default 1024): blocks in Z for world height.
- Scaling (non‑uniform voxels)
  - `game.rules.scaling.xy_world_size` → float (default 100): world centimeters per block in X/Y.
  - `game.rules.scaling.z_world_size` → float (default 25): world centimeters per layer in Z.
  - `game.rules.scaling.player_height` → float (default equals Z size; used for derived calcs, don’t change directly unless you understand the ripple effects).
  - `game.rules.distances.player_height_in_layers` → int (default 8): player height measured in layers.
- Distances
  - `game.rules.distances.interaction_distance` → int (default 500): interaction reach in world units.
- Region
  - `game.rules.region.size` → int (default 24): chunks per axis in a region.
  - `game.rules.region.file.segment_size_bytes` → int (default 10240): region file segment size (bytes).
- Tick
  - `game.ticks.per_second` → int (default 24): target game ticks per second.
  - `game.ticks.budget_ns` → float (default 1,500,000): per‑frame tick budget in nanoseconds for spreading work.

Notes
- These are bound via `FAutoConsoleVariableRef`; changing them at runtime takes effect for systems that read the value each frame/tick. Some systems may cache values at init; restart if in doubt.
- When introducing new tunables, follow the existing CVar naming scheme and registration pattern in `GameConstants.h`.

---

### World/Scaling Model
- The project uses anisotropic voxel scaling:
  - X/Y block size: `GameConstants::Scaling::XYWorldSize` (cm), default 100.
  - Z layer size: `GameConstants::Scaling::ZWorldSize` (cm), default 25.
- Consequences
  - Asset bounds and placement often need conversion between world cm and voxel/layer counts.
  - Any geometry‑based estimation must divide world extents by the appropriate size constant (XY vs Z).

---

### Data Assets – Instances
Files: `Source/Bluevox/Data/InstanceTypeDataAsset.h/.cpp`.

- Primary Asset Type
  - Returns `FPrimaryAssetId("InstanceType", GetFName())`. Content browser assets should be placed under `Content/Data/Instances` (current convention) for organization.
- Key properties
  - Visual: `StaticMesh`, `CullDistance`.
  - Entity conversion: `bCanConvertToEntity`, `EntityClass`, `EntityConversionDistance`.
  - Generation: `SpawnChance`, `Radius`, `Height`, `ValidSurfaces`, `MinScale`, `MaxScale`.
- Editor automation
  - `PostEditChangeProperty` recomputes `Radius` and `Height` when `StaticMesh` changes by calling `EstimateInstanceSpacingFromMesh`.
  - Estimation logic (see header) computes:
    - `widthXY_cm = 2 * max(Ext.X, Ext.Y)` and `heightZ_cm = 2 * Ext.Z` from the mesh bounding box.
    - Converts to blocks/layers using `XYWorldSize` and `ZWorldSize` (protects against divide‑by‑zero).
  - This ensures generation spacing stays consistent with current scaling CVars. If you change scaling at runtime, assets won’t auto‑recalculate unless you re‑save or trigger property change in editor.
- Guidelines
  - Avoid setting `MinScale/MaxScale` far from 1.0 unless generation systems explicitly incorporate scale into collision/spacing; the current spacing estimator assumes mesh scale ≈ 1.
  - When you change `StaticMesh`, let the editor update values; don’t hand‑edit `Radius/Height` unless you know the impact.

---

### Chunk/Region/IO
- Chunk size: `GameConstants::Chunk::Size` (default 48 blocks in X/Y).
- World height: `GameConstants::Chunk::Height` (default 1024 layers).
- Region size: `GameConstants::Region::Size` (default 24 chunks per axis).
- File versions and sizes
  - `GameConstants::Chunk::File::FileVersion` (currently 1).
  - `GameConstants::Region::File::SegmentSizeBytes` (default 10240).
- When making breaking on‑disk changes, bump file version constants and provide migration/loader guards.

---

### Coordinates and Positions
- Position helpers live under `Source/Bluevox/Chunk/Position` (see `GlobalPosition.h`, `LocalPosition.h`).
- Be consistent with conversions between:
  - World cm ↔ block/layer indices (use scaling constants).
  - Local (within chunk) ↔ global (world) positions (use chunk size constants).
- Always keep Z math in “layers” terms; do not assume cubic voxels.

---

### Coding Conventions (Project‑Specific Over UE Style)
- Constants pattern
  - Use `namespace GameConstants::<Group>` with `extern inline` variables registered to CVars via `FAutoConsoleVariableRef` (see `GameConstants.h`).
  - Prefer adding new tunables here rather than scattering magic numbers.
- Asset/Editor hooks
  - For editor‑reactive assets, prefer `PostEditChangeProperty` and mark packages dirty after programmatic edits (`MarkPackageDirty()`).
- Defensive math
  - Guard divisions with `FMath::Max(1e-3f, Value)` as seen in spacing estimation to avoid NaNs when CVars are misconfigured.
- Namespaces and includes
  - Keep includes minimal and follow existing include order in each module. Use forward declarations where possible.

---

### Performance and Ticking
- Target tick rate: 24 TPS with a budget of ~1.5 ms per frame for server‑style logic spreading (see `GameConstants::Tick`).
- If you add long‑running tasks in tick, respect `TickBudget` and consider spreading work across frames.

---

### Debugging Tips
- Use the in‑editor console to tweak CVars at runtime (see list above) to validate generation, culling, interaction distances, etc.
- For data assets that depend on scaling, if values look off after changing CVars, re‑open the asset and toggle `StaticMesh` to force recomputation.
- Prefer explicit logs with tags; Rider’s Unreal log console helps filter.

---

### Common Pitfalls
- Changing scaling CVars without re‑evaluating data assets can cause incorrect spacing or collisions.
- Assuming cubic voxels will produce visually/physically wrong results; Z uses a much smaller world size than XY.
- When adding new on‑disk formats or changing chunk layout, coordinate the version constants and region segment size; don’t silently change binary layouts.

---

### Adding New Instance Types (Quick Checklist)
1) Create a new `InstanceTypeDataAsset` under `Content/Data/Instances`.
2) Assign `StaticMesh`; let the editor recompute `Radius` and `Height`.
3) Set `ValidSurfaces` materials and `SpawnChance`.
4) If the instance should convert to an entity at runtime, enable `bCanConvertToEntity` and set `EntityClass` and `EntityConversionDistance`.
5) Verify in world with current scaling; adjust CVars if necessary and re‑save.


---

### Design Patterns
- Performance first (voxel workload): prefer cache-friendly data layouts, avoid per-tick heap allocations, reuse buffers, batch work (e.g., HISM for instances, combined mesh builds), and respect `GameConstants::Tick::TickBudget` when spreading tasks.
- Prefer Unreal built-ins:
  - Replication and RPCs over custom net code; `UWorld` timers/`FTSTicker` over manual loops; `FAsyncTask`/`Async(EAsyncExecution::...)` for background work; Delegates/Dynamic Delegates for events; Subsystems for global services; HISM for large instance rendering.
- Event-driven architecture:
  - Expose key lifecycle hooks via delegates; avoid polling. Example listener naming: `Handle_OnAllRenderTasksFinishedForChunk` (exact delegate name after `Handle_`).
  - Publish events for chunk load/unload, mesh build completion, instance-to-entity conversions, network packet received, etc.
- Composition over inheritance:
  - Build modular features as `UActorComponent`/`UObject` modules attached to facades/actors rather than deep class trees. Prefer small, testable components (e.g., conversion policy, culling policy) that can be swapped.
- Networking authority prefixes (method naming):
  - `Cl_` → callable on any side, intended to run on client under normal flow.
  - `Sv_` → callable on any side, intended to run on server under normal flow.
  - `ClOnly_` → must only run on client (guards/asserts if authority != client).
  - RPCs must start with `R` and include authority: examples
    - Server RPC: `RSv_RequestChunkData(...)`
    - Client RPC: `RCl_ApplyChunkData(...)` (or `RCl_NotifyConversion(...)`)
- Delegate handlers naming:
  - Always prefix with `Handle_` followed by the exact delegate name (e.g., `Handle_OnAllRenderTasksFinishedForChunk`). Keep handler side-effects minimal and dispatch to systems as needed.
- Server-authoritative flow:
  - Default to performing heavy gameplay logic on the server; replicate results to clients. This reduces client overhead and preserves consistency. Clients should prefer prediction/visualization only where needed; authoritative state lives on the server.
