// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainController.generated.h"

class UInputAction;
class AGameManager;
class UPlayerNetwork;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClientReadyChanged, bool, bReady);

/**
 * 
 */
UCLASS()
class BLUEVOX_API AMainController : public APlayerController
{
	GENERATED_BODY()

	AMainController();

	UFUNCTION(Client, Reliable)
	void Sv_SetClientReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category = "Game")
	void HandleOnClientReadyChanged();

	UPROPERTY(Replicated)
	int32 FarDistance = 12;
	
public:
	UFUNCTION()
	int32 GetFarDistance() const;
	
	UFUNCTION(Server, Reliable)
	void SetFarDistance(int32 NewFarDistance);
	
	UPROPERTY(BlueprintReadWrite)
	UPlayerNetwork* PlayerNetwork;
	
	UPROPERTY(BlueprintReadWrite)
	AGameManager* GameManager;
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnClientReadyChanged OnClientReadyChanged;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bServerReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = HandleOnClientReadyChanged)
	bool bClientReady = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputMappingContext* DefaultInputContext;

	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void SetServerReady(bool bReady);

	UFUNCTION(BlueprintCallable)
	void SetClientReady(bool bReady);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
