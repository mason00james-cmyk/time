#include "SHRewindComponent.h"
#include "SHTimeSubsystem.h"
#include "SHRewindable.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

USHRewindComponent::USHRewindComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void USHRewindComponent::BeginPlay()
{
	Super::BeginPlay();
	if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this))
	{
		SpawnClock = TS->GameClock;
		Record(TS->GameClock);
	}
}

void USHRewindComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (!TS) return;

	if (TS->bRewinding)
	{
		bWasRewinding = true;
		ApplyAt(TS->GameClock);
		return;
	}

	if (bWasRewinding)
	{
		bWasRewinding = false;
		EndRewind();
	}

	if (TS->bDead) return;
	Record(TS->GameClock);
}

void USHRewindComponent::Record(float Clock)
{
	if (Snaps.Num() > 0 && Clock - Snaps.Last().Time < SampleInterval) return;

	AActor* Owner = GetOwner();
	FSHSnapshot S;
	S.Time = Clock;
	S.Xform = Owner->GetActorTransform();

	if (ACharacter* Char = Cast<ACharacter>(Owner))
	{
		S.Velocity = Char->GetCharacterMovement()->Velocity;
		if (AController* Ctrl = Char->GetController())
		{
			S.ControlRot = Ctrl->GetControlRotation();
		}
	}
	if (ISHRewindable* R = Cast<ISHRewindable>(Owner))
	{
		S.bAlive = R->IsRewindAlive();
	}
	Snaps.Add(S);

	// Drop samples older than the rewind window (always keep at least one).
	int32 Drop = 0;
	while (Drop < Snaps.Num() - 1 && Snaps[Drop].Time < Clock - KeepSeconds) ++Drop;
	if (Drop > 0) Snaps.RemoveAt(0, Drop, EAllowShrinking::No);
}

void USHRewindComponent::ApplyAt(float Clock)
{
	AActor* Owner = GetOwner();

	if (bDestroyIfRewoundBeforeSpawn && Clock < SpawnClock)
	{
		Owner->Destroy();
		return;
	}

	while (Snaps.Num() > 1 && Snaps.Last().Time > Clock) Snaps.Pop(EAllowShrinking::No);
	if (Snaps.Num() == 0) return;

	const FSHSnapshot& S = Snaps.Last();

	if (ISHRewindable* R = Cast<ISHRewindable>(Owner))
	{
		if (R->IsRewindAlive() != S.bAlive) R->SetRewindAlive(S.bAlive);
	}

	if (S.bAlive)
	{
		Owner->SetActorTransform(S.Xform, false, nullptr, ETeleportType::TeleportPhysics);
		if (bRestoreControlRotation)
		{
			if (APawn* Pawn = Cast<APawn>(Owner))
			{
				if (AController* Ctrl = Pawn->GetController()) Ctrl->SetControlRotation(S.ControlRot);
			}
		}
	}
}

void USHRewindComponent::EndRewind()
{
	if (Snaps.Num() == 0) return;
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		Char->GetCharacterMovement()->Velocity = Snaps.Last().Velocity;
	}
}
