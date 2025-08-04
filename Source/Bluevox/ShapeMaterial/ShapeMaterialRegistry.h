
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

	void RegisterMaterial(const FName& MaterialName, UMaterialInterface* Material);
	
public:
	void RegisterAll();

	uint8 GetMaterialByName(const FName& MaterialName) const
	{
		if (const int32* Index = MaterialByName.Find(MaterialName))
		{
			return *Index;
		}
		
		return INDEX_NONE;
	}

	virtual void Serialize(FArchive& Ar) override;
};
