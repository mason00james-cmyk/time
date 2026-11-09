#include "SHPlayerAbilities.h"
#include "SHGuard.h"
#include "SHHUD.h"
#include "SHRewindComponent.h"
#include "SHTimeSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"

USHPlayerAbilities::USHPlayerAbilities()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void USHPlayerAbilities::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		UCharacterMovementComponent* Move = Char->GetCharacterMovement();
		Move->MaxWalkSpeed = WalkSpeed;
		Move->MaxAcceleration = Acceleration;
		Move->BrakingDecelerationWalking = Acceleration;
		Move->GroundFriction = 12.f;
	}

	// The player records itself too, so rewind moves you back with everything else.
	USHRewindComponent* PlayerRewind = NewObject<USHRewindComponent>(GetOwner(), TEXT("PlayerRewind"));
	PlayerRewind->bRestoreControlRotation = true;
	PlayerRewind->RegisterComponent();
}

void USHPlayerAbilities::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char) return;

	if (!bInputBound)
	{
		if (APlayerController* PC = Cast<APlayerController>(Char->GetController())) BindInput(PC);
	}

	const float Real = FApp::GetDeltaTime(); // undilated
	PunchCooldown -= Real;
	PunchMoveTimer -= Real;

	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (!TS) return;

	if (TS->bRewinding || TS->bDead || TS->bWon)
	{
		// The subsystem freezes the world; freeze the player with it.
		Char->CustomTimeDilation = 1.f;
		CurrentDilation = SlowDilation;
		return;
	}

	// Moving = holding a movement key, in the air, or mid-punch. Input (acceleration), not velocity,
	// so time snaps back the instant a key is pressed.
	UCharacterMovementComponent* Move = Char->GetCharacterMovement();
	const bool bMoving = Move->GetCurrentAcceleration().SizeSquared() > 1.f || Move->IsFalling() || PunchMoveTimer > 0.f;

	CurrentDilation = FMath::FInterpTo(CurrentDilation, bMoving ? 1.f : SlowDilation, Real, InterpSpeed);
	UGameplayStatics::SetGlobalTimeDilation(this, CurrentDilation);
	Char->CustomTimeDilation = 1.f / CurrentDilation; // player always runs at real speed

	if (UCameraComponent* Cam = Char->FindComponentByClass<UCameraComponent>())
	{
		const float Alpha = FMath::Clamp((CurrentDilation - SlowDilation) / (1.f - SlowDilation), 0.f, 1.f);
		Cam->SetFieldOfView(FMath::Lerp(SlowFOV, FastFOV, Alpha));
	}
}

void USHPlayerAbilities::BindInput(APlayerController* PC)
{
	SHInput = NewObject<UInputComponent>(GetOwner(), UInputComponent::StaticClass(), TEXT("SHInput"));
	SHInput->RegisterComponent();
	SHInput->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &USHPlayerAbilities::Punch);
	SHInput->BindKey(EKeys::R, IE_Pressed, this, &USHPlayerAbilities::RewindPressed);
	SHInput->BindKey(EKeys::R, IE_Released, this, &USHPlayerAbilities::RewindReleased);
	SHInput->BindKey(EKeys::Enter, IE_Pressed, this, &USHPlayerAbilities::RestartLevel);
	PC->PushInputComponent(SHInput);

	PC->ClientSetHUD(ASHHUD::StaticClass());
	bInputBound = true;
}

void USHPlayerAbilities::Punch()
{
	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (!TS || TS->bDead || TS->bWon || TS->bRewinding || PunchCooldown > 0.f) return;

	PunchCooldown = PunchCooldownTime;
	PunchMoveTimer = 0.25f; // punching moves time

	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Ctrl = Pawn ? Pawn->GetController() : nullptr;
	if (!Ctrl) return;

	FVector EyeLoc;
	FRotator EyeRot;
	Ctrl->GetPlayerViewPoint(EyeLoc, EyeRot);
	const FVector Fwd = EyeRot.Vector();

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Punch), false, Pawn);
	GetWorld()->SweepMultiByObjectType(Hits, EyeLoc, EyeLoc + Fwd * PunchRange, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(PunchRadius), Params);

	for (const FHitResult& Hit : Hits)
	{
		ASHGuard* Guard = Cast<ASHGuard>(Hit.GetActor());
		if (Guard && Guard->IsAlive())
		{
			const FVector Flat = FVector(Fwd.X, Fwd.Y, 0.f).GetSafeNormal();
			Guard->KnockOut(Flat * PunchForward + FVector(0.f, 0.f, PunchUp));
			break;
		}
	}
}

void USHPlayerAbilities::RewindPressed()
{
	if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this)) TS->StartRewind();
}

void USHPlayerAbilities::RewindReleased()
{
	if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this)) TS->StopRewind();
}

void USHPlayerAbilities::RestartLevel()
{
	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (TS && (TS->bDead || TS->bWon))
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
	}
}
