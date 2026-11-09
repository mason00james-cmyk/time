#include "SHPlayerAbilities.h"
#include "SHGuard.h"
#include "SHHUD.h"
#include "SHRewindComponent.h"
#include "SHTimeSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
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

	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		if (bHideTemplateMeshes)
		{
			// The First Person template gives the player arms (and a body). We only want our own fist.
			TArray<USkeletalMeshComponent*> Skels;
			Char->GetComponents<USkeletalMeshComponent>(Skels);
			for (USkeletalMeshComponent* S : Skels)
			{
				S->SetHiddenInGame(true, true);
				S->SetCastShadow(false);
			}
		}
		if (bShowFist) BuildFist(Char);
	}
}

void USHPlayerAbilities::BuildFist(ACharacter* Char)
{
	UCameraComponent* Cam = Char->FindComponentByClass<UCameraComponent>();
	if (!Cam) return;

	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	FistRoot = NewObject<USceneComponent>(Char, TEXT("FistRoot"));
	FistRoot->SetupAttachment(Cam);
	FistRoot->RegisterComponent();

	auto Part = [&](const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor& Color)
	{
		UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(Char, Name);
		P->SetupAttachment(FistRoot);
		P->SetStaticMesh(Mesh);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->CastShadow = false;
		P->SetRelativeLocationAndRotation(Loc, Rot);
		P->SetRelativeScale3D(Scale);
		P->RegisterComponent();
		if (Mat)
		{
			P->SetMaterial(0, Mat);
			if (UMaterialInstanceDynamic* MID = P->CreateDynamicMaterialInstance(0)) MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	};

	// A chunky cartoon fist + sleeve, bottom right of the screen.
	Part(TEXT("Fist"), Sphere, FVector(0.f, 0.f, 0.f), FRotator::ZeroRotator, FVector(0.085f, 0.072f, 0.072f), FLinearColor(0.62f, 0.40f, 0.24f));
	Part(TEXT("Sleeve"), Cyl, FVector(-10.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(0.07f, 0.07f, 0.14f), FLinearColor(0.15f, 0.15f, 0.18f));
	UpdateFist(0.f);
}

void USHPlayerAbilities::UpdateFist(float Real)
{
	if (!FistRoot) return;
	FistAnim = FMath::Min(1.f, FistAnim + Real / 0.22f);
	// Out fast, back slower.
	const float T = FistAnim < 0.3f ? FistAnim / 0.3f : 1.f - (FistAnim - 0.3f) / 0.7f;
	const float E = FMath::Clamp(T, 0.f, 1.f);
	const FVector Rest(48.f, 24.f, -26.f);
	const FVector Out(82.f, 7.f, -12.f);
	FistRoot->SetRelativeLocation(FMath::Lerp(Rest, Out, E));
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
	UpdateFist(Real);
	if (PunchActive > 0.f)
	{
		PunchActive -= Real;
		if (PunchSweep()) PunchActive = 0.f;
	}

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
	PunchActive = 0.22f;    // the punch keeps checking for a hit while you lunge
	FistAnim = 0.f;

	ACharacter* Char = Cast<ACharacter>(GetOwner());
	AController* Ctrl = Char ? Char->GetController() : nullptr;
	if (!Ctrl) return;

	FVector EyeLoc;
	FRotator EyeRot;
	Ctrl->GetPlayerViewPoint(EyeLoc, EyeRot);
	const FVector Fwd = FVector(EyeRot.Vector().X, EyeRot.Vector().Y, 0.f).GetSafeNormal();

	// Lunge at the closest living guard roughly in front of you.
	ASHGuard* Best = nullptr;
	float BestDist = LungeRange;
	for (TActorIterator<ASHGuard> It(GetWorld()); It; ++It)
	{
		ASHGuard* G = *It;
		if (!G->IsAlive()) continue;
		FVector To = G->GetActorLocation() - Char->GetActorLocation();
		To.Z = 0.f;
		const float D = To.Size();
		if (D > BestDist || D < 1.f) continue;
		if (FVector::DotProduct(To / D, Fwd) < LungeCone) continue;
		FHitResult Block;
		FCollisionQueryParams P(SCENE_QUERY_STAT(Lunge), false, Char);
		P.AddIgnoredActor(G);
		if (GetWorld()->LineTraceSingleByChannel(Block, EyeLoc, G->GetActorLocation(), ECC_Visibility, P)) continue;
		Best = G;
		BestDist = D;
	}
	if (Best)
	{
		FVector Dir = Best->GetActorLocation() - Char->GetActorLocation();
		Dir.Z = 0.f;
		Char->LaunchCharacter(Dir.GetSafeNormal() * LungeSpeed + FVector(0.f, 0.f, 50.f), true, true);
	}

	if (PunchSweep()) PunchActive = 0.f;
}

bool USHPlayerAbilities::PunchSweep()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Ctrl = Pawn ? Pawn->GetController() : nullptr;
	if (!Ctrl) return false;

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
			return true;
		}
	}
	return false;
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
