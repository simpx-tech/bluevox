#pragma once

inline void PrintSystemError()
{
	// Buffer to store the error message
	TCHAR ErrorBuffer[2048];

	// Retrieve the system error message
	FPlatformMisc::GetSystemErrorMessage(ErrorBuffer, 2048, 0);

	// Convert a TCHAR array to FString for logging
	const FString ErrorMessage(ErrorBuffer);

	UE_LOG(LogTemp, Error, TEXT("System Error: %s"), *ErrorMessage);
}