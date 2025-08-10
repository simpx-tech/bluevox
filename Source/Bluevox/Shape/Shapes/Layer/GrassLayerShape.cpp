// Fill out your copyright notice in the Description page of Project Settings.


#include "GrassLayerShape.h"

#include "Bluevox/Game/GameRules.h"

FName UGrassLayerShape::GetNameId() const
{
	return GameRules::Constants::GShape_Layer_Grass;
}

uint16 UGrassLayerShape::GetMaterialId() const
{
	return 3;
}
