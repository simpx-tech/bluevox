// Fill out your copyright notice in the Description page of Project Settings.


#include "StoneLayerShape.h"

#include "Bluevox/Game/GameConstants.h"

uint16 UStoneLayerShape::GetMaterialId() const
{
	return 2;
}

FName UStoneLayerShape::GetNameId() const
{
	return GameConstants::Shapes::GShape_Layer_Stone;
}
