#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Game/VoxelMaterial.h"
#include "Piece.generated.h"

USTRUCT(BlueprintType)
struct FPiece
{
	GENERATED_BODY()

	FPiece()
	{
	}

	FPiece(const EMaterial InMaterialId, const uint16 InSize)
		: MaterialId(InMaterialId), Size(InSize)
	{
	}

	UPROPERTY()
	EMaterial MaterialId = EMaterial::Void;

	UPROPERTY()
	uint16 Size = 1;

	friend FArchive& operator<<(FArchive& Ar, FPiece& Piece)
	{
		Ar << Piece.MaterialId;
		Ar << Piece.Size;
		return Ar;
	}
};