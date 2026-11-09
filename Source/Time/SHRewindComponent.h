// Second Hand - records an actor's state over game time and plays it back while rewinding.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SHRewindComponent.generated.h"

struct FSHSnapshot
{
	float Time = 0.f;
	FTransform Xform;
	FVector Velocity = FVector::ZeroVector;
	FRotator ControlRot = FRotator::ZeroRotator;
	bool bAlive = true;
};

UCLASS(ClassGroup = (SecondHand), meta = (BlueprintSpawnableComponent))
class TIME_API USHRewindComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USHRewindComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** For the player: also rewind where the camera was looking. */
	UPROPERTY(EditAnywhere, Category = "Rewind")
	bool bRestoreControlRotation = false;

	/** For bullets: destroy the actor if time is rewound to before it existed. */
	UPROPERTY(EditAnywhere, Category = "Rewind")
	bool bDestroyIfRewoundBeforeSpawn = false;

	float SpawnClock = 0.f;

protected:
	virtual void BeginPlay() override;

private:
	void Record(float Clock);
	void ApplyAt(float Clock);
	void EndRewind();

	TArray<FSHSnapshot> Snaps;
	bool bWasRewinding = false;

	static constexpr float SampleInterval = 1.f / 60.f; // game seconds between samples
	static constexpr float KeepSeconds = 4.5f;
};
