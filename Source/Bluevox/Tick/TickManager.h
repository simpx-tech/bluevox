// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameTickable.h"
#include "UObject/Object.h"
#include "TickManager.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UTickManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float CalculatedCyclesNeededToTick = 0.f;

	UPROPERTY()
	int32 CalculatedTickBudget = 0;

	UPROPERTY()
	int32 CurrentBudget = 0;

	UPROPERTY()
	int32 LastGameTickTime = 0;

	UPROPERTY()
	int32 CurrentTickableClassIndex = -1;

	UPROPERTY()
	int32 CurrentTickableIndex = 0;
	
	UPROPERTY()
	bool bRunningGameTick = false;

	UPROPERTY()
	TArray<const UClass*> TickableClasses;
	
	TMap<const UClass*, TArray<TScriptInterface<IGameTickable>>> TickablesByClass;

	TQueue<TFunction<void()>, EQueueMode::Mpsc> ScheduledFns;

	UFUNCTION()
	void RemoveTickableByIndexes(TArray<int32>& Indexes);
	
	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;

	UFUNCTION()
	void PrepareForGameTick();
	
	void GameTick();
	
public:
	void RecalculateBudget();
	
	void Init();
	
	void RegisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject);

	void UnregisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject);

	template <typename AsyncFunc, typename ThenFunc, typename ValidateFunc = decltype([](auto&&){ return true; })>
void UTickManager::RunAsyncThen(AsyncFunc&& AsyncFn, ThenFunc&& ThenFn,
	ValidateFunc&& StillValidFn = [](auto&&) { return true; });

	void ScheduleFn(TFunction<void()>&& Func);
};
