#include "SHPlayerAbilities.h"
#include "SHHUD.h"
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

	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (TS && TS->bDead)
	{
		// The subsystem freezes the world on death; freeze the player with it.
		Char->CustomTimeDilation = 1.f;
		CurrentDilation = SlowDilation;
		return;
	}

	const float Real = FApp::GetDeltaTime(); // real seconds, not slowed down

	// Moving = holding a movement key or in the air. We check input (acceleration), not velocity,
	// so time snaps back the instant a key is pressed.
	UCharacterMovementComponent* Move = Char->GetCharacterMovement();
	const bool bMoving = Move->GetCurrentAcceleration().SizeSquared() > 1.f || Move->IsFalling();

	CurrentDilation = FMath::FInterpTo(CurrentDilation, bMoving ? 1.f : SlowDilation, Real, InterpSpeed);
	UGameplayStatics::SetGlobalTimeDilation(this, CurrentDilation);
	Char->CustomTimeDilation = 1.f / CurrentDilation; // keep the player itself at real speed

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
	SHInput->BindKey(EKeys::Enter, IE_Pressed, this, &USHPlayerAbilities::RestartLevel);
	PC->PushInputComponent(SHInput);

	PC->ClientSetHUD(ASHHUD::StaticClass());
	bInputBound = true;
}

void USHPlayerAbilities::RestartLevel()
{
	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (TS && TS->bDead)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
	}
}
