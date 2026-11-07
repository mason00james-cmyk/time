// Second Hand - owns the game clock and player death.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SHTimeSubsystem.generated.h"

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

	void PlayerDied();

	/** Game-time clock. Advances with time dilation (slow when the world is slow). */
	float GameClock = 0.f;
	/** Real seconds played. */
	float RunTime = 0.f;

	bool bDead = false;

	static constexpr float FrozenDilation = 0.0001f; // Unreal's minimum: effectively frozen
};
