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

	if (bDead)
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FrozenDilation);
		return;
	}

	RunTime += Real;
	GameClock += Real * World->GetWorldSettings()->GetEffectiveTimeDilation();
}

void USHTimeSubsystem::PlayerDied()
{
	if (bDead) return;
	bDead = true;
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FrozenDilation);
	}
}
