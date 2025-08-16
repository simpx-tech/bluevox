// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "MainCharacter.generated.h"

class AGameManager;
class UInputAction;
struct FInputActionValue;

UCLASS()
class BLUEVOX_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY()
	AGameManager* GameManager = nullptr;

public:
	// Sets default values for this character's properties
	AMainCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HorizontalSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LookSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VerticalSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bReady = false;
	
	UFUNCTION()
	void HandleMoveAction(const FInputActionValue& Value);

	UFUNCTION()
	void HandleLookAction(const FInputActionValue& Value);

	UFUNCTION()
	void HandleLeftClickAction();

	UFUNCTION()
	void HandleRightClickAction();
	
	UFUNCTION()
	void HandleJumpAction(const FInputActionValue& Value);

	UFUNCTION()
	void HandleCrouchAction(const FInputActionValue& Value);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
