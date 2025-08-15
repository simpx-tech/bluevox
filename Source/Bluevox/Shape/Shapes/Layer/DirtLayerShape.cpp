// Fill out your copyright notice in the Description page of Project Settings.


#include "DirtLayerShape.h"

#include "Bluevox/Game/GameConstants.h"

FName UDirtLayerShape::GetNameId() const
{
	return GameConstants::Constants::GShape_Layer_Dirt;
}

uint16 UDirtLayerShape::GetMaterialId() const
{
	return 1;
}
