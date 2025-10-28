// Fill out your copyright notice in the Description page of Project Settings.


#include "InstanceTypeDataAsset.h"

FPrimaryAssetId UInstanceTypeDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId("InstanceType", GetFName());
}