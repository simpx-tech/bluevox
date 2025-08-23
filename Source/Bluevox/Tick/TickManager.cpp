// Fill out your copyright notice in the Description page of Project Settings.


#include "TickManager.h"

#include "Bluevox/Game/GameConstants.h"

void UTickManager::RemoveTickableByIndexes(TArray<int32>& Indexes)
{
	if (Indexes.Num() == 0)
	{
		return;
	}
	
	const auto Class = TickableClasses.IsValidIndex(CurrentTickableClassIndex) ? TickableClasses[CurrentTickableClassIndex] : nullptr;
	CurrentTickableIndex -= FMath::Max(Indexes.Num(), 0);
	
	if (!TickablesByClass.Contains(Class))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemoveTickableByIndexes] Tickable class %s not found in TickablesByClass map."), *Class->GetName());
		return;
	}
	
	for (const int32 Index : Indexes)
	{
		TickablesByClass[Class].RemoveAt(Index);
	}

	if (TickablesByClass[Class].Num() == 0)
	{
		TickablesByClass.Remove(Class);
		TickableClasses.Remove(Class);
		CurrentTickableClassIndex = TickableClasses.Num() > 0 ? CurrentTickableClassIndex - 1 : -1;
	}

	Indexes.Reset();
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
		TFunction<void()> Func;
		if (ScheduledFns.Dequeue(Func))
		{
			const auto StartTime = FPlatformTime::Cycles();
			Func();
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
		RemoveTickableByIndexes(ToRemoveIdx);
	};
	
	const auto DeltaTime = GetWorld()->GetDeltaSeconds();
	for (int i = CurrentTickableClassIndex; i < TickableClasses.Num() && i != -1; i++)
	{
		const auto CurrentTickableClass = TickableClasses[i];
		const auto CurrentTickableList = TickablesByClass.FindRef(CurrentTickableClass);
		const auto Len = CurrentTickableList.Num();

		for (int j = CurrentTickableIndex; j < Len; ++j)
		{
			const auto StartTime = FPlatformTime::Cycles();
			const auto& TickableInterface = CurrentTickableList[j];
			if (TickableInterface)
			{
				TickableInterface->GameTick(DeltaTime);
			} else
			{
				ToRemoveIdx.Add(j);
			}
			const auto EndTime = FPlatformTime::Cycles();
			CurrentBudget -= EndTime - StartTime;

			// No budget and has more
			if (CurrentBudget <= 0 && i < TickableClasses.Num() - 1 && j < Len - 1)
			{
				return;
			}
		}

		RemoveTickableByIndexes(ToRemoveIdx);
	}

	bRunningGameTick = false;
}

void UTickManager::OnWorldBeginTearDown(UWorld* World)
{
	UE_LOG(LogTemp, Log, TEXT("UTickManager::OnWorldBeginTearDown called. Running scheduled functions and waiting for async ones."));
	while (PendingTasks > 0)
	{
		while (!ScheduledFns.IsEmpty())
		{
			TFunction<void()> Func;
			if (ScheduledFns.Dequeue(Func))
			{
				Func();
			}
		}

		FPlatformProcess::Sleep(0.01f);
	}

	UE_LOG(LogTemp, Log, TEXT("UTickManager::OnWorldBeginTearDown finished. All tasks completed."));
}

void UTickManager::RecalculateBudget()
{
	const double SecondsPerCycle = FPlatformTime::GetSecondsPerCycle();
	
	const auto TickRate = 1.0f / GameConstants::Tick::TicksPerSecond;
	CalculatedCyclesNeededToTick = static_cast<uint64>(SecondsPerCycle / TickRate);

	const double BudgetSeconds = GameConstants::Tick::TickBudget * 1e-9;
	CalculatedTickBudget = static_cast<uint64>(BudgetSeconds / SecondsPerCycle);
}

UTickManager* UTickManager::Init()
{
	RecalculateBudget();
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UTickManager::OnWorldBeginTearDown);
	return this;
}

void UTickManager::RegisterUObjectTickable(const TScriptInterface<IGameTickable>& TickableObject)
{
	if (!TickableObject)
	{
		return;
	}

	const UObject* Obj = TickableObject.GetObject();
	const UClass* ClassKey = Obj->GetClass();

	if (!TickablesByClass.Contains(ClassKey))
	{
		TickableClasses.Add(ClassKey);
		TickablesByClass.Add(ClassKey, {});
	}

	const auto Bucket = TickablesByClass.Find(ClassKey);
	Bucket->AddUnique(TickableObject);
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

void UTickManager::Th_ScheduleFn(TFunction<void()>&& Func)
{
	ScheduledFns.Enqueue(MoveTemp(Func));
}

ETickableTickType UTickManager::GetTickableTickType() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Always;
}