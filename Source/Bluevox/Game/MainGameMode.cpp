// Fill out your copyright notice in the Description page of Project Settings.


#include "MainGameMode.h"

#include "MainCharacter.h"
#include "MainController.h"

AMainGameMode::AMainGameMode()
{
	PlayerControllerClass = AMainController::StaticClass();
	DefaultPawnClass = AMainCharacter::StaticClass();
}
