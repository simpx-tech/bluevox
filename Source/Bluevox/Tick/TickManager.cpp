// Fill out your copyright notice in the Description page of Project Settings.


#include "TickManager.h"

#include "Bluevox/Game/GameRules.h"

void UTickManager::RemoveTickableByIndexes(const UClass* Class, const TArray<int32>& Indexes)
{
	if (!TickablesByClass.Contains(Class))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemoveTickableByIndexes] Tickable class %s not found in TickablesByClass map."), *Class->GetName());
		return;
	}
	
	for (const int32 Index : Indexes)
	{
		TickablesByClass[Class].RemoveAt(Index);
	}
}

void UTickManager::Tick(float DeltaTime)
{
	CurrentBudget = CalculatedTickBudget;
	
	// Continue previous tickables processing
	if (bRunningGameTick)
	{
		GameTick();
	}
	
	while (!ScheduledFns.IsEmpty() && CurrentBudget > 0)
	{
		TFunction<void()> HeavyFn;
		if (ScheduledFns.Dequeue(HeavyFn))
		{
			const auto StartTime = FPlatformTime::Cycles();
			HeavyFn();
			const auto EndTime = FPlatformTime::Cycles();
			CurrentBudget -= EndTime - StartTime;
		}
	}

	if (FPlatformTime::Cycles() > LastGameTickTime + CalculatedCyclesNeededToTick && CurrentBudget > 0)
	{
		PrepareForGameTick();
		GameTick();
	}
}

TStatId UTickManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTickManager, STATGROUP_Tickables);
}

void UTickManager::PrepareForGameTick()
{
	bRunningGameTick = true;
	CurrentTickableIndex = 0;
	CurrentTickableClassIndex = TickableClasses.Num() > 0 ? 0 : -1;
}

void UTickManager::GameTick()
{
	TArray<int32> ToRemoveIdx;
	
	ON_SCOPE_EXIT {
		CurrentTickableIndex -= ToRemoveIdx.Num();
		RemoveTickableByIndexes(CurrentTickableClass, ToRemoveIdx);
	};
	
	bRunningGameTick = true;
	const auto DeltaTime = GetWorld()->GetDeltaSeconds();
	for (auto& Pair : TickablesByClass)
	{
		CurrentTickableClass = Pair.Key;
		const auto Len = Pair.Value.Num();
		
		for (int i = 0; i < Len; ++i)
		{
			const auto& TickableInterface = Pair.Value[i];
			if (TickableInterface)
			{
				TickableInterface->GameTick(DeltaTime);
			} else
			{
				ToRemoveIdx.Add(i);
			}
		}

		RemoveTickableByIndexes(CurrentTickableClass, ToRemoveIdx);
	}

	bRunningGameTick = false;
}

void UTickManager::RecalculateBudget()
{
	const double SecondsPerCycle = FPlatformTime::GetSecondsPerCycle();
	
	const auto TickRate = 1.0f / GameRules::Tick::TicksPerSecond;
	CalculatedCyclesNeededToTick = static_cast<uint64>(SecondsPerCycle / TickRate);

	const double BudgetSeconds = GameRules::Tick::TickBudget * 1e-9;
	CalculatedTickBudget = static_cast<uint64>(BudgetSeconds / SecondsPerCycle);
}

void UTickManager::Init()
{
	RecalculateBudget();
}

void UTickManager::RegisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject)
{
	if (!TickableObject)
	{
		return;
	}

	const UObject* Obj = TickableObject.GetObject();
	UClass*  ClassKey = Obj->GetClass();
	TArray<TScriptInterface<IGameTickable>>& Bucket = TickablesByClass.FindOrAdd(ClassKey);
	Bucket.AddUnique(TickableObject);
}

void UTickManager::UnregisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject)
{
	if (!TickableObject)
	{
		return;
	}

	const UObject* Obj = TickableObject.GetObject();
	const UClass* ClassKey = Obj->GetClass();
	if (TArray<TScriptInterface<IGameTickable>>* Bucket = TickablesByClass.Find(ClassKey))
	{
		Bucket->Remove(TickableObject);
		if (Bucket->Num() == 0)
		{
			TickablesByClass.Remove(ClassKey);
		}
	}
}

void UTickManager::ScheduleFn(TFunction<void()>&& Func)
{
	ScheduledFns.Enqueue(MoveTemp(Func));
}

template <typename AsyncFunc, typename ThenFunc, typename ValidateFunc = decltype([](auto&&){ return true; })>
void UTickManager::RunAsyncThen(AsyncFunc&& AsyncFn, ThenFunc&& ThenFn,
	ValidateFunc&& StillValidFn)
{
	auto Fut = Async(EAsyncExecution::ThreadPool, AsyncFn);
	using FutureType = decltype(Fut);
	using ReturnType = typename FutureType::ElementType;

	Fut.Then([ThenFn = MoveTemp(ThenFn), StillValidFn = MoveTemp(StillValidFn)](TFuture<ReturnType>&& Future)
	{
		if constexpr (std::is_same_v<ReturnType, void>)
		{
			ScheduleFn(
				[ThenFn = MoveTemp(ThenFn), StillValidFn = MoveTemp(StillValidFn)]
				{
					if (StillValidFn())
						ThenFn();
				}
			);
		}
		else
		{
			auto Result = Future.Get();
			ScheduleFn(
				[ThenFn = MoveTemp(ThenFn), StillValidFn = MoveTemp(StillValidFn), Result = MoveTemp(Result)]
				{
					if (StillValidFn(Result))
						ThenFn(Result);
				}
			);
		}
	});
}
