// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NetworkPacket.h"
#include "TestNetworkPacket.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UTestNetworkPacket : public UNetworkPacket
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	int32 TestValue = 0;

	UPROPERTY(BlueprintReadWrite)
	FString TestString;

	UPROPERTY(BlueprintReadWrite)
	TArray<uint8> TestArray;

	virtual void Serialize(FArchive& Ar) override;

	UFUNCTION(BlueprintCallable)
	void Initialize(int32 Size = 1000000);
};
