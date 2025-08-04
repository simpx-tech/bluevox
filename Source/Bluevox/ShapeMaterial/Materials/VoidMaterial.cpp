// Fill out your copyright notice in the Description page of Project Settings.


#include "VoidMaterial.h"

#include "Bluevox/Game/GameRules.h"

FName UVoidMaterial::GetNameId() const
{
	return GameRules::Constants::GMaterial_Void;
}
