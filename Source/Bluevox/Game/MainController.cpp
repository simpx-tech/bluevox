// Fill out your copyright notice in the Description page of Project Settings.


#include "MainController.h"

#include "GameManager.h"
#include "LogMainController.h"
#include "MainCharacter.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/VirtualMap/VirtualMap.h"
#include "Bluevox/Network/PlayerNetwork.h"
#include "Bluevox/Network/UpdateChunkNetworkPacket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/IConsoleManager.h"

AMainController::AMainController()
{
	PlayerNetwork = CreateDefaultSubobject<UPlayerNetwork>(TEXT("PlayerNetwork"));
}

void AMainController::Sv_CheckReady()
{
	if (bServerReady && bClientReady)
	{
		Multicast_OnFullReady();
	}
}

void AMainController::Multicast_OnFullReady_Implementation()
{
	UE_LOG(LogMainController, Log, TEXT("Player %s is fully ready"), *GetName());

	if (GameManager->bServer)
	{
		GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		SetActorEnableCollision(true);

		if (GameManager->bClient)
		{
			OnPlayerReady.Broadcast();
		}
	} else
	{
		GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		SetActorEnableCollision(true);
		OnPlayerReady.Broadcast();
	}
}

int32 AMainController::GetFarDistance() const
{
	return FarDistance;
}

FBlockRaycastResult AMainController::BlockRaycast() const
{
	FVector ViewLoc; FRotator ViewRot;
	GetPlayerViewPoint(ViewLoc, ViewRot);
	const FVector Dir = ViewRot.Vector();
	const FVector Start = ViewLoc;
	const FVector End = Start + Dir * GameConstants::Distances::InteractionDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VoxelPick), true);
	Params.bReturnPhysicalMaterial = false;
	Params.AddIgnoredActor(GetPawn());

	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		return FBlockRaycastResult();

	// Snap normal to the principal axis
	const FVector N = Hit.ImpactNormal;
	const int32 Axis = FMath::Abs(N.X) > FMath::Abs(N.Y)
		? (FMath::Abs(N.X) > FMath::Abs(N.Z) ? 0 : 2)
		: FMath::Abs(N.Y) > FMath::Abs(N.Z) ? 1 : 2;
	FVector Face(0,0,0);
	if (Axis == 0) Face.X = N.X >= 0 ? 1 : -1;
	if (Axis == 1) Face.Y = N.Y >= 0 ? 1 : -1;
	if (Axis == 2) Face.Z = N.Z >= 0 ? 1 : -1;

	// DrawDebugLine(
	// 	GetWorld(), Start, Hit.ImpactPoint, FColor::Yellow, false, -1.0f, 0, 1.0f);
	// DrawDebugLine(
	// 	GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Face * GameRules::Scaling::ZSize, FColor::Red, false, -1.0f, 0);
	
	const double Increment = 0.25 * (Axis == 2 ? GameConstants::Scaling::ZWorldSize : GameConstants::Scaling::XYWorldSize);
	
	FBlockRaycastResult Result;
	Result.bHit = true;
	Result.Position = FGlobalPosition::FromActorLocation(Hit.ImpactPoint - Face * Increment);
	Result.PlacePosition = FGlobalPosition::FromActorLocation(Hit.ImpactPoint + Face * Increment);

	return Result;
}

void AMainController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const auto ActorPosition = FGlobalPosition::FromActorLocation(GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector);
	GEngine->AddOnScreenDebugMessage(
		0, 0.0f, FColor::Blue,
		FString::Printf(TEXT("Position: %s"), *ActorPosition.ToString()));
	
	LastRaycastResult = BlockRaycast();
	if (LastRaycastResult.bHit)
	{
		GEngine->AddOnScreenDebugMessage(
			1, 0.0f, FColor::Green,
			FString::Printf(TEXT("Looking at block: %s"), *LastRaycastResult.Position.ToString()));
		GEngine->AddOnScreenDebugMessage(
			2, 0.0f, FColor::Blue,
			FString::Printf(TEXT("Place position: %s"), *LastRaycastResult.PlacePosition.ToString()));

		DrawDebugBox(
			GetWorld(), LastRaycastResult.Position.AsActorLocationCopy() + FVector(GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::ZWorldSize * 0.5f),
			FVector(GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::ZWorldSize * 0.5f),
			FQuat::Identity, FColor::Green, false, -1.0f);
		DrawDebugBox(
			GetWorld(), LastRaycastResult.PlacePosition.AsActorLocationCopy() + FVector(GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::ZWorldSize * 0.5f),
			FVector(GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::ZWorldSize * 0.5f),
			FQuat::Identity, FColor::Blue, false, -1.0f);
	}
}

