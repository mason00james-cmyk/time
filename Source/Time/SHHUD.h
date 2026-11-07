// Second Hand - simple canvas HUD: crosshair, slow-mo tint, death screen.
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
