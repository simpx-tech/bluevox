// Fill out your copyright notice in the Description page of Project Settings.


#include "VoidShape.h"

#include "Bluevox/Game/GameRules.h"

FName UVoidShape::GetNameId() const
{
	return GameRules::Constants::GShape_Void;
}

int32 UVoidShape::GetMaterialCost() const
{
	return 0;
}
