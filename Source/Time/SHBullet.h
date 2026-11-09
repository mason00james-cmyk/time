// Second Hand - slow, visible bullet. Moves itself (no projectile component) so rewind is simple.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SHRewindable.h"
#include "SHBullet.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USHRewindComponent;

UCLASS()
class TIME_API ASHBullet : public AActor, public ISHRewindable
{
	GENERATED_BODY()

public:
	ASHBullet();

	virtual void Tick(float DeltaSeconds) override;

	/** Spawn a bullet flying in Direction. Class may be null (uses ASHBullet). */
	static ASHBullet* Fire(UWorld* World, TSubclassOf<ASHBullet> Class, const FVector& Location, const FVector& Direction, AActor* Shooter, bool bPlaySound = true);

	virtual bool IsRewindAlive() const override { return bLive; }
	virtual void SetRewindAlive(bool bAlive) override;

	/** cm per second at full time speed (1500 = 15 m/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	float Speed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	float LifeSeconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	FLinearColor Color = FLinearColor(1.f, 0.35f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	float GuardKnockback = 1400.f;

	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<USHRewindComponent> Rewind;

	UPROPERTY()
	TObjectPtr<AActor> Shooter;

	FVector Velocity = FVector::ZeroVector;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bLive = true;
	float DeadClock = 0.f;
};
