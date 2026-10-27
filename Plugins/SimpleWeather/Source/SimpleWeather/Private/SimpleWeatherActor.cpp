#include "SimpleWeatherActor.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "WeatherControlWidget.h"
#include "JumoLocomotionComponent.h"
#include "FairySwimmingComponent.h"
#include "SwimmingValidationActor.h"
#include "DialogueManagerComponent.h"
#include "IslandValidationActor.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InputComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
// The marketplace Blueprint has no native parent API. Marshal its single
// scalar argument using reflection so Blueprint float/double layout is respected.
bool SetWeatherSystemScalar(AActor* Target, FName Name, float Value)
{
    UFunction* Function = Target->FindFunction(Name);
    if (!Function || Function->NumParms != 1)
    {
        UE_LOG(LogTemp, Error, TEXT("WEATHER_SYSTEM: missing single-argument function %s"), *Name.ToString());
        return false;
    }
    FStructOnScope Parameters(Function);
    for (TFieldIterator<FProperty> It(Function); It; ++It)
    {
        if (!It->HasAnyPropertyFlags(CPF_Parm) || It->HasAnyPropertyFlags(CPF_ReturnParm)) continue;
        void* Address = It->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory());
        if (FFloatProperty* Float = CastField<FFloatProperty>(*It)) Float->SetPropertyValue(Address, Value);
        else if (FDoubleProperty* Double = CastField<FDoubleProperty>(*It)) Double->SetPropertyValue(Address, Value);
        else if (FBoolProperty* Bool = CastField<FBoolProperty>(*It)) Bool->SetPropertyValue(Address, Value > 0.f);
        else return false;
        Target->ProcessEvent(Function, Parameters.GetStructMemory());
        return true;
    }
    return false;
}
}

ASimpleWeatherActor::ASimpleWeatherActor()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
    Sun->SetupAttachment(RootComponent);
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->SetRelativeRotation(FRotator(-40, -35, 0));
    Sun->bAtmosphereSunLight = true;
    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(RootComponent);
    SkyLight->SetMobility(EComponentMobility::Movable);
    SkyLight->SetRealTimeCaptureEnabled(true);
    Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
    Atmosphere->SetupAttachment(RootComponent);
    Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
    Fog->SetupAttachment(RootComponent);
    Rain = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rain"));
    Rain->SetupAttachment(RootComponent);
    Rain->SetMobility(EComponentMobility::Movable);
    Rain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rain->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Rain->SetStaticMesh(Cube.Object);
    WindAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("IslandWind"));
    SurfAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("IslandSurf"));
    WindAudio->SetupAttachment(RootComponent);
    SurfAudio->SetupAttachment(RootComponent);
    WindAudio->SetAutoActivate(false);
    SurfAudio->SetAutoActivate(false);
    WindAudio->bAllowSpatialization = false;
    SurfAudio->bAllowSpatialization = false;
    static ConstructorHelpers::FObjectFinder<USoundWave> Wind(TEXT("/Game/Weather/Audio/S_Wind"));
    static ConstructorHelpers::FObjectFinder<USoundWave> Surf(TEXT("/Game/Weather/Audio/S_Surf"));
    WindAudio->SetSound(Wind.Object);
    SurfAudio->SetSound(Surf.Object);
}

void ASimpleWeatherActor::BuildRain()
{
    // Apply on load as well: existing weather maps contain the old cube material.
    if (UMaterialInterface* Water = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Weather/M_ClearRain.M_ClearRain")))
    {
        Rain->SetMaterial(0, Water);
    }
    Rain->ClearInstances();
    Drops.Reset(400);
    if (IsValid(WeatherSystemActor))
    {
        Rain->SetVisibility(false);
        return;
    }
    FRandomStream Random(580);
    for (int32 Index = 0; Index < 400; ++Index)
    {
        Drops.Emplace(FQuat::Identity,
            FVector(Random.FRandRange(-1000, 1000), Random.FRandRange(-1000, 1000), Random.FRandRange(50, 1250)),
            FVector(0.005, 0.005, 0.35));
    }
    Rain->AddInstances(Drops, false, false);
    UpdateRainDensity();
}

void ASimpleWeatherActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    BuildRain();
    SetWeather(Weather);
}

