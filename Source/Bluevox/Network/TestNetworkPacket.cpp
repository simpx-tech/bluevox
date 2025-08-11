// Fill out your copyright notice in the Description page of Project Settings.


#include "TestNetworkPacket.h"

void UTestNetworkPacket::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << TestValue;
	Ar << TestString;
	Ar << TestArray;
}

void UTestNetworkPacket::Initialize(const int32 Size)
{
	TestArray.Reserve(Size);
	for (int32 i = 0; i < Size; ++i)
	{
		TestArray.Add(i % 256);
	}
}
