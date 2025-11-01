// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Chunk/Position/GlobalPosition.h"
#include "GameFramework/PlayerController.h"
#include "MainController.generated.h"

UENUM(BlueprintType)
enum class EGraphicsQualityPreset : uint8
{
	Low      UMETA(DisplayName = "Low"),
	Medium   UMETA(DisplayName = "Medium"),
	High     UMETA(DisplayName = "High"),
	Epic     UMETA(DisplayName = "Epic"),
	Cinematic UMETA(DisplayName = "Cinematic")
};

// Use a project-specific name to avoid colliding with engine enums (EAntiAliasingMethod)
UENUM(BlueprintType)
enum class EBluevoxAAMethod : uint8
{
	AA_None UMETA(DisplayName = "None"),
	AA_FXAA UMETA(DisplayName = "FXAA"),
	AA_TAA UMETA(DisplayName = "TAA"),
	AA_MSAA UMETA(DisplayName = "MSAA"),
	AA_TSR UMETA(DisplayName = "TSR")
};

// DLSS quality modes (mirrors DLSS plugin order)
UENUM(BlueprintType)
enum class EBluevoxDLSSQuality : uint8
{
	DLSS_Off             UMETA(DisplayName = "Off"),
	DLSS_UltraPerformance UMETA(DisplayName = "UltraPerformance"),
	DLSS_Performance      UMETA(DisplayName = "Performance"),
	DLSS_Balanced         UMETA(DisplayName = "Balanced"),
	DLSS_Quality          UMETA(DisplayName = "Quality"),
	DLSS_UltraQuality     UMETA(DisplayName = "UltraQuality"),
	DLSS_DLAA             UMETA(DisplayName = "DLAA")
};

struct FPiece;
class UInputAction;
class AGameManager;
class UPlayerNetwork;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClientReady);

struct FBlockRaycastResult
{
	bool bHit = false;
	FGlobalPosition Position;
	FGlobalPosition PlacePosition;
};

/**
 * 
 */
UCLASS()
class BLUEVOX_API AMainController : public APlayerController
{
	GENERATED_BODY()

private:
	void Cl_ApplySavedGraphicsSettings();
	static int32 PresetToLevel(EGraphicsQualityPreset Preset);
	static EGraphicsQualityPreset LevelToPreset(int32 Level);

public:
	AMainController();

	UFUNCTION(Server, Reliable)
	void RSv_SetClientReady(bool bReady);

	UPROPERTY(EditAnywhere, Replicated)
	int32 FarDistance = 12;

	UFUNCTION()
	void Sv_CheckReady();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnFullReady();
	
public:
	UPROPERTY()
	uint16 Id = 0;

	// Graphics settings API (client-side)
	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void Cl_ApplyGraphicsQualityPreset(EGraphicsQualityPreset Preset);

	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void Cl_SetVirtualShadowMapsEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void Cl_SetLumenEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void Cl_SetAntialiasingMethod(EBluevoxAAMethod Method);

	// DLSS controls
	UFUNCTION(BlueprintCallable, Category = "Graphics|DLSS")
	void Cl_SetDLSSSettings(bool bEnable, EBluevoxDLSSQuality QualityMode, float Sharpness = 0.5f);

	UFUNCTION(BlueprintPure, Category = "Graphics|DLSS")
	bool Cl_GetSavedDLSSEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Graphics|DLSS")
	EBluevoxDLSSQuality Cl_GetSavedDLSSQuality() const;

	UFUNCTION(BlueprintPure, Category = "Graphics|DLSS")
	float Cl_GetSavedDLSSSharpness() const;

	UFUNCTION(BlueprintPure, Category = "Graphics")
	EGraphicsQualityPreset Cl_GetSavedGraphicsQualityPreset() const;

	UFUNCTION(BlueprintPure, Category = "Graphics")
	bool Cl_GetSavedVSMEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Graphics")
	bool Cl_GetSavedLumenEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Graphics")
	EBluevoxAAMethod Cl_GetSavedAntialiasingMethod() const;
	
	// DEV remember to update this before saving
	UPROPERTY()
	FGlobalPosition SavedGlobalPosition;
	
	UFUNCTION()
	int32 GetFarDistance() const;
	
	UFUNCTION(Server, Reliable)
	void RSv_SetFarDistance(int32 NewFarDistance);

	FBlockRaycastResult BlockRaycast() const;

	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(BlueprintReadWrite)
	UPlayerNetwork* PlayerNetwork;
	
	UPROPERTY(BlueprintReadWrite)
	AGameManager* GameManager;
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnClientReady OnPlayerReady;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bServerReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bClientReady = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LeftClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* RightClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* PickupItemAction;

	// Quick inventory slot input actions (1-12)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* QuickSlot1Action;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* QuickSlot2Action;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* QuickSlot3Action;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* QuickSlot4Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputMappingContext* DefaultInputContext;

	FBlockRaycastResult LastRaycastResult;
	
	virtual void BeginPlay() override;

	virtual void OnRep_PlayerState() override;

	UFUNCTION(Reliable, Server)
	void RSv_LeftClick();

	UFUNCTION(Reliable, Server)
	void RSv_RightClick();
	
	UFUNCTION(BlueprintCallable)
	void Sv_SetServerReady(bool bReady);

	UFUNCTION(BlueprintCallable)
	void Cl_SetClientReady(bool bReady);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Serialize(FArchive& Ar) override;

	void SerializeForWorldSave(FArchive& Ar);
};
