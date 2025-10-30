// Fill out your copyright notice in the Description page of Project Settings.


#include "Chunk.h"

#include "ChunkRegistry.h"
#include "LogChunk.h"
#include "ChunkStats.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Tick/TickManager.h"
#include "Bluevox/Utils/Face.h"
#include "Data/ChunkData.h"
#include "Position/LocalPosition.h"
#include "Runtime/GeometryFramework/Public/Components/DynamicMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "VirtualMap/ChunkTaskManager.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"
#include "Bluevox/Data/InstanceTypeDataAsset.h"
#include "Engine/AssetManager.h"

void AChunk::BeginDestroy()
{
	if (GameManager)
	{
		GameManager->TickManager->UnregisterUObjectTickable(this);	
	}
	
	Super::BeginDestroy();
}

// Sets default values
AChunk::AChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->bEnableComplexCollision = true;
	MeshComponent->bCastShadowAsTwoSided = true;
	MeshComponent->CollisionType = CTF_UseComplexAsSimple;
	MeshComponent->bUseAsyncCooking = true;
	MeshComponent->SetMobility(EComponentMobility::Type::Static);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	RootComponent = MeshComponent;
}

// TODO also pass render state, so know if should tick or not (in case of live x lod chunks)
AChunk* AChunk::Init(const FChunkPosition InPosition, AGameManager* InGameManager,
	UChunkData* InData)
{
	Position = InPosition;
	GameManager = InGameManager;
	ChunkData = InData;
	
	return this;
}

