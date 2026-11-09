// Second Hand - owns the game clock, rewind state, the rewind meter and player death.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SHTimeSubsystem.generated.h"

class USoundBase;
class USoundAttenuation;

UCLASS()
class TIME_API USHTimeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static USHTimeSubsystem* Get(const UObject* WorldContext);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USHTimeSubsystem, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}

	void StartRewind();
	void StopRewind();
	void PlayerDied();
	void AddMeter(float Seconds);
	void Win();

	/** Play a sound from /Game/SecondHand/Audio by name. 3D sounds fade with distance and pitch down in slow-mo. */
	void PlaySfx(FName Name, const FVector& Location, float Volume = 1.f, bool b3D = true);
	USoundBase* GetSound(FName Name);

	/** Show an on-screen hint box for Seconds (real time). */
	void ShowHint(const FString& Text, float Seconds = 7.f);
	FString HintText;
	float HintTimeLeft = 0.f;
	float HintDuration = 1.f;

	/** Game-time clock. Advances with time dilation, runs backward while rewinding. */
	float GameClock = 0.f;
	/** Seconds of rewind available (max MaxRewind). */
	float RewindMeter = 4.f;
	/** Real seconds played (for the victory screen / time trial). */
	float RunTime = 0.f;

	bool bRewinding = false;
	bool bDead = false;
	bool bWon = false;
	float WinTime = 0.f;

	static constexpr float MaxRewind = 4.f;
	UPROPERTY()
	TMap<FName, TObjectPtr<USoundBase>> SoundCache;

	UPROPERTY()
	TObjectPtr<USoundAttenuation> Attenuation;

	static constexpr float FrozenDilation = 0.0001f;   // Unreal's minimum: effectively frozen
	static constexpr float MeterRegenPerSecond = 0.25f;

private:
	float RewindFloor = 0.f;
};