void AMainController::RSv_SetFarDistance_Implementation(const int32 NewFarDistance)
{
	const auto OldFarDistance = FarDistance;
	FarDistance = NewFarDistance;
	GameManager->VirtualMap->Sv_UpdateFarDistanceForPlayer(this, OldFarDistance, FarDistance);
}

void AMainController::RSv_SetClientReady_Implementation(const bool bReady)
{
	UE_LOG(LogMainController, Log, TEXT("Client set to %d for player %s"), bReady, *GetName());
	bClientReady = bReady;
	Sv_CheckReady();
}

void AMainController::BeginPlay()
{
	GameManager = Cast<AGameManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameManager::StaticClass()));
	PlayerNetwork->Init(GameManager, this, PlayerState);
	
	Super::BeginPlay();

	// Apply saved graphics settings on the local player only (client side)
	if (IsLocalController())
	{
		Cl_ApplySavedGraphicsSettings();
	}

	GameManager->OnPlayerJoin.Broadcast(this);
}

// --- Graphics settings helpers and API ---
namespace
{
	static const TCHAR* const BluevoxGraphicsSection = TEXT("Bluevox.Graphics");
	static const TCHAR* const KeyPreset = TEXT("ScalabilityPreset");
	static const TCHAR* const KeyVSM = TEXT("VSMEnabled");
	static const TCHAR* const KeyLumen = TEXT("LumenEnabled");
	static const TCHAR* const KeyAA = TEXT("AAMethod");
	// DLSS keys
	static const TCHAR* const KeyDLSSEnabled = TEXT("DLSSEnabled");
	static const TCHAR* const KeyDLSSQuality = TEXT("DLSSQuality");
	static const TCHAR* const KeyDLSSSharpness = TEXT("DLSSSharpness");
}

int32 AMainController::PresetToLevel(EGraphicsQualityPreset Preset)
{
	switch (Preset)
	{
		case EGraphicsQualityPreset::Low: return 0;
		case EGraphicsQualityPreset::Medium: return 1;
		case EGraphicsQualityPreset::High: return 2;
		case EGraphicsQualityPreset::Epic: return 3;
		case EGraphicsQualityPreset::Cinematic: return 4;
		default: return 3;
	}
}

EGraphicsQualityPreset AMainController::LevelToPreset(const int32 Level)
{
	switch (Level)
	{
		case 0: return EGraphicsQualityPreset::Low;
		case 1: return EGraphicsQualityPreset::Medium;
		case 2: return EGraphicsQualityPreset::High;
		case 3: return EGraphicsQualityPreset::Epic;
		case 4: return EGraphicsQualityPreset::Cinematic;
		default: return EGraphicsQualityPreset::Epic;
	}
}

