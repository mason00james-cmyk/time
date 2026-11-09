#include "SHTimeSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"

USHTimeSubsystem* USHTimeSubsystem::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<USHTimeSubsystem>() : nullptr;
}

void USHTimeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World) return;

	const float Real = FApp::GetDeltaTime(); // undilated
	HintTimeLeft = FMath::Max(0.f, HintTimeLeft - Real);

	if (bWon)
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FrozenDilation);
		return;
	}

	if (!bDead) RunTime += Real;

	if (bRewinding)
	{
		const float Step = FMath::Min(Real, RewindMeter);
		GameClock = FMath::Max(RewindFloor, GameClock - Step);
		RewindMeter -= Step;
		UGameplayStatics::SetGlobalTimeDilation(World, FrozenDilation);
		if (RewindMeter <= 0.f || GameClock <= RewindFloor)
		{
			StopRewind();
		}
		return;
	}

	if (bDead)
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FrozenDilation);
		return;
	}

	const float Dilation = World->GetWorldSettings()->GetEffectiveTimeDilation();
	GameClock += Real * Dilation;
	RewindMeter = FMath::Min(MaxRewind, RewindMeter + MeterRegenPerSecond * Real);
}

void USHTimeSubsystem::StartRewind()
{
	if (bWon || bRewinding || RewindMeter < 0.1f) return;
	bRewinding = true;
	bDead = false;
	RewindFloor = FMath::Max(0.f, GameClock - MaxRewind);
}

void USHTimeSubsystem::StopRewind()
{
	bRewinding = false;
}

void USHTimeSubsystem::PlayerDied()
{
	if (bWon || bDead || bRewinding) return;
	bDead = true;
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FrozenDilation);
	}
}

void USHTimeSubsystem::AddMeter(float Seconds)
{
	RewindMeter = FMath::Clamp(RewindMeter + Seconds, 0.f, MaxRewind);
}

void USHTimeSubsystem::Win()
{
	if (bWon || bDead || bRewinding) return;
	bWon = true;
	WinTime = RunTime;
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FrozenDilation);
	}
}

void USHTimeSubsystem::ShowHint(const FString& Text, float Seconds)
{
	HintText = Text;
	HintDuration = FMath::Max(Seconds, 0.5f);
	HintTimeLeft = HintDuration;
}
