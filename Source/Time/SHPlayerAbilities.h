// Second Hand - add this ONE component to BP_FirstPersonCharacter.
// Handles: time flow (slow-mo when still), fast movement, FOV kick, punching, rewind input,
// restart, the player's own rewind recording, and installs the HUD.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SHPlayerAbilities.generated.h"

class UInputComponent;

UCLASS(ClassGroup = (SecondHand), meta = (BlueprintSpawnableComponent))
class TIME_API USHPlayerAbilities : public UActorComponent
{
	GENERATED_BODY()

public:
	USHPlayerAbilities();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Second Hand")
	float GetCurrentDilation() const { return CurrentDilation; }

	// ---- Time ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Time")
	float SlowDilation = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Time")
	float InterpSpeed = 20.f;

	// ---- Movement ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Movement")
	float WalkSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Movement")
	float Acceleration = 6000.f;

	// ---- Camera ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Camera")
	float SlowFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Camera")
	float FastFOV = 100.f;

	// ---- Punch ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float PunchRange = 170.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float PunchRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float PunchCooldownTime = 0.4f;

	/** Ragdoll launch speed (cm/s) forward and up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float PunchForward = 1100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float PunchUp = 300.f;

protected:
	virtual void BeginPlay() override;

private:
	void BindInput(APlayerController* PC);
	void Punch();
	void RewindPressed();
	void RewindReleased();
	void RestartLevel();

	float CurrentDilation = 1.f;
	float PunchCooldown = 0.f;
	float PunchMoveTimer = 0.f;
	bool bInputBound = false;

	UPROPERTY()
	TObjectPtr<UInputComponent> SHInput;
};
