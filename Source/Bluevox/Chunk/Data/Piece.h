#pragma once

#include "CoreMinimal.h"
#include "Piece.generated.h"

UENUM()
enum EIdType
{
	Shape = 0 UMETA(DisplayName = "Shape"),
};

USTRUCT(BlueprintType)
struct FPiece
{
	GENERATED_BODY()

	FPiece()
	{
	}

	FPiece(const uint16 InId, const uint16 InSize)
		: Id(InId)
	{
		SetSizeType(InSize, Shape);
	}

	static constexpr uint16 Low14_Mask   = (1u << 14) - 1;
	static constexpr uint16 High2_Mask   = ~Low14_Mask; 
	static constexpr unsigned High2_Shift = 14;

	UPROPERTY()
	uint16 Id = 0;

	UPROPERTY()
	uint16 PackedSizeAndType = 0;

	FORCEINLINE uint16_t GetSize() const
	{
		return PackedSizeAndType & Low14_Mask;
	}

	FORCEINLINE EIdType GetType() const
	{
		return static_cast<EIdType>(PackedSizeAndType >> High2_Shift);
	}

	static uint16 MakeSizeType(const uint16 NewSize, const EIdType NewType);

	FORCEINLINE void SetSizeType(const uint16 NewSize, const EIdType NewType)
	{
		PackedSizeAndType = MakeSizeType(NewSize, NewType);
	}

	FORCEINLINE void SetSize(const uint16 NewSize) noexcept
	{
		PackedSizeAndType = PackedSizeAndType & High2_Mask | NewSize & Low14_Mask;
	}

	FORCEINLINE void SetType(const EIdType NewType) noexcept
	{
		PackedSizeAndType = PackedSizeAndType & Low14_Mask |
							(static_cast<uint16>(NewType) & 0x3) << High2_Shift;
	}

	friend FArchive& operator<<(FArchive& Ar, FPiece& Piece)
	{
		Ar << Piece.Id;
		Ar << Piece.PackedSizeAndType;
		return Ar;
	}
};

inline uint16 FPiece::MakeSizeType(const uint16 NewSize, const EIdType NewType)
{
	return NewSize & Low14_Mask |
		(static_cast<uint16>(NewType) & 0x3) << High2_Shift;
}
