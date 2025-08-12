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

	UFUNCTION()
	void GameTick();

	UFUNCTION()
	void OnWorldBeginTearDown(UWorld* World);
	
public:
	void RecalculateBudget();
	
	UTickManager* Init();
	
	void RegisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject);

	void UnregisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject);

	template<typename AsyncFunc, typename ThenFunc>
	void RunAsyncThen(AsyncFunc&& AsyncFn, ThenFunc&& ThenFn);

	void Th_ScheduleFn(TFunction<void()>&& Func);

	virtual ETickableTickType GetTickableTickType() const override;
};

template<class A, class T>
void UTickManager::RunAsyncThen(A&& AsyncFn, T&& ThenFn)
{
	PendingTasks++;

	using R = std::invoke_result_t<A>;
	auto Fut = Async(EAsyncExecution::ThreadPool,
					 std::forward<A>(AsyncFn));

	Fut.Then([this, thenFn = std::forward<T>(ThenFn)]
			 (TFuture<R>&& Future) mutable
	{
		if constexpr (std::is_void_v<R>)
		{
			Th_ScheduleFn([=, this]() mutable
			{
				thenFn();
				--PendingTasks;
			});
		}
		else
		{
			R Result = Future.Get();
			Th_ScheduleFn([=, this, res = std::move(Result)]() mutable
			{
				thenFn(std::move(res));
				--PendingTasks;
			});
		}
	});
}
