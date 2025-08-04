// Fill out your copyright notice in the Description page of Project Settings.


#include "LayerShape.h"

#include "Bluevox/Game/GameRules.h"
#include "Bluevox/ShapeMaterial/ShapeMaterialRegistry.h"

FName ULayerShape::GetNameId() const
{
	return GameRules::Constants::GShape_Layer;
}

void ULayerShape::InitializeAllowedMaterials(UMaterialRegistry* Registry)
{
	using namespace GameRules::Constants;
	AllowedMaterials.Add(Registry->GetMaterialByName(GMaterial_Glass));
	AllowedMaterials.Add(Registry->GetMaterialByName(GMaterial_Stone));
	AllowedMaterials.Add(Registry->GetMaterialByName(GMaterial_Glass));
}
