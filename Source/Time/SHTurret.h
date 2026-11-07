// Second Hand - a wall turret that fires bullets on a timer. Used for room 1 and the Bullet Gauntlet.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SHTurret.generated.h"

class ASHBullet;
class UArrowComponent;
class UStaticMeshComponent;

UCLASS()
class TIME_API ASHTurret : public AActor
{
	GENERATED_BODY()

public:
	ASHTurret();
	virtual void Tick(float DeltaSeconds) override;

	/** Game seconds between shots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	float FireInterval = 1.f;

	/** Seconds before the first shot (stagger several turrets). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	float StartDelay = 0.f;

	/** Off = fire straight along the arrow. On = aim at the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	bool bAimAtPlayer = false;

	/** Random yaw spread in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	float Spread = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	TSubclassOf<ASHBullet> BulletClass;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UArrowComponent> Muzzle;

protected:
	virtual void BeginPlay() override;

private:
	float NextShotTime = 0.f;
};
