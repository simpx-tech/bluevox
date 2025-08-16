// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Chunk/Position/GlobalPosition.h"
#include "GameFramework/PlayerController.h"
#include "MainController.generated.h"

struct FPiece;
class UInputAction;
class AGameManager;
class UPlayerNetwork;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClientReady);

struct FBlockRaycastResult
{
	bool bHit = false;
	FGlobalPosition Position;
	FGlobalPosition PlacePosition;
};

/**
 * 
 */
UCLASS()
class BLUEVOX_API AMainController : public APlayerController
{
	GENERATED_BODY()

	AMainController();

	UFUNCTION(Server, Reliable)
	void RSv_SetClientReady(bool bReady);

	UPROPERTY(EditAnywhere, Replicated)
	int32 FarDistance = 12;

	UFUNCTION()
	void Sv_CheckReady();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnFullReady();
	
public:
	UPROPERTY()
	uint16 Id = 0;
	
	// DEV remember to update this before saving
	UPROPERTY()
	FGlobalPosition SavedGlobalPosition;
	
	UFUNCTION()
	int32 GetFarDistance() const;
	
	UFUNCTION(Server, Reliable)
	void RSv_SetFarDistance(int32 NewFarDistance);

	FBlockRaycastResult BlockRaycast() const;

	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(BlueprintReadWrite)
	UPlayerNetwork* PlayerNetwork;
	
	UPROPERTY(BlueprintReadWrite)
	AGameManager* GameManager;
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnClientReady OnPlayerReady;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bServerReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bClientReady = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LeftClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* RightClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputMappingContext* DefaultInputContext;

	FBlockRaycastResult LastRaycastResult;
	
	virtual void BeginPlay() override;

	virtual void OnRep_PlayerState() override;

	UFUNCTION(Reliable, Server)
	void RSv_LeftClick();

	UFUNCTION(Reliable, Server)
	void RSv_RightClick();
	
	UFUNCTION(BlueprintCallable)
	void Sv_SetServerReady(bool bReady);

	UFUNCTION(BlueprintCallable)
	void Cl_SetClientReady(bool bReady);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Serialize(FArchive& Ar) override;

	void SerializeForWorldSave(FArchive& Ar);
};
