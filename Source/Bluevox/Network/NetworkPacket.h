// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NetworkPacket.generated.h"

class AGameManager;
/**
 * 
 */
UCLASS(BlueprintType)
class BLUEVOX_API UNetworkPacket : public UObject
{
	GENERATED_BODY()

public:
	void Compress(TArray<uint8>& OutData);

	void DecompressAndSerialize(const TArray<uint8>& InData);

	UFUNCTION(BlueprintCallable, Category = "Network")
	virtual void OnReceive(AGameManager* GameManager);
};
