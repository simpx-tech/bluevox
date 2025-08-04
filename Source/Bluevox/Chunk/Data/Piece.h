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

	static constexpr uint16_t Low14_Mask  = (1u << 14) - 1;
	static constexpr unsigned High2_Shift = 14;

	UPROPERTY()
	uint16 Id = 0;

	UPROPERTY()
	uint16 PackedSizeAndType = 0;

	UFUNCTION()
	FORCEINLINE uint16_t GetSize() const
	{
		return PackedSizeAndType & Low14_Mask;
	}

	UFUNCTION()
	FORCEINLINE EIdType GetType() const
	{
		return static_cast<EIdType>(PackedSizeAndType >> High2_Shift);
	}

	UFUNCTION()
	FORCEINLINE void SetSizeType(const uint16_t NewSize, const EIdType NewType)
	{
		PackedSizeAndType = NewSize & Low14_Mask
		 | static_cast<uint16_t>(NewType & 0x3) << High2_Shift;
	}

	UFUNCTION()
	FORCEINLINE void SetSize(const uint16_t NewSize) noexcept {
		PackedSizeAndType = PackedSizeAndType & High2_Shift
			 | NewSize & Low14_Mask;
	}

	UFUNCTION()
	FORCEINLINE void SetType(const EIdType NewType) noexcept {
		PackedSizeAndType = PackedSizeAndType & Low14_Mask
			 | static_cast<uint16_t>(NewType & 0x3) << High2_Shift & High2_Shift;
	}

	friend FArchive& operator<<(FArchive& Ar, FPiece& Piece)
	{
		Ar << Piece.Id;
		Ar << Piece.PackedSizeAndType;
		return Ar;
	}
};
