// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "MainController.h"
#include "EnhancedInputSubsystems.h"  
#include "EnhancedInputComponent.h"
#include "GameManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AMainCharacter::AMainCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	const auto MovementComponent = GetCharacterMovement();
	MovementComponent->GravityScale = 0.0f;
	MovementComponent->AirControl = 1.0f;
	MovementComponent->MaxFlySpeed = 1200.f;
	MovementComponent->BrakingDecelerationFlying = 4096.0f;
	MovementComponent->MaxAcceleration = 10'000.f;
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (const AMainController* PlayerController = Cast<AMainController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(PlayerController->DefaultInputContext, 0);
		}
	}

	GameManager = Cast<AGameManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameManager::StaticClass()));

	GetCharacterMovement()->SetMovementMode(MOVE_None);

	// TODO temp
	if (GameManager->bServer)
	{
		GameManager->LocalController->SetServerReady(true);	
	} else if (GameManager->bClient)
	{
		GameManager->LocalController->SetClientReady(true);
	}
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	const auto MainController = Cast<AMainController>(GetController());
	if (MainController)
	{
		if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
		{
			EnhancedInputComponent->BindAction(MainController->MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::HandleMoveAction);

			EnhancedInputComponent->BindAction(MainController->LookAction, ETriggerEvent::Triggered, this, &AMainCharacter::HandleLookAction);

			EnhancedInputComponent->BindAction(MainController->JumpAction, ETriggerEvent::Triggered, this, &AMainCharacter::HandleJumpAction);

			EnhancedInputComponent->BindAction(MainController->CrouchAction, ETriggerEvent::Triggered, this, &AMainCharacter::HandleCrouchAction);
		}
	}
}

void AMainCharacter::HandleMoveAction(const FInputActionValue& Value)
{
	const auto Direction = Value.Get<FVector2D>();
	const auto Forward = GetActorForwardVector();
	const auto Right = GetActorRightVector();

	const FVector ForwardMovement = Forward * Direction.Y;
	const FVector RightMovement = Right * Direction.X;
	
	if (Controller)
	{
		AddMovementInput(ForwardMovement, 1.0f);
		AddMovementInput(RightMovement, 1.0f);
	}
}

void AMainCharacter::HandleLookAction(const FInputActionValue& Value)
{
	const auto LookDelta = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(LookDelta.X * LookSpeed);
		AddControllerPitchInput(-LookDelta.Y * LookSpeed);
	}
}

void AMainCharacter::HandleJumpAction(const FInputActionValue& Value)
{
	AddMovementInput(FVector::UpVector, VerticalSpeed);
}

void AMainCharacter::HandleCrouchAction(const FInputActionValue& Value)
{
	AddMovementInput(FVector::DownVector, VerticalSpeed);
}

