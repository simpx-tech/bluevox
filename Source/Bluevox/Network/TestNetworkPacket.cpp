// Fill out your copyright notice in the Description page of Project Settings.


#include "TestNetworkPacket.h"

void UTestNetworkPacket::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << TestValue;
	Ar << TestString;
	Ar << TestArray;
}

void UTestNetworkPacket::Initialize()
{
	TestArray.Reserve(1'000'000);
	for (int32 i = 0; i < 1'000'000; ++i)
	{
		TestArray.Add(i % 256);
	}
}
