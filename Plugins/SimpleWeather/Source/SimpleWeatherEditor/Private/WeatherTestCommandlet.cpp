#include "WeatherTestCommandlet.h"
#include "SimpleWeatherActor.h"
#include "Camera/CameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/PostProcessVolume.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/UnrealType.h"

UWeatherTestCommandlet::UWeatherTestCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UWeatherTestCommandlet::Main(const FString& Params)
{
    const FString MapPath(TEXT("/Game/Maps/LV_WeatherTest"));
    const bool bValidateOnly = FParse::Param(*Params, TEXT("ValidateOnly"));
    if (!bValidateOnly && FPackageName::DoesPackageExist(MapPath))
    {
        UE_LOG(LogTemp, Error, TEXT("LV_WeatherTest already exists. Refusing to overwrite."));
        return 1;
    }
    UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
    if (!World) return 2;
    // The original project disables auto exposure. Keep exposure local to this
    // map so daylight in lux does not clip to white and rain remains darker.
    APostProcessVolume* Exposure = World->SpawnActor<APostProcessVolume>();
    if (!Exposure) return 11;
    Exposure->SetActorLabel(TEXT("Weather Test Exposure"));
    Exposure->bUnbound = true;
    Exposure->Priority = 100.f;
    Exposure->Settings.bOverride_AutoExposureMethod = true;
    Exposure->Settings.AutoExposureMethod = AEM_Histogram;
    Exposure->Settings.bOverride_AutoExposureMinBrightness = true;
    Exposure->Settings.bOverride_AutoExposureMaxBrightness = true;
    Exposure->Settings.AutoExposureMinBrightness = 12.f;
    Exposure->Settings.AutoExposureMaxBrightness = 12.f;
    Exposure->Settings.bOverride_AutoExposureBias = true;
    Exposure->Settings.AutoExposureBias = 0.f;
    ASimpleWeatherActor* Weather = World->SpawnActor<ASimpleWeatherActor>();
    if (!Weather) return 3;
    Weather->SetActorLabel(TEXT("Weather - R toggles Clear / Rain"));
    Weather->bEnableTestKey = true;
    Weather->SetWeather(ESimpleWeather::Clear);
    if (Weather->Rain->IsVisible() || Weather->IsActorTickEnabled()) return 4;
    Weather->ToggleWeather();
    if (Weather->GetWeather() != ESimpleWeather::Rain || !Weather->Rain->IsVisible()
        || !Weather->IsActorTickEnabled() || Weather->Rain->GetInstanceCount() != 400) return 5;
    FTransform Before, After;
    Weather->Rain->GetInstanceTransform(0, Before);
    Weather->Tick(0.016f);
    Weather->Rain->GetInstanceTransform(0, After);
    if (Before.GetLocation().Equals(After.GetLocation())) return 6;
    for (int32 Index = 0; Index < 20; ++Index) Weather->ToggleWeather();
    if (Weather->Rain->GetInstanceCount() != 400) return 7;
    Weather->SetRainAmount(0.f);
    if (Weather->GetActiveRainDropCount() != 0 || Weather->Rain->IsVisible() || Weather->IsActorTickEnabled()) return 12;
    Weather->SetRainAmount(0.5f);
    if (Weather->GetActiveRainDropCount() != 200 || !Weather->Rain->IsVisible() || !Weather->IsActorTickEnabled()) return 13;
    Weather->SetRainAmount(2.f);
    if (Weather->GetRainAmount() != 1.f || Weather->GetActiveRainDropCount() != 400 || Weather->Rain->GetInstanceCount() != 400) return 14;
    Weather->SetWeather(ESimpleWeather::Clear);
    if (Weather->Rain->IsVisible() || Weather->IsActorTickEnabled() || Weather->Sun->Intensity != 10000.f) return 8;
    if (bValidateOnly)
    {
        UE_LOG(LogTemp, Display, TEXT("WEATHER_VALIDATION_PASS: clear/rain, movement, density 0/50/100 percent, clamp, stable instance count; no maps saved"));
        return 0;
    }

    AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -25), FRotator::ZeroRotator);
    Floor->SetActorLabel(TEXT("Weather Test Ground"));
    Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Floor->SetActorScale3D(FVector(24, 24, 0.5));
    World->SpawnActor<APlayerStart>(FVector(-750, 0, 150), FRotator(0, 0, 0));
    ACameraActor* Camera = World->SpawnActor<ACameraActor>(FVector(-1100, -800, 450), FRotator(-12, 36, 0));
    FByteProperty* AutoPlayer = FindFProperty<FByteProperty>(ACameraActor::StaticClass(), TEXT("AutoActivateForPlayer"));
    if (!AutoPlayer) return 10;
    AutoPlayer->SetPropertyValue_InContainer(Camera, EAutoReceiveInput::Player0);
    if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapPath)) return 9;
    UE_LOG(LogTemp, Display, TEXT("WEATHER_TEST_PASS: clear/rain, tick, animation, repeat toggles; saved %s"), *MapPath);
    return 0;
}
