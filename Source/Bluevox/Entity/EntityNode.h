#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EntityNode.generated.h"

class AEntityFacade;

UCLASS()
class BLUEVOX_API UEntityNode : public UObject
{
	GENERATED_BODY()
public:
	UEntityNode() = default;

	UFUNCTION()
	UEntityNode* Init(AEntityFacade* InFacade);

	UFUNCTION()
	void OnServerTick(float DeltaSeconds);

	UFUNCTION()
	void OnFacadeStateChanged();

	UFUNCTION()
	AEntityFacade* GetFacade() const { return Facade.Get(); }

private:
	UPROPERTY()
	TWeakObjectPtr<AEntityFacade> Facade;
};