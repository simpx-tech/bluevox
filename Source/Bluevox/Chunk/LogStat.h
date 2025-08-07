#pragma once
DECLARE_STATS_GROUP(TEXT("Chunks"), STATGROUP_Chunks, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("AChunk::BeginRender"), STAT_Chunk_BeginRender, STATGROUP_Chunks);

DECLARE_CYCLE_STAT(TEXT("ProcessColumn"), STAT_Chunk_BeginRender_ProcessColumn, STATGROUP_Chunks);

DECLARE_CYCLE_STAT(TEXT("ProcessPiece"), STAT_Chunk_BeginRender_ProcessPiece, STATGROUP_Chunks);

DECLARE_CYCLE_STAT(TEXT("AllHorizontalFaces"), STAT_Chunk_BeginRender_ProcessPiece_AllHorizontalFaces, STATGROUP_Chunks);

DECLARE_CYCLE_STAT(TEXT("UShape::Render"), STAT_Shape_Render, STATGROUP_Chunks);