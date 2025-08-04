
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MaterialRegistry.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UMaterialRegistry : public UObject
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FName, int32> MaterialByName;

	void RegisterMaterial(const FName& MaterialName, UMaterialInterface* Material);
	
public:
	void RegisterAll();
};
