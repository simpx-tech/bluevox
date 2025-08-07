
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ShapeMaterialRegistry.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UMaterialRegistry : public UObject
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UTexture*> Materials;
	
	UPROPERTY()
	TMap<FName, int32> MaterialByName;

	void RegisterMaterial(const FName& MaterialName, UTexture2D* Material);
	
public:
	void RegisterAll();

	uint8 GetMaterialByName(const FName& MaterialName) const
	{
		if (const int32* Index = MaterialByName.Find(MaterialName))
		{
			return *Index;
		}

		UE_LOG(LogTemp, Warning, TEXT("Material %s not found in registry"), *MaterialName.ToString());
		return 0;
	}

	virtual void Serialize(FArchive& Ar) override;
};
