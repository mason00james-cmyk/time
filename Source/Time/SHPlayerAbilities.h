// Second Hand - add this component to BP_FirstPersonCharacter.
// Stage 2: time flow + death freeze, Enter to restart, HUD.
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
	/** World speed while standing still (0.05 = 5%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Second Hand|Time")
	float SlowDilation = 0.05f;

	/** How fast time snaps between slow and full speed. Higher = snappier. */
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

protected:
	virtual void BeginPlay() override;

private:
	void BindInput(APlayerController* PC);
	void RestartLevel();

	float CurrentDilation = 1.f;
	bool bInputBound = false;

	UPROPERTY()
	TObjectPtr<UInputComponent> SHInput;
};