void AMainController::Cl_ApplyGraphicsQualityPreset(const EGraphicsQualityPreset Preset)
{
	if (!IsLocalController()) return;

	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->SetOverallScalabilityLevel(PresetToLevel(Preset));
		// Apply non-resolution settings immediately in PIE/editor as well
		Settings->ApplyNonResolutionSettings();
		Settings->SaveSettings();
	}

	if (GConfig)
	{
		GConfig->SetInt(BluevoxGraphicsSection, KeyPreset, PresetToLevel(Preset), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void AMainController::Cl_SetVirtualShadowMapsEnabled(const bool bEnabled)
{
	if (!IsLocalController()) return;

	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Enable")))
	{
		CVar->Set(bEnabled ? 1 : 0, ECVF_SetByGameSetting);
	}

	if (GConfig)
	{
		GConfig->SetBool(BluevoxGraphicsSection, KeyVSM, bEnabled, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void AMainController::Cl_SetLumenEnabled(const bool bEnabled)
{
	if (!IsLocalController()) return;

	// 2 = Lumen, 1 = SSGI/ScreenSpace
	if (IConsoleVariable* GiCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod")))
	{
		GiCVar->Set(bEnabled ? 2 : 1, ECVF_SetByGameSetting);
	}
	if (IConsoleVariable* ReflCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ReflectionMethod")))
	{
		ReflCVar->Set(bEnabled ? 2 : 1, ECVF_SetByGameSetting);
	}

	if (GConfig)
	{
		GConfig->SetBool(BluevoxGraphicsSection, KeyLumen, bEnabled, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void AMainController::Cl_SetAntialiasingMethod(const EBluevoxAAMethod Method)
{
	if (!IsLocalController()) return;

	int32 MethodInt = 2; // default TAA
	switch (Method)
	{
		case EBluevoxAAMethod::AA_None: MethodInt = 0; break;
		case EBluevoxAAMethod::AA_FXAA: MethodInt = 1; break;
		case EBluevoxAAMethod::AA_TAA:  MethodInt = 2; break;
		case EBluevoxAAMethod::AA_MSAA: MethodInt = 3; break;
		case EBluevoxAAMethod::AA_TSR:  MethodInt = 4; break;
		default: MethodInt = 2; break;
	}

	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod")))
	{
		CVar->Set(MethodInt, ECVF_SetByGameSetting);
	}

	if (GConfig)
	{
		GConfig->SetInt(BluevoxGraphicsSection, KeyAA, MethodInt, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void AMainController::Cl_SetDLSSSettings(const bool bEnable, const EBluevoxDLSSQuality QualityMode, const float Sharpness)
{
	if (!IsLocalController()) return;

	// Enable/disable DLSS
	if (IConsoleVariable* EnableCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable")))
	{
		EnableCVar->Set(bEnable ? 1 : 0, ECVF_SetByGameSetting);
	}

	// Map quality to Unreal DLSS quality CVAR values
	int32 QualityValue = -2; // -2 = Auto (we won't use, but keep as default)
	switch (QualityMode)
	{
		case EBluevoxDLSSQuality::DLSS_UltraPerformance: QualityValue = 4; break;
		case EBluevoxDLSSQuality::DLSS_Performance:      QualityValue = 3; break;
		case EBluevoxDLSSQuality::DLSS_Balanced:         QualityValue = 2; break;
		case EBluevoxDLSSQuality::DLSS_Quality:          QualityValue = 1; break;
		case EBluevoxDLSSQuality::DLSS_UltraQuality:     QualityValue = 0; break;
		case EBluevoxDLSSQuality::DLSS_DLAA:             QualityValue = -1; break;
		case EBluevoxDLSSQuality::DLSS_Off:              QualityValue = 1; break; // default quality when off
		default: QualityValue = 1; break;
	}

	if (bEnable)
	{
		if (IConsoleVariable* QualityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Quality")))
		{
			QualityCVar->Set(QualityValue, ECVF_SetByGameSetting);
		}
	}

	// Sharpness clamp [0..1]
	const float ClampedSharpness = FMath::Clamp(Sharpness, 0.0f, 1.0f);
	if (IConsoleVariable* SharpCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Sharpness")))
	{
		SharpCVar->Set(ClampedSharpness, ECVF_SetByGameSetting);
	}

	// Persist
	if (GConfig)
	{
		GConfig->SetBool(BluevoxGraphicsSection, KeyDLSSEnabled, bEnable, GGameUserSettingsIni);
		GConfig->SetInt(BluevoxGraphicsSection, KeyDLSSQuality, static_cast<int32>(QualityMode), GGameUserSettingsIni);
		GConfig->SetFloat(BluevoxGraphicsSection, KeyDLSSSharpness, ClampedSharpness, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

EGraphicsQualityPreset AMainController::Cl_GetSavedGraphicsQualityPreset() const
{
	int32 Level = 3;
	if (GConfig)
	{
		GConfig->GetInt(BluevoxGraphicsSection, KeyPreset, Level, GGameUserSettingsIni);
	}
	return LevelToPreset(Level);
}

bool AMainController::Cl_GetSavedVSMEnabled() const
{
	bool bEnabled = true;
	if (GConfig)
	{
		GConfig->GetBool(BluevoxGraphicsSection, KeyVSM, bEnabled, GGameUserSettingsIni);
	}
	return bEnabled;
}

bool AMainController::Cl_GetSavedLumenEnabled() const
{
	bool bEnabled = true;
	if (GConfig)
	{
		GConfig->GetBool(BluevoxGraphicsSection, KeyLumen, bEnabled, GGameUserSettingsIni);
	}
	return bEnabled;
}

EBluevoxAAMethod AMainController::Cl_GetSavedAntialiasingMethod() const
{
	int32 MethodInt = 2; // default TAA
	if (GConfig)
	{
		GConfig->GetInt(BluevoxGraphicsSection, KeyAA, MethodInt, GGameUserSettingsIni);
	}
	switch (MethodInt)
	{
		case 0: return EBluevoxAAMethod::AA_None;
		case 1: return EBluevoxAAMethod::AA_FXAA;
		case 2: return EBluevoxAAMethod::AA_TAA;
		case 3: return EBluevoxAAMethod::AA_MSAA;
		case 4: return EBluevoxAAMethod::AA_TSR;
		default: return EBluevoxAAMethod::AA_TAA;
	}
}

bool AMainController::Cl_GetSavedDLSSEnabled() const
{
	bool bEnabled = false;
	if (GConfig)
	{
		GConfig->GetBool(BluevoxGraphicsSection, KeyDLSSEnabled, bEnabled, GGameUserSettingsIni);
	}
	return bEnabled;
}

EBluevoxDLSSQuality AMainController::Cl_GetSavedDLSSQuality() const
{
	int32 Saved = static_cast<int32>(EBluevoxDLSSQuality::DLSS_Quality);
	if (GConfig)
	{
		GConfig->GetInt(BluevoxGraphicsSection, KeyDLSSQuality, Saved, GGameUserSettingsIni);
	}
	// Clamp to enum range
	Saved = FMath::Clamp(Saved, 0, static_cast<int32>(EBluevoxDLSSQuality::DLSS_DLAA));
	return static_cast<EBluevoxDLSSQuality>(Saved);
}

float AMainController::Cl_GetSavedDLSSSharpness() const
{
	float Sharpness = 0.5f;
	if (GConfig)
	{
		GConfig->GetFloat(BluevoxGraphicsSection, KeyDLSSSharpness, Sharpness, GGameUserSettingsIni);
	}
	return FMath::Clamp(Sharpness, 0.0f, 1.0f);
}

void AMainController::Cl_ApplySavedGraphicsSettings()
{
	if (!IsLocalController()) return;

	Cl_ApplyGraphicsQualityPreset(Cl_GetSavedGraphicsQualityPreset());
	Cl_SetVirtualShadowMapsEnabled(Cl_GetSavedVSMEnabled());
	Cl_SetLumenEnabled(Cl_GetSavedLumenEnabled());
	Cl_SetAntialiasingMethod(Cl_GetSavedAntialiasingMethod());
	Cl_SetDLSSSettings(Cl_GetSavedDLSSEnabled(), Cl_GetSavedDLSSQuality(), Cl_GetSavedDLSSSharpness());
}

void AMainController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (PlayerState && GameManager->bInitialized && GameManager->bClient)
	{
		PlayerNetwork->NotifyClientNetReady();
	}
}

void AMainController::RSv_LeftClick_Implementation()
{
	if (LastRaycastResult.bHit)
	{
		const auto DirtShapeId = EMaterial::Void;
		
		const auto UpdateChunkPacket = NewObject<UUpdateChunkNetworkPacket>(this)->Init(LastRaycastResult.Position, FPiece(DirtShapeId, 1));
		
		// Run locally
		UpdateChunkPacket->OnReceive(GameManager);
		
		// TODO improve this, have a global network relay
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (const AMainController* MainController = Cast<AMainController>(PlayerController))
			{
				if (MainController != GameManager->LocalController)
				{
					MainController->PlayerNetwork->SendToClient(UpdateChunkPacket);
				}
			}
		}
	}
}

void AMainController::RSv_RightClick_Implementation()
{
	if (LastRaycastResult.bHit)
	{
		// const auto WaterShapeId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Textures::GShape_Water);
		
		const auto UpdateChunkPacket = NewObject<UUpdateChunkNetworkPacket>(this)->Init(LastRaycastResult.PlacePosition, FPiece(EMaterial::Stone, 4));
		
		// Run locally
		UpdateChunkPacket->OnReceive(GameManager);
		
		// TODO improve this
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (const AMainController* MainController = Cast<AMainController>(PlayerController))
			{
				if (MainController != GameManager->LocalController)
				{
					MainController->PlayerNetwork->SendToClient(UpdateChunkPacket);
				}
			}
		}
	}
}

void AMainController::Sv_SetServerReady(const bool bReady)
{
	UE_LOG(LogMainController, Log, TEXT("Server ready set to %d for player %s"), bReady, *GetName());
	bServerReady = bReady;
	GameManager->LocalCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	Sv_CheckReady();
}

void AMainController::Cl_SetClientReady(const bool bReady)
{
	RSv_SetClientReady(bReady);
}

void AMainController::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMainController, bServerReady);
	DOREPLIFETIME(AMainController, bClientReady);
}

void AMainController::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << SavedGlobalPosition;
}

void AMainController::SerializeForWorldSave(FArchive& Ar)
{
	Ar << SavedGlobalPosition;
}
