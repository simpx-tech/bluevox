#include "EntityCreatedInChunkPacket.h"

#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/Chunk.h"

void UEntityCreatedInChunkPacket::OnReceive(AGameManager* GameManager)
{
	if (!GameManager || !GameManager->ChunkRegistry)
	{
		return;
	}

	AChunk* Chunk = GameManager->ChunkRegistry->GetChunkActor(ChunkPosition);
	if (Chunk)
	{
		Chunk->Cl_RemoveInstance(InstanceTypeId, InstanceIndex);
	}
}

void UEntityCreatedInChunkPacket::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Ar << ChunkPosition;
		FString AssetIdString;
		if (Ar.IsSaving())
		{
			AssetIdString = InstanceTypeId.ToString();
		}
		Ar << AssetIdString;
		if (Ar.IsLoading())
		{
			InstanceTypeId = FPrimaryAssetId(AssetIdString);
		}
		Ar << InstanceIndex;
	}
}
