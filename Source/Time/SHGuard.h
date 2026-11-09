// Second Hand - a guard. Rushers chase you, Pistol/Shotgun guards keep distance and shoot.
// One punch (or one bullet) knocks them out into a ragdoll. Rewind brings them back.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SHRewindable.h"
#include "SHGuard.generated.h"

class ASHBullet;
class USHRewindComponent;
class UAnimSequence;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESHGuardType : uint8
{
	Rusher,
	Pistol,
	Shotgun
};

UCLASS()
class TIME_API ASHGuard : public ACharacter, public ISHRewindable
{
	GENERATED_BODY()

public:
	ASHGuard();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Guard")
	void KnockOut(FVector VelocityChange);

	UFUNCTION(BlueprintCallable, Category = "Guard")
	void Revive();

	UFUNCTION(BlueprintPure, Category = "Guard")
	bool IsAlive() const { return bAlive; }

	virtual bool IsRewindAlive() const override { return bAlive; }
	virtual void SetRewindAlive(bool bNewAlive) override { if (bNewAlive && !bAlive) Revive(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	ESHGuardType GuardType = ESHGuardType::Pistol;

	/** Turn off to keep a guard idle until a room activates it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	bool bActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	TSubclassOf<ASHBullet> BulletClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float SightRange = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Animation")
	TObjectPtr<UAnimSequence> IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Animation")
	TObjectPtr<UAnimSequence> RunAnim;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<UStaticMeshComponent> GunMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<USHRewindComponent> Rewind;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

private:
	void FireAt(const FVector& Target);
	bool CanSee(const APawn* Player) const;
	void UpdateAnim(bool bRunning);

	bool bAlive = true;
	float NextShotTime = 0.f;
	FTransform MeshRelative;
	int8 AnimState = -1;
};
