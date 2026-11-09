// Second Hand - a quick flash of light at a gun's muzzle. Lives in game time, so it lingers in slow-mo.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SHMuzzleFlash.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class TIME_API ASHMuzzleFlash : public AActor
{
	GENERATED_BODY()

public:
	ASHMuzzleFlash();
	virtual void Tick(float DeltaSeconds) override;

	static void Spawn(UWorld* World, const FVector& Location, const FRotator& Facing);

	/** Game seconds the flash lasts. */
	UPROPERTY(EditAnywhere, Category = "Flash")
	float Duration = 0.08f;

	UPROPERTY(VisibleAnywhere, Category = "Flash")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(VisibleAnywhere, Category = "Flash")
	TObjectPtr<UStaticMeshComponent> Burst;

private:
	float Age = 0.f;
};
