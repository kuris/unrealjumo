#pragma once
#include "Commandlets/Commandlet.h"
#include "WeatherTestCommandlet.generated.h"

/** Explicitly creates only /Game/Maps/LV_WeatherTest; refuses to overwrite it. */
UCLASS()
class UWeatherTestCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UWeatherTestCommandlet();
    virtual int32 Main(const FString& Params) override;
};
