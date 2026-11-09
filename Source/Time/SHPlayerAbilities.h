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
	float PunchRange = 240.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float PunchRadius = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float PunchCooldownTime = 0.35f;

	/** If a guard is within this distance in front of you, clicking lunges you into him. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float LungeRange = 550.f;

	/** Lunge speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float LungeSpeed = 2600.f;

	/** How wide the "in front of you" cone is for lunging (0.5 = 60 degrees each side). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float LungeCone = 0.55f;

	/** Ragdoll launch speed (cm/s) forward and up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float PunchForward = 2400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	float PunchUp = 450.f;

	/** Show the cartoon fist in the corner of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Punch")
	bool bShowFist = true;

	/** Hide the template's first-person arms / body meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Camera")
	bool bHideTemplateMeshes = true;

protected:
	virtual void BeginPlay() override;

private:
	void BindInput(APlayerController* PC);
	void Punch();
	bool PunchSweep();
	void BuildFist(ACharacter* Char);
	void UpdateFist(float Real);
	void RewindPressed();
	void RewindReleased();
	void RestartLevel();

	float CurrentDilation = 1.f;
	float PunchCooldown = 0.f;
	float PunchMoveTimer = 0.f;
	float PunchActive = 0.f;   // real seconds left in which the punch can still connect
	float FistAnim = 1.f;      // 0 -> 1 over one punch

	UPROPERTY()
	TObjectPtr<USceneComponent> FistRoot;

	UPROPERTY()
	TObjectPtr<class UAudioComponent> RewindAudio;

	void UpdateRewindAudio(bool bRewinding);
	bool bInputBound = false;

	UPROPERTY()
	TObjectPtr<UInputComponent> SHInput;
};
