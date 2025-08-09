#pragma once

#include "CoreMinimal.h"
#include "SegmentedHeader.h"
#include "Bluevox/Utils/PrintSystemError.h"

struct FSegmentedFile
{
	FSegmentedFile()
	{
	}

	~FSegmentedFile()
	{
		FWriteScopeLock WriteLock(FileLock);
		if (FileHandle)
		{
			FileHandle->Flush();
			FileHandle.Reset();
		}
	}

	void WriteZeroes(const uint32 AtPosition, const int64 NumZeroBytes) const
	{
		constexpr int32 ChunkSize = 4096;
		constexpr uint8 ZeroChunk[ChunkSize] = {};

		int64 Remaining = NumZeroBytes;
		FileHandle->Seek(AtPosition);
		while (Remaining > 0)
		{
			const int64 ToWrite = FMath::Min(Remaining, static_cast<int64>(ChunkSize));
			if (!FileHandle->Write(ZeroChunk, ToWrite))
			{
				PrintSystemError();
				UE_LOG(LogTemp, Error, TEXT("Failed to write zero bytes."));
				break;
			}
			Remaining -= ToWrite;
		}
	}

	bool Th_WriteSegment(const int32 Index, const TArray<uint8>& Data)
	{
		FWriteScopeLock WriteLock(FileLock);
		if (!Header.SectionsHeaders.IsValidIndex(Index))
		{
			ensureMsgf(false, TEXT("Invalid section index: %d"), Index);
			return false;
		}

		auto& SectionHeader = Header.SectionsHeaders[Index];

		// TODO consider case where it fails to write the segment, we should not update the header in that case
		// TODO make a stage/tmp file or more advanced safe strategies
		const auto TotalSize = Data.Num();
		if (TotalSize > static_cast<int32>(SectionHeader.SegmentsUsed * Header.SegmentSize))
		{
			const auto SegmentsNeeded = FMath::CeilToInt(static_cast<float>(TotalSize) / Header.SegmentSize);
			const auto SegmentsAdded = SegmentsNeeded - SectionHeader.SegmentsUsed;
			const int64 BytesOffset = SegmentsAdded * Header.SegmentSize;

			// If there are other segments after this one, we need to shift them
			if (Index + 1 < Header.SectionsHeaders.Num())
			{
				const auto NextHeader = Header.SectionsHeaders[Index + 1];

				const auto LastHeader = Header.SectionsHeaders.Last();
				const int64 EndPosition = LastHeader.Offset + LastHeader.SegmentsUsed * Header.SegmentSize;

				const auto ShiftSize = EndPosition - NextHeader.Offset;

				TArray<uint8> ToMove;
				ToMove.SetNumUninitialized(ShiftSize);

				FileHandle->Seek(NextHeader.Offset);
				if (!FileHandle->Read(ToMove.GetData(), ShiftSize))
				{
					PrintSystemError();
					ensureMsgf(false, TEXT("Failed to read segments to shift"));
					return false;
				}

				const auto NewOffset = NextHeader.Offset + BytesOffset;
				FileHandle->Seek(NewOffset);
				if (!FileHandle->Write(ToMove.GetData(), ToMove.Num()))
				{
					PrintSystemError();
					ensureMsgf(false, TEXT("Failed to write shifted segments to file"));
					return false;
				}

				for (int32 i = Index + 1; i < Header.SectionsHeaders.Num(); ++i)
				{
					Header.SectionsHeaders[i].Offset += BytesOffset;
				}
			}

			// Since with size 0, it's actually storing the last byte from the last segment, we need to add 1 byte offset
			if (SectionHeader.SegmentsUsed == 0)
			{
				SectionHeader.Offset++;
			}
			SectionHeader.SegmentsUsed = SegmentsNeeded;

			// Fill the rest of the section with zeroes
			const auto Size = SegmentsNeeded * Header.SegmentSize;
			WriteZeroes(SectionHeader.Offset + TotalSize, Size - TotalSize);
		}

		// Update headers on disk
		SectionHeader.Size = TotalSize;
		Header.WriteTo(FileHandle.Get());
		FileHandle->Flush();

		// Finally, write the data
		FileHandle->Seek(Header.SectionsHeaders[Index].Offset);
		if (!FileHandle->Write(Data.GetData(), TotalSize))
		{
			PrintSystemError();
			ensureMsgf(false, TEXT("Failed to write segment data to file"));
			return false;
		}

		return true;
	}

	bool Th_ReadSegment(const int32 Index, TArray<uint8>& OutData)
	{
		FReadScopeLock ReadLock(FileLock);
		if (!Header.SectionsHeaders.IsValidIndex(Index))
		{
			ensureMsgf(false, TEXT("Invalid section index: %d"), Index);
			return false;
		}

		if (Header.SectionsHeaders[Index].SegmentsUsed == 0)
		{
			OutData.Empty();
			return true;
		}
		
		const auto TotalSize = Header.SectionsHeaders[Index].Size;

		OutData.SetNumUninitialized(TotalSize);
		if (!FileHandle->ReadAt(OutData.GetData(), TotalSize, Header.SectionsHeaders[Index].Offset))
		{
			PrintSystemError();
			ensureMsgf(false, TEXT("Failed to read segment data from file at index %d"), Index);
			return false;
		}

		return true;
	}

	static TSharedPtr<FSegmentedFile> CreateOnDisk(const FString& FilePath, const uint32 SegmentedSize, const uint32 SegmentsCount)
	{
		if (SegmentsCount == 0 || SegmentedSize == 0)
		{
			ensureMsgf(false, TEXT("SegmentsCount and SegmentedSize must be greater than 0"));
			return nullptr;
		}

		if (FPaths::FileExists(FilePath))
		{
			ensureMsgf(false, TEXT("File already exists: %s"), *FilePath);
			return nullptr;
		}

		const auto SegmentedFile = MakeShared<FSegmentedFile>();

		SegmentedFile->FileHandle = TUniquePtr<IFileHandle>(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*FilePath, true, true));
		
		SegmentedFile->Header = FSegmentedHeader{SegmentedSize, SegmentsCount};
		SegmentedFile->Header.WriteTo(SegmentedFile->FileHandle.Get());
		SegmentedFile->FileHandle->Flush();

		return SegmentedFile; 
	}

	static TSharedPtr<FSegmentedFile> LoadFromDisk(const FString& FilePath)
	{
		if (!FPaths::FileExists(FilePath))
		{
			ensureMsgf(false, TEXT("File does not exist: %s"), *FilePath);
			return nullptr;
		}

		const auto SegmentedFile = MakeShared<FSegmentedFile>();
		SegmentedFile->FileHandle = TUniquePtr<IFileHandle>(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*FilePath, true, true));

		if (!SegmentedFile->FileHandle)
		{
			PrintSystemError();
			ensureMsgf(false, TEXT("Failed to open file: %s"), *FilePath);
			return nullptr;
		}

		FSegmentedHeader::ReadFrom(SegmentedFile->FileHandle.Get(), SegmentedFile->Header);

		return SegmentedFile;
	}
private:
	FRWLock FileLock;

	TUniquePtr<IFileHandle> FileHandle;

	FSegmentedHeader Header;
};
