// Fill out your copyright notice in the Description page of Project Settings.


#include "ShapeMaterialRegistry.h"

void UMaterialRegistry::RegisterMaterial(const FName& MaterialName, UMaterialInterface* Material)
{
	// DEV
}

void UMaterialRegistry::RegisterAll()
{
	// DEV when looping for all, should register only the "leaf" classes, including the overriden in editor
}

void UMaterialRegistry::Serialize(FArchive& Ar)
{
	Ar << Materials;
}
