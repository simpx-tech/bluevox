#pragma once

#include "CoreMinimal.h"
#include "SegmentedHeader.h"

struct FSegmentedFile
{
	FSegmentedFile()
	{
	}

	~FSegmentedFile()
	{
		if (FileHandle)
		{
			FileHandle->Flush();
			FileHandle.Reset();
		}
	}

	bool WriteSegment(const int32 Index, const TArray<uint8>& Data)
	{
		FWriteScopeLock WriteLock(FileLock);
		if (!Header.SectionsHeaders.IsValidIndex(Index))
		{
			checkf(false, TEXT("Invalid section index: %d"), Index);
			return false;
		}

		// Resize if necessary
		const auto TotalSize = Data.Num();
		if (TotalSize > static_cast<int32>(Header.SectionsHeaders[Index].SegmentsUsed * Header.SegmentSize))
		{
			const auto SegmentsNeeded = FMath::CeilToInt(static_cast<float>(TotalSize) / Header.SegmentSize);
			const auto SegmentsAdded = SegmentsNeeded - Header.SectionsHeaders[Index].SegmentsUsed;
			const auto BytesOffset = SegmentsAdded * Header.SegmentSize;

			Header.SectionsHeaders[Index].SegmentsUsed = SegmentsNeeded;

			// If there are other segments after this one, we need to shift them
			if (Index + 1 < Header.SectionsHeaders.Num())
			{
				const auto NextHeader = Header.SectionsHeaders[Index + 1];
					
				for (int32 i = Index + 1; i < Header.SectionsHeaders.Num(); ++i)
				{
					Header.SectionsHeaders[i].Offset += BytesOffset;
				}

				const auto LastHeader = Header.SectionsHeaders.Last();
				const auto EndPosition = LastHeader.Offset + LastHeader.SegmentsUsed * Header.SegmentSize;

				const auto ShiftSize = EndPosition - NextHeader.Offset;

				TArray<uint8> ToMove;
				ToMove.SetNumUninitialized(ShiftSize);

				FileHandle->Seek(NextHeader.Offset);
				if (!FileHandle->Read(ToMove.GetData(), ShiftSize))
				{
					checkf(false, TEXT("Failed to read segments to shift"));
					return false;
				}

				const auto NewOffset = NextHeader.Offset + BytesOffset;
				FileHandle->Seek(NewOffset);
				if (!FileHandle->Write(ToMove.GetData(), ToMove.Num()))
				{
					checkf(false, TEXT("Failed to write shifted segments to file"));
					return false;
				}

				// Update headers on disk
				Header.WriteTo(FileHandle.Get());
			}
		}
		
		FileHandle->Seek(Header.SectionsHeaders[Index].Offset);
		if (!FileHandle->Write(Data.GetData(), TotalSize))
		{
			checkf(false, TEXT("Failed to write segment data to file"));
			return false;
		}

		return true;
	}

	bool ReadSegment(const int32 Index, TArray<uint8>& OutData)
	{
		FReadScopeLock ReadLock(FileLock);
		if (!Header.SectionsHeaders.IsValidIndex(Index))
		{
			checkf(false, TEXT("Invalid section index: %d"), Index);
			return false;
		}
		
		const auto TotalSize = Header.SectionsHeaders[Index].SegmentsUsed * Header.SegmentSize;

		FileHandle->Seek(Header.SectionsHeaders[Index].Offset);
		OutData.SetNumUninitialized(TotalSize);
		if (!FileHandle->Read(OutData.GetData(), TotalSize))
		{
			return false;
		}

		return true;
	}

	static TSharedPtr<FSegmentedFile> CreateOnDisk(const FString& FilePath, const uint32 SegmentedSize, const uint32 SegmentsCount)
	{
		if (SegmentsCount == 0 || SegmentedSize == 0)
		{
			checkf(false, TEXT("SegmentsCount and SegmentedSize must be greater than 0"));
			return nullptr;
		}

		if (FPaths::FileExists(FilePath))
		{
			checkf(false, TEXT("File already exists: %s"), *FilePath);
			return nullptr;
		}

		const auto SegmentedFile = MakeShared<FSegmentedFile>();

		SegmentedFile->FileHandle = TUniquePtr<IFileHandle>(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*FilePath, false, true));
		
		SegmentedFile->Header = FSegmentedHeader::Create(SegmentedSize, SegmentsCount);
		SegmentedFile->Header.WriteTo(SegmentedFile->FileHandle.Get());

		return SegmentedFile; 
	}

	static TSharedPtr<FSegmentedFile> LoadFromDisk(const FString& FilePath)
	{
		if (!FPaths::FileExists(FilePath))
		{
			checkf(false, TEXT("File does not exist: %s"), *FilePath);
			return nullptr;
		}

		const auto SegmentedFile = MakeShared<FSegmentedFile>();
		SegmentedFile->FileHandle = TUniquePtr<IFileHandle>(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*FilePath, false, true));

		if (!SegmentedFile->FileHandle)
		{
			checkf(false, TEXT("Failed to open file: %s"), *FilePath);
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
