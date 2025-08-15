// Fill out your copyright notice in the Description page of Project Settings.


#include "GrassLayerShape.h"

#include "Bluevox/Game/GameConstants.h"

FName UGrassLayerShape::GetNameId() const
{
	return GameConstants::Constants::GShape_Layer_Grass;
}

uint16 UGrassLayerShape::GetMaterialId() const
{
	return 3;
}
