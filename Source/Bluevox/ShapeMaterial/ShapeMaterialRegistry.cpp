// Fill out your copyright notice in the Description page of Project Settings.


#include "ShapeMaterialRegistry.h"

#include "ShapeMaterial.h"

void UMaterialRegistry::RegisterMaterial(const FName& MaterialName, UTexture2D* Material)
{
	// DEV
}

void UMaterialRegistry::RegisterAll()
{
	TArray<UClass*> AllClasses;
	GetDerivedClasses(UShapeMaterial::StaticClass(), AllClasses, true);

	TSet<UClass*> NonLeafClasses;
	for (const UClass* Cls : AllClasses)
	{
		if (Cls->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}
		
		// Walk up the inheritance chain marking every parent as “non-leaf”
		for (UClass* Parent = Cls->GetSuperClass(); 
			 Parent && Parent != UPrimaryDataAsset::StaticClass(); 
			 Parent = Parent->GetSuperClass())
		{
			NonLeafClasses.Add(Parent);
		}
	}

	TArray<UClass*> LeafClasses;
	for (UClass* Cls : AllClasses)
	{
		if (Cls->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}
		if (!NonLeafClasses.Contains(Cls))
		{
			LeafClasses.Add(Cls);
		}
	}

	for (const UClass* Leaf : LeafClasses)
	{
		const UShapeMaterial* CDO = Leaf->GetDefaultObject<UShapeMaterial>();
		RegisterMaterial(CDO->GetNameId(), CDO->Texture);
	}
}

void UMaterialRegistry::Serialize(FArchive& Ar)
{
	Ar << Materials;
}
