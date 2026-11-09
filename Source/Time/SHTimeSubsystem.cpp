#include "SHTimeSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

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
	PlaySfx(TEXT("SFX_Death"), FVector::ZeroVector, 0.9f, false);
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
	PlaySfx(TEXT("SFX_Win"), FVector::ZeroVector, 0.9f, false);
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

USoundBase* USHTimeSubsystem::GetSound(FName Name)
{
	if (TObjectPtr<USoundBase>* Found = SoundCache.Find(Name)) return Found->Get();
	const FString Path = FString::Printf(TEXT("/Game/SecondHand/Audio/%s.%s"), *Name.ToString(), *Name.ToString());
	USoundBase* S = LoadObject<USoundBase>(nullptr, *Path);
	SoundCache.Add(Name, S);
	return S;
}

void USHTimeSubsystem::PlaySfx(FName Name, const FVector& Location, float Volume, bool b3D)
{
	USoundBase* S = GetSound(Name);
	UWorld* World = GetWorld();
	if (!S || !World) return;

	if (!b3D)
	{
		UGameplayStatics::PlaySound2D(World, S, Volume);
		return;
	}

	if (!Attenuation)
	{
		Attenuation = NewObject<USoundAttenuation>(this);
		Attenuation->Attenuation.bAttenuate = true;
		Attenuation->Attenuation.bSpatialize = true;
		Attenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		Attenuation->Attenuation.AttenuationShapeExtents = FVector(400.f, 0.f, 0.f);
		Attenuation->Attenuation.FalloffDistance = 2600.f;
	}

	// Slow-mo pitches sounds down (0.55x at a crawl, 1x at full speed).
	const float Dilation = World->GetWorldSettings()->GetEffectiveTimeDilation();
	const float Pitch = FMath::Lerp(0.55f, 1.f, FMath::Clamp(Dilation, 0.f, 1.f)) * FMath::FRandRange(0.95f, 1.05f);
	UGameplayStatics::PlaySoundAtLocation(World, S, Location, Volume, Pitch, 0.f, Attenuation);
}
