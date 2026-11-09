// Second Hand - simple canvas HUD: crosshair, rewind meter, slow-mo tint, death and rewind screens.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SHHUD.generated.h"

UCLASS()
class TIME_API ASHHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
