#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Utils/PrintSystemError.h"
#include "SegmentedHeader.generated.h"

USTRUCT(BlueprintType)
struct FSectionHeader
{
	GENERATED_BODY()
	
	UPROPERTY()
	uint32 Offset = 0;

	UPROPERTY()
	uint32 Size = 0;

	UPROPERTY()
	uint16 SegmentsUsed = 0;

	UPROPERTY()
	int32 Version = 1;
};

USTRUCT(BlueprintType)
struct FSegmentedHeaderBase
{
	GENERATED_BODY()
	
	FSegmentedHeaderBase() {}

	FSegmentedHeaderBase(const uint32 InSegmentSize, const uint32 InSectionsCount)
		: SegmentSize(InSegmentSize)
		, SectionsCount(InSectionsCount)
	{
	}
	
	UPROPERTY()
	uint32 SegmentSize = 0;

	UPROPERTY()
	uint32 SectionsCount = 0;

	UPROPERTY()
	uint32 TotalHeaderSize = 0;
};

USTRUCT(BlueprintType)
struct FSegmentedHeader : public FSegmentedHeaderBase
{
	GENERATED_BODY()

	FSegmentedHeader()
	{
	}

	FSegmentedHeader(const uint32 InSegmentSize, const uint32 InSectionsCount)
		: FSegmentedHeaderBase(InSegmentSize, InSectionsCount)
	{
		TotalHeaderSize = sizeof(FSegmentedHeaderBase) +
								  static_cast<uint64>(InSectionsCount) * sizeof(FSectionHeader);

		SectionsHeaders.SetNum(InSectionsCount);
		for (uint32 i = 0; i < InSectionsCount; ++i)
		{
			SectionsHeaders[i].Offset = TotalHeaderSize;
		}
	}

	UPROPERTY()
	TArray<FSectionHeader> SectionsHeaders;

	static bool ReadFrom(IFileHandle* Handle, FSegmentedHeader& Out)
	{
		if (!Handle) { return false; }

		Handle->Seek(0);

		FSegmentedHeaderBase Flat{};
		if (!Handle->Read(reinterpret_cast<uint8*>(&Flat), sizeof(Flat)))
		{
			return false;
		}

		const int32 BytesForArray = static_cast<uint64>(Flat.SectionsCount) *
									 sizeof(FSectionHeader);
		if (Handle->Size() < static_cast<int32>(sizeof(Flat)) + BytesForArray)
		{
			// Truncated file
			return false;
		}

		Out.SegmentSize   = Flat.SegmentSize;
		Out.SectionsCount = Flat.SectionsCount;
		Out.SectionsHeaders.SetNumUninitialized(Flat.SectionsCount);

		if (Flat.SectionsCount > 0)
		{
			return Handle->Read(
				reinterpret_cast<uint8*>(Out.SectionsHeaders.GetData()),
				BytesForArray);
		}
		return true;
	}

	bool WriteTo(IFileHandle* Handle) const
	{
		if (!Handle) { return false; }

		Handle->Seek(0);

		const auto AsFlat = FSegmentedHeaderBase(SegmentSize, SectionsCount);
		if (!Handle->Write(reinterpret_cast<const uint8*>(&AsFlat), sizeof(AsFlat)))
		{
			PrintSystemError();
			return false;
		}

		if (SectionsCount > 0)
		{
			const uint64 Bytes = sizeof(FSectionHeader) * SectionsCount;
			return Handle->Write(
				reinterpret_cast<const uint8*>(SectionsHeaders.GetData()), Bytes);
		}
		
		return true;
	}
};