void AChunk::SetRenderState(const EChunkState State) const
{
	UE_LOG(LogChunk, Verbose, TEXT("SetRenderState for chunk %s to %s"), *Position.ToString(), *UEnum::GetValueAsString(State));
	const auto Visible = EnumHasAllFlags(State, EChunkState::Visible);
	MeshComponent->SetVisibility(Visible);

	// Update instance visibility
	for (const auto& Pair : ChunkInstanceComponents)
	{
		if (Pair.Value)
		{
			Pair.Value->SetVisibility(Visible);
		}
	}

	// const auto Collision = EnumHasAllFlags(State, EChunkState::Collision);
	// MeshComponent->SetCollisionEnabled(Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

bool AChunk::BeginRender(UE::Geometry::FDynamicMesh3& OutMesh, bool bForceRender)
{
 SCOPE_CYCLE_COUNTER(STAT_Chunk_BeginRender);
	UE_LOG(LogChunk, Verbose, TEXT("Th_BeginRender for chunk %s"), *Position.ToString());

	const uint64 Start = FPlatformTime::Cycles64();

	if (!bForceRender && RenderedAtDirtyChanges.GetValue() == ChunkData->Changes)
	{
		return false;
	}

 // Reset and prepare mesh attributes
	OutMesh.Clear();
	OutMesh.EnableAttributes();
	OutMesh.Attributes()->SetNumUVLayers(2);
	OutMesh.Attributes()->SetNumNormalLayers(1);

	using namespace UE::Geometry;
	FDynamicMeshUVOverlay* UV0 = OutMesh.Attributes()->PrimaryUV();
	FDynamicMeshUVOverlay* UV1 = OutMesh.Attributes()->GetUVLayer(1);
	FDynamicMeshNormalOverlay* NO = OutMesh.Attributes()->PrimaryNormals();

	const auto ChunkRegistry = GameManager->ChunkRegistry;

 // Lambda to add a quad (two triangles) with UVs and per-face normals (UV0 = texcoords, UV1.x = texture array index)
	auto AddQuad = [&](FVector3d V0, FVector3d V1, FVector3d V2, FVector3d V3,
		FVector2f T0_UV0, FVector2f T1_UV0, FVector2f T2_UV0, FVector2f T3_UV0,
		float TextureIndex, const FVector3f& DesiredNormal)
	{
		// Ensure winding matches desired outward normal using the first triangle
		const FVector3d E1 = V1 - V0;
		const FVector3d E2 = V2 - V0;
		FVector3d TriN = E1.Cross(E2);
		if (TriN.Dot(FVector3d(DesiredNormal)) > 0)
		{
			// Flip winding by swapping V1 <-> V3 and their UVs (global winding correction)
			Swap(V1, V3);
			Swap(T1_UV0, T3_UV0);
			// Recompute normal (optional)
		}

		const int32 I0 = OutMesh.AppendVertex(V0);
		const int32 I1 = OutMesh.AppendVertex(V1);
		const int32 I2 = OutMesh.AppendVertex(V2);
		const int32 I3 = OutMesh.AppendVertex(V3);

		const int32 Tri0 = OutMesh.AppendTriangle(I0, I1, I2);
		const int32 Tri1 = OutMesh.AppendTriangle(I0, I2, I3);

		if (UV0)
		{
			const int32 E0 = UV0->AppendElement(T0_UV0);
			const int32 E1u = UV0->AppendElement(T1_UV0);
			const int32 E2u = UV0->AppendElement(T2_UV0);
			const int32 E3u = UV0->AppendElement(T3_UV0);
			UV0->SetTriangle(Tri0, FIndex3i(E0, E1u, E2u));
			UV0->SetTriangle(Tri1, FIndex3i(E0, E2u, E3u));
		}

		if (UV1)
		{
			const FVector2f LayerUV(TextureIndex, 0.0f);
			const int32 E0_1 = UV1->AppendElement(LayerUV);
			const int32 E1_1 = UV1->AppendElement(LayerUV);
			const int32 E2_1 = UV1->AppendElement(LayerUV);
			const int32 E3_1 = UV1->AppendElement(LayerUV);
			UV1->SetTriangle(Tri0, FIndex3i(E0_1, E1_1, E2_1));
			UV1->SetTriangle(Tri1, FIndex3i(E0_1, E2_1, E3_1));
		}

		if (NO)
		{
			const FVector3f N = DesiredNormal.GetSafeNormal();
			const int32 N0 = NO->AppendElement(N);
			const int32 N1 = NO->AppendElement(N);
			const int32 N2 = NO->AppendElement(N);
			const int32 N3 = NO->AppendElement(N);
			NO->SetTriangle(Tri0, FIndex3i(N0, N1, N2));
			NO->SetTriangle(Tri1, FIndex3i(N0, N2, N3));
		}
	};

	// Geometry scaling
	const float SXY = GameConstants::Scaling::XYWorldSize;
	const float SZ = GameConstants::Scaling::ZWorldSize;

	// Precompute base world column position
	FColumnPosition GlobalPos;
	const int32 BaseChunkPosX = Position.X * GameConstants::Chunk::Size;
	const int32 BaseChunkPosY = Position.Y * GameConstants::Chunk::Size;

	TStaticArray<FRenderNeighbor, 4> Neighbors;

	auto EmitSideSpan = [&](EFace Face, int32 LocalX, int32 LocalY, int32 Z0, int32 Z1, float TexIndex)
	{
		const double x0 = LocalX * SXY;
		const double x1 = (LocalX + 1) * SXY;
		const double y0w = LocalY * SXY;
		const double y1w = (LocalY + 1) * SXY;
		const double z0w = Z0 * SZ;
		const double z1w = Z1 * SZ;

		const int32 GlobalXInt = BaseChunkPosX + LocalX;
		const int32 GlobalYInt = BaseChunkPosY + LocalY;

		constexpr float TileXYPerBlock = 1.0f; // 1 block (100 units) = 1 tile horizontally
		constexpr float TilePerLayerZ = 0.25f; // 1 layer (25 units) = 0.25 tile vertically

		switch (Face)
		{
		case EFace::North: // +X, plane at x1
		{
			const float u0 = (GlobalYInt) * TileXYPerBlock;
			const float u1 = (GlobalYInt + 1) * TileXYPerBlock;
			const float v0 = Z0 * TilePerLayerZ;
			const float v1 = Z1 * TilePerLayerZ;
   AddQuad(
				FVector3d(x1, y0w, z0w), FVector3d(x1, y1w, z0w), FVector3d(x1, y1w, z1w), FVector3d(x1, y0w, z1w),
				FVector2f(u0, v0), FVector2f(u1, v0), FVector2f(u1, v1), FVector2f(u0, v1),
				TexIndex, FVector3f(1, 0, 0));
			break;
		}
		case EFace::South: // -X, plane at x0
		{
			const float u0 = (GlobalYInt) * TileXYPerBlock;
			const float u1 = (GlobalYInt + 1) * TileXYPerBlock;
			const float v0 = Z0 * TilePerLayerZ;
			const float v1 = Z1 * TilePerLayerZ;
   AddQuad(
				FVector3d(x0, y1w, z0w), FVector3d(x0, y0w, z0w), FVector3d(x0, y0w, z1w), FVector3d(x0, y1w, z1w),
				FVector2f(u1, v0), FVector2f(u0, v0), FVector2f(u0, v1), FVector2f(u1, v1),
				TexIndex, FVector3f(-1, 0, 0));
			break;
		}
		case EFace::East: // +Y, plane at y1
		{
			const float u0 = (GlobalXInt) * TileXYPerBlock;
			const float u1 = (GlobalXInt + 1) * TileXYPerBlock;
			const float v0 = Z0 * TilePerLayerZ;
			const float v1 = Z1 * TilePerLayerZ;
   AddQuad(
				FVector3d(x0, y1w, z0w), FVector3d(x0, y1w, z1w), FVector3d(x1, y1w, z1w), FVector3d(x1, y1w, z0w),
				FVector2f(u0, v0), FVector2f(u0, v1), FVector2f(u1, v1), FVector2f(u1, v0),
				TexIndex, FVector3f(0, 1, 0));
			break;
		}
		case EFace::West: // -Y, plane at y0
		{
			const float u0 = (GlobalXInt) * TileXYPerBlock;
			const float u1 = (GlobalXInt + 1) * TileXYPerBlock;
			const float v0 = Z0 * TilePerLayerZ;
			const float v1 = Z1 * TilePerLayerZ;
   AddQuad(
				FVector3d(x0, y0w, z0w), FVector3d(x1, y0w, z0w), FVector3d(x1, y0w, z1w), FVector3d(x0, y0w, z1w),
				FVector2f(u0, v0), FVector2f(u1, v0), FVector2f(u1, v1), FVector2f(u0, v1),
				TexIndex, FVector3f(0, -1, 0));
			break;
		}
		default:
			break;
		}
	};

	auto EmitCap = [&](bool bTop, int32 LocalX, int32 LocalY, int32 Z, float TexIndex)
	{
		const double x0w = LocalX * SXY;
		const double x1w = (LocalX + 1) * SXY;
		const double y0w = LocalY * SXY;
		const double y1w = (LocalY + 1) * SXY;
		const double zw = Z * SZ;

		const int32 GlobalXInt = BaseChunkPosX + LocalX;
		const int32 GlobalYInt = BaseChunkPosY + LocalY;
		constexpr float TileXYPerBlock = 1.0f;

		const float u0 = (GlobalXInt) * TileXYPerBlock;
		const float u1 = (GlobalXInt + 1) * TileXYPerBlock;
		const float v0 = (GlobalYInt) * TileXYPerBlock;
		const float v1 = (GlobalYInt + 1) * TileXYPerBlock;

		if (bTop)
		{
			// Top face, normal +Z (CCW when viewed from above)
   AddQuad(
				FVector3d(x0w, y0w, zw), FVector3d(x0w, y1w, zw), FVector3d(x1w, y1w, zw), FVector3d(x1w, y0w, zw),
				FVector2f(u0, v0), FVector2f(u0, v1), FVector2f(u1, v1), FVector2f(u1, v0),
				TexIndex, FVector3f(0, 0, 1));
		}
		else
		{
			// Bottom face, normal -Z (CCW when viewed from below)
   AddQuad(
				FVector3d(x1w, y0w, zw), FVector3d(x1w, y1w, zw), FVector3d(x0w, y1w, zw), FVector3d(x0w, y0w, zw),
				FVector2f(u1, v0), FVector2f(u1, v1), FVector2f(u0, v1), FVector2f(u0, v0),
				TexIndex, FVector3f(0, 0, -1));
		}
	};

	for (int32 LocalX = 0; LocalX < GameConstants::Chunk::Size; ++LocalX)
	{
		for (int32 LocalY = 0; LocalY < GameConstants::Chunk::Size; ++LocalY)
		{
			SCOPE_CYCLE_COUNTER(STAT_Chunk_BeginRender_ProcessColumn);

			const int32 ColumnIndex = UChunkData::GetIndex(LocalX, LocalY);
			const TArray<FPiece>& Pieces = ChunkData->Columns[ColumnIndex].Pieces;
			const int32 PiecesCount = Pieces.Num();

			GlobalPos.X = BaseChunkPosX + LocalX;
			GlobalPos.Y = BaseChunkPosY + LocalY;

			// Setup neighbor iterators (aligned by Z)
			for (const EFace Face : FaceUtils::AllHorizontalFaces)
			{
				const FChunkColumn& Column = ChunkRegistry->Th_GetColumn(GlobalPos + FaceUtils::GetHorizontalOffsetByFace(Face));
				FRenderNeighbor& Neighbor = Neighbors[static_cast<uint8>(Face)];
				Neighbor.Column = &Column;
				Neighbor.Index = 0;
				Neighbor.Start = 0;
				Neighbor.Size = Column.Pieces.Num() > 0 ? Column.Pieces[0].Size : 0;
				Neighbor.Piece = Column.Pieces.Num() > 0 ? &Column.Pieces[0] : nullptr;
				Neighbor.MaterialId = Neighbor.Piece ? Neighbor.Piece->MaterialId : EMaterial::Void;
			}

			int32 CurZ = 0;
			for (int32 PieceIdx = 0; PieceIdx < PiecesCount; ++PieceIdx)
			{
				const FPiece& Piece = Pieces[PieceIdx];
				const int32 PieceSize = Piece.Size;
				const float TexIndex = static_cast<float>(static_cast<uint8>(Piece.MaterialId));

				if (Piece.MaterialId == EMaterial::Void)
				{
					// Advance neighbors through this empty span
					for (const EFace Face : FaceUtils::AllHorizontalFaces)
					{
						FRenderNeighbor& N = Neighbors[static_cast<uint8>(Face)];
						while (N.Index + 1 < N.Column->Pieces.Num() && (CurZ + PieceSize) >= (N.Start + N.Size))
						{
							N.Start += N.Size;
							N.Index += 1;
							N.Size = N.Column->Pieces[N.Index].Size;
							N.Piece = &N.Column->Pieces[N.Index];
							N.MaterialId = N.Piece->MaterialId;
						}
					}
					CurZ += PieceSize;
					continue;
				}

				// Emit side faces by greedy merging along Z where neighbor is void
				for (const EFace Face : FaceUtils::AllHorizontalFaces)
				{
					FRenderNeighbor& N = Neighbors[static_cast<uint8>(Face)];
					int32 Processed = 0;
					int32 RunStartZ = -1;
					while (Processed < PieceSize)
					{
						const int32 CurrentZ = CurZ + Processed;
						const int32 NeighborEndZ = N.Start + N.Size;
						const int32 SpanEndZ = FMath::Min(CurZ + PieceSize, NeighborEndZ);
						const bool bNeighborIsVoid = (N.MaterialId == EMaterial::Void);

						if (bNeighborIsVoid)
						{
							if (RunStartZ < 0) RunStartZ = CurrentZ;
						}
						else if (RunStartZ >= 0)
						{
							EmitSideSpan(Face, LocalX, LocalY, RunStartZ, CurrentZ, TexIndex);
							RunStartZ = -1;
						}

						Processed += (SpanEndZ - CurrentZ);

						if (SpanEndZ >= NeighborEndZ && (N.Index + 1) < N.Column->Pieces.Num())
						{
							N.Start += N.Size;
							N.Index += 1;
							N.Size = N.Column->Pieces[N.Index].Size;
							N.Piece = &N.Column->Pieces[N.Index];
							N.MaterialId = N.Piece->MaterialId;
						}
					}
					if (RunStartZ >= 0)
					{
						EmitSideSpan(Face, LocalX, LocalY, RunStartZ, CurZ + PieceSize, TexIndex);
					}
				}

				// Top cap
				bool bRenderTop = true;
				if (PieceIdx < PiecesCount - 1)
				{
					bRenderTop = (Pieces[PieceIdx + 1].MaterialId == EMaterial::Void);
				}
				if (bRenderTop)
				{
					EmitCap(true, LocalX, LocalY, CurZ + PieceSize, TexIndex);
				}

				// Bottom cap: only if there is void below AND not at world Z==0
				bool bRenderBottom = false;
				if (PieceIdx == 0)
				{
					bRenderBottom = (CurZ > 0);
				}
				else
				{
					bRenderBottom = (Pieces[PieceIdx - 1].MaterialId == EMaterial::Void);
				}
				if (bRenderBottom)
				{
					EmitCap(false, LocalX, LocalY, CurZ, TexIndex);
				}

				CurZ += PieceSize;
			}
		}
	}

	RenderedAtDirtyChanges.Set(ChunkData->Changes);

	const uint64 End = FPlatformTime::Cycles64();
	UE_LOG(LogChunk, Verbose, TEXT("Chunk %s rendered in %f ms"), *Position.ToString(), FPlatformTime::ToMilliseconds64(End - Start));

	return true;
}

void AChunk::CommitRender(FRenderResult&& RenderResult) const
{
	UE_LOG(LogChunk, Verbose, TEXT("Committing render for chunk %s"), *Position.ToString());
	MeshComponent->SetMaterial(0, GameManager->ChunkMaterial);
	MeshComponent->SetMesh(MoveTemp(RenderResult.Mesh));
}