void ASimpleWeatherActor::BeginPlay()
{
    Super::BeginPlay();
    BuildRain(); // Restore transient animation data after loading a saved map.
    if (FParse::Param(FCommandLine::Get(), TEXT("IslandValidate")))
        GetWorld()->SpawnActor<AIslandValidationActor>();
    if (FParse::Param(FCommandLine::Get(), TEXT("SwimmingValidate")))
        GetWorld()->SpawnActor<ASwimmingValidationActor>();
    // Keep this correction in the weather actor; original project exposure
    // settings and source maps remain untouched.
    RuntimeExposure = NewObject<UPostProcessComponent>(this, TEXT("WeatherRuntimeExposure"));
    RuntimeExposure->bUnbound = true;
    RuntimeExposure->Priority = 2000.f;
    FPostProcessSettings& Exposure = RuntimeExposure->Settings;
    Exposure.bOverride_AutoExposureMethod = true;
    Exposure.AutoExposureMethod = AEM_Histogram;
    Exposure.bOverride_AutoExposureMinBrightness = true;
    Exposure.bOverride_AutoExposureMaxBrightness = true;
    Exposure.AutoExposureMinBrightness = 12.f;
    Exposure.AutoExposureMaxBrightness = 12.f;
    Exposure.bOverride_AutoExposureBias = true;
    Exposure.AutoExposureBias = 1.5f;
    RuntimeExposure->RegisterComponent();
    SetWeather(bStartClear ? ESimpleWeather::Clear : Weather);
    WindAudio->FadeIn(2.f, 0.22f);
    SurfAudio->FadeIn(2.f, 0.35f);
    UE_LOG(LogTemp, Display, TEXT("ISLAND_AMBIENCE_STARTED: wind=%s surf=%s"),
        *GetPathNameSafe(WindAudio->Sound), *GetPathNameSafe(SurfAudio->Sound));
    GetWorldTimerManager().SetTimer(LocomotionSetupTimer, this, &ASimpleWeatherActor::SetupPlayerLocomotion, 0.2f, true);
    if (bShowWeatherButton)
    {
        if (APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0))
        {
            if (Player->IsLocalController())
            {
                WeatherControls = CreateWidget<UWeatherControlWidget>(Player);
                if (WeatherControls)
                {
                    WeatherControls->SetWeatherActor(this);
                    // Mobile joysticks live in the global viewport overlay.
                    // A player-screen ZOrder cannot put UI above that layer.
                    WeatherControls->AddToViewport(1000);
                    UE_LOG(LogTemp, Display, TEXT("WEATHER_UI: controls added above mobile joystick overlay"));
                    FInputModeGameAndUI InputMode;
                    InputMode.SetHideCursorDuringCapture(false);
                    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                    Player->SetInputMode(InputMode);
                    Player->bEnableTouchEvents = true;
                    Player->bEnableClickEvents = true;
#if PLATFORM_DESKTOP
                    Player->bShowMouseCursor = true;
#endif
                }
            }
        }
    }
    if (bEnableTestKey)
    {
        if (APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0))
        {
            EnableInput(Player);
            FInputKeyBinding& Binding = InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ASimpleWeatherActor::ToggleWeather);
            Binding.bConsumeInput = false;
        }
    }
}

void ASimpleWeatherActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(LocomotionSetupTimer);
    if (WeatherControls)
    {
        WeatherControls->RemoveFromParent();
        WeatherControls = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void ASimpleWeatherActor::SetWeather(ESimpleWeather NewWeather)
{
    Weather = NewWeather;
    const bool bRaining = Weather == ESimpleWeather::Rain;
    Sun->SetIntensity(bRaining ? 6500.f : 10000.f);
    Sun->SetLightColor(bRaining ? FLinearColor(0.82f, 0.88f, 1.f) : FLinearColor(1.f, 0.95f, 0.85f));
    SkyLight->SetIntensity(bRaining ? 1.2f : 1.5f);
    Fog->SetFogDensity(bRaining ? 0.008f : 0.002f);
    UpdateRainDensity();
    if (WeatherControls) WeatherControls->RefreshLabel();
    UE_LOG(LogTemp, Display, TEXT("WEATHER_STATE: %s, rain amount %d%%"),
        bRaining ? TEXT("Rain") : TEXT("Clear"), FMath::RoundToInt(RainAmount * 100.f));
}

int32 ASimpleWeatherActor::GetActiveRainDropCount() const
{
    return FMath::Clamp(FMath::RoundToInt(400.f * RainAmount), 0, 400);
}

void ASimpleWeatherActor::SetRainAmount(float NewAmount)
{
    const float PreviousAmount = RainAmount;
    RainAmount = FMath::IsFinite(NewAmount) ? FMath::Clamp(NewAmount, 0.f, 1.f) : 0.f;
    UpdateRainDensity();
    if (WeatherControls) WeatherControls->RefreshLabel();
    if (!FMath::IsNearlyEqual(PreviousAmount, RainAmount))
    {
        UE_LOG(LogTemp, Display, TEXT("WEATHER_INPUT: rain slider %d%%"), FMath::RoundToInt(RainAmount * 100.f));
    }
}

void ASimpleWeatherActor::UpdateRainDensity()
{
    if (IsValid(WeatherSystemActor))
    {
        Rain->SetVisibility(false);
        SetActorTickEnabled(true); // Track the player even while clear.
        UpdateWeatherSystem();
        return;
    }
    const int32 ActiveCount = GetActiveRainDropCount();
    for (int32 Index = 0; Index < Drops.Num(); ++Index)
    {
        Drops[Index].SetScale3D(Index < ActiveCount ? FVector(0.005, 0.005, 0.35) : FVector::ZeroVector);
    }
    if (!Drops.IsEmpty()) Rain->BatchUpdateInstancesTransforms(0, Drops, false, true, true);
    const bool bAnimateRain = Weather == ESimpleWeather::Rain && ActiveCount > 0;
    Rain->SetVisibility(bAnimateRain);
    SetActorTickEnabled(bAnimateRain);
}

void ASimpleWeatherActor::UpdateWeatherSystem()
{
    const float Amount = Weather == ESimpleWeather::Rain ? RainAmount : 0.f;
    // Keep the Blueprint's delayed BeginPlay initialization in agreement with
    // the controls, including non-instance-editable authored state variables.
    if (FBoolProperty* On = FindFProperty<FBoolProperty>(WeatherSystemActor->GetClass(), TEXT("RainOn?")))
        On->SetPropertyValue_InContainer(WeatherSystemActor, Weather == ESimpleWeather::Rain);
    if (FNumericProperty* Humidity = FindFProperty<FNumericProperty>(WeatherSystemActor->GetClass(), TEXT("Humidity")))
        Humidity->SetFloatingPointPropertyValue(Humidity->ContainerPtrToValuePtr<void>(WeatherSystemActor), Amount);
    // Drive the asset's own Niagara and material-collection implementation.
    SetWeatherSystemScalar(WeatherSystemActor, TEXT("SetRainComponentState"), Amount > 0.f ? 1.f : 0.f);
    SetWeatherSystemScalar(WeatherSystemActor, TEXT("SetRain"), Amount);
    SetWeatherSystemScalar(WeatherSystemActor, TEXT("SetHumidity"), Amount);
    UE_LOG(LogTemp, Display, TEXT("WEATHER_SYSTEM_APPLIED: %s amount=%.2f"),
        *WeatherSystemActor->GetClass()->GetPathName(), Amount);
}

void ASimpleWeatherActor::ToggleWeather()
{
    if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
        if (const UDialogueManagerComponent* Dialogue = Player->FindComponentByClass<UDialogueManagerComponent>())
            if (Dialogue->IsConversationActive()) return;
    SetWeather(Weather == ESimpleWeather::Clear ? ESimpleWeather::Rain : ESimpleWeather::Clear);
}

void ASimpleWeatherActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsValid(WeatherSystemActor))
    {
        // Use the asset's own placement function, which follows the player's
        // root and applies its authored rain offset.
        if (UFunction* Follow = WeatherSystemActor->FindFunction(TEXT("SetRainLocation")))
            WeatherSystemActor->ProcessEvent(Follow, nullptr);
        return;
    }
    if (Weather != ESimpleWeather::Rain || Drops.IsEmpty()) return;
    // Move just the rain volume, leaving the map's sun/fog rig in place.
    if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
        Rain->SetWorldLocation(Player->GetActorLocation() - FVector(0.f, 0.f, 150.f));
    for (FTransform& Drop : Drops)
    {
        FVector Position = Drop.GetLocation();
        Position.Z = 50.f + FMath::Fmod(Position.Z - 50.f - 700.f * DeltaSeconds, 1200.f);
        if (Position.Z < 50.f) Position.Z += 1200.f;
        Drop.SetLocation(Position);
    }
    Rain->BatchUpdateInstancesTransforms(0, Drops, false, true, true);
}

void ASimpleWeatherActor::SetupPlayerLocomotion()
{
    ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!Player) return;
    if (!Player->FindComponentByClass<UFairySwimmingComponent>())
    {
        auto* Swimming = NewObject<UFairySwimmingComponent>(Player);
        Player->AddInstanceComponent(Swimming);
        Swimming->RegisterComponent();
    }
    if (!Player->FindComponentByClass<UJumoLocomotionComponent>())
    {
        UJumoLocomotionComponent* Playback = NewObject<UJumoLocomotionComponent>(Player);
        Player->AddInstanceComponent(Playback);
        Playback->RegisterComponent();
    }
    if (!Player->FindComponentByClass<UDialogueManagerComponent>())
    {
        UDialogueManagerComponent* Dialogue = NewObject<UDialogueManagerComponent>(Player);
        Player->AddInstanceComponent(Dialogue);
        Dialogue->RegisterComponent();
    }
    GetWorldTimerManager().ClearTimer(LocomotionSetupTimer);
}
