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
	int32 PendingTasks = 0;

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

	// TODO handle BeginDestroy -> wait for all tasks to finish
	
public:
	void RecalculateBudget();
	
	UTickManager* Init();
	
	void RegisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject);

	void UnregisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject);

	template <typename AsyncFunc, typename ThenFunc, typename ValidateFunc = decltype([](auto&&){ return true; })>
	void RunAsyncThen(
		AsyncFunc&& AsyncFn,
		ThenFunc&& ThenFn,
		ValidateFunc&& StillValidFn = [](auto&&) { return true; }
	);

	void ScheduleFn(TFunction<void()>&& Func);
};

template <typename AsyncFunc, typename ThenFunc, typename ValidateFunc>
void UTickManager::RunAsyncThen(AsyncFunc&& AsyncFn, ThenFunc&& ThenFn, ValidateFunc&& StillValidFn)
{
	// bump our counter immediately
	PendingTasks++;

	// figure out what AsyncFn() returns
	using ReturnType = std::invoke_result_t<AsyncFunc>;

	// fire off the background task
	auto Fut = Async(EAsyncExecution::ThreadPool, Forward<AsyncFunc>(AsyncFn));

	Fut.Then([Instance = this,
			  ThenFn      = MoveTemp(ThenFn),
			  StillValidFn = MoveTemp(StillValidFn)](TFuture<ReturnType>&& Future) mutable
	{
		if constexpr (std::is_void_v<ReturnType>)
		{
			Instance->ScheduleFn(
				[Instance,
				 ThenFn      = MoveTemp(ThenFn),
				 StillValidFn = MoveTemp(StillValidFn)]() mutable
				{
					if (StillValidFn())
					{
						ThenFn();
					}
					Instance->PendingTasks--;
				});
		}
		else
		{
			ReturnType Result = Future.Get();
			Instance->ScheduleFn(
				[Instance,
				 ThenFn      = MoveTemp(ThenFn),
				 StillValidFn = MoveTemp(StillValidFn),
				 Result      = MoveTemp(Result)]() mutable
				{
					if (StillValidFn(Result))
					{
						ThenFn(Result);
					}
					Instance->PendingTasks--;
				});
		}
	});
}
