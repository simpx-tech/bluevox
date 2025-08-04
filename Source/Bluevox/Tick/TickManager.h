// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameTickable.h"
#include "UObject/Object.h"
#include "TickManager.generated.h"

namespace TickManagerFunc
{
	struct FNo_Validate
	{
		template<class... Ts>
		constexpr bool operator()(Ts&&...) const noexcept { return true; }
	};

	struct FNo_Finally
	{
		constexpr void operator()() const noexcept {}
	};
}

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

	// TODO handle BeginDestroy -> wait for all tasks to finish
	
public:
	void RecalculateBudget();
	
	UTickManager* Init();
	
	void RegisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject);

	void UnregisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject);

	template<
	typename  AsyncFunc,
	typename  ThenFunc,
	typename  ValidateFunc = TickManagerFunc::FNo_Validate,
	typename  FinallyFunc  = TickManagerFunc::FNo_Finally>
	void RunAsyncThen( AsyncFunc&&   AsyncFn,
				   ThenFunc&&    ThenFn,
				   ValidateFunc&& ValidateFn = {},
				   FinallyFunc&&  FinallyFn  = {} );

	void ScheduleFn(TFunction<void()>&& Func);
};

template<class A, class T, class V, class F>
void UTickManager::RunAsyncThen(A&& AsyncFn, T&& ThenFn,
								V&& ValidateFn, F&& FinallyFn)
{
	PendingTasks++;

	using R = std::invoke_result_t<A>;
	auto Fut = Async(EAsyncExecution::ThreadPool,
					 std::forward<A>(AsyncFn));

	Fut.Then([this,
			  thenFn      = std::forward<T>(ThenFn),
			  validateFn  = std::forward<V>(ValidateFn),
			  finallyFn   = std::forward<F>(FinallyFn)]
			 (TFuture<R>&& Future) mutable
	{
		auto Call_Finally = [&]
		{
			if constexpr (!std::is_same_v<std::decay_t<F>, TickManagerFunc::FNo_Finally>)
				finallyFn();
		};

		if constexpr (std::is_void_v<R>)
		{
			ScheduleFn([=, this]() mutable
			{
				if constexpr (!std::is_same_v<std::decay_t<V>, TickManagerFunc::FNo_Validate>)
				{
					if (!validateFn()) { Call_Finally(); --PendingTasks; return; }
				}
				thenFn();
				Call_Finally();
				--PendingTasks;
			});
		}
		else
		{
			R Result = Future.Get();
			ScheduleFn([=, this, res = std::move(Result)]() mutable
			{
				if constexpr (!std::is_same_v<std::decay_t<V>, TickManagerFunc::FNo_Validate>)
				{
					if (!validateFn(res)) { Call_Finally(); --PendingTasks; return; }
				}
				thenFn(std::move(res));
				Call_Finally();
				--PendingTasks;
			});
		}
	});
}
