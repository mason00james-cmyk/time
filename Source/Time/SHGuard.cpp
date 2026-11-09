#include "SHGuard.h"
#include "SHBullet.h"
#include "SHRewindComponent.h"
#include "SHTimeSubsystem.h"
#include "AIController.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ASHGuard::ASHGuard()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 94.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -94.f), FRotator(0.f, -90.f, 0.f));

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = 400.f;
	Move->bOrientRotationToMovement = false;

	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gun"));
	GunMesh->SetupAttachment(GetMesh(), TEXT("hand_r"));
	GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Rewind = CreateDefaultSubobject<USHRewindComponent>(TEXT("Rewind"));
}

void ASHGuard::BeginPlay()
{
	Super::BeginPlay();

	MeshRelative = GetMesh()->GetRelativeTransform();
	GetMesh()->OnComponentHit.AddDynamic(this, &ASHGuard::OnMeshHit);

	if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this))
	{
		NextShotTime = TS->GameClock + 1.f + FMath::FRand();
	}

	switch (GuardType)
	{
	case ESHGuardType::Rusher:
		GetCharacterMovement()->MaxWalkSpeed = 650.f;
		GunMesh->SetVisibility(false);
		break;
	case ESHGuardType::Pistol:
		GetCharacterMovement()->MaxWalkSpeed = 350.f;
		break;
	case ESHGuardType::Shotgun:
		GetCharacterMovement()->MaxWalkSpeed = 300.f;
		break;
	}

	UpdateAnim(false);
}

void ASHGuard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Keep the gun in the hand but always pointing where the guard faces
	// (the model's barrel points along its -Y, hence the +90 yaw).
	if (bAlive && GunMesh->GetStaticMesh())
	{
		GunMesh->SetWorldRotation(GetActorRotation() + FRotator(0.f, 90.f, 0.f));
	}

	if (!bAlive || !bActive) return;

	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (!TS || TS->bRewinding || TS->bDead) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player) return;

	const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	const float Dist = ToPlayer.Size2D();
	const FVector Dir = FVector(ToPlayer.X, ToPlayer.Y, 0.f).GetSafeNormal();

	// Turn to face the player.
	const FRotator Want(0.f, Dir.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Want, DeltaSeconds, 10.f));

	const bool bSees = Dist < SightRange && CanSee(Player);

	float StopDist = 0.f;
	float Interval = 1.5f;
	switch (GuardType)
	{
	case ESHGuardType::Rusher:  StopDist = 0.f;   Interval = 0.f; break;
	case ESHGuardType::Pistol:  StopDist = 900.f; Interval = 1.5f; break;
	case ESHGuardType::Shotgun: StopDist = 600.f; Interval = 2.5f; break;
	}

	const bool bRun = bSees && Dist > StopDist;
	if (bRun) AddMovementInput(Dir);
	UpdateAnim(bRun);

	if (GuardType == ESHGuardType::Rusher)
	{
		if (Dist < 110.f && FMath::Abs(ToPlayer.Z) < 150.f) TS->PlayerDied();
		return;
	}

	// After a rewind the clock may be well before the scheduled shot; pull it back in.
	if (NextShotTime > TS->GameClock + Interval) NextShotTime = TS->GameClock + Interval * 0.5f;

	if (bSees && Dist < StopDist + 600.f && TS->GameClock >= NextShotTime)
	{
		NextShotTime = TS->GameClock + Interval;
		FireAt(Player->GetActorLocation() + FVector(0.f, 0.f, 30.f));
	}
}

void ASHGuard::FireAt(const FVector& Target)
{
	const FVector Muzzle = GunMesh->GetStaticMesh()
		? GunMesh->GetComponentLocation() + GetActorForwardVector() * 30.f
		: GetActorLocation() + GetActorForwardVector() * 50.f + FVector(0.f, 0.f, 40.f);

	const FVector Aim = (Target - Muzzle).GetSafeNormal();

	if (GuardType == ESHGuardType::Shotgun)
	{
		for (int32 i = 0; i < 5; ++i)
		{
			FRotator R = Aim.Rotation();
			R.Yaw += FMath::Lerp(-8.f, 8.f, i / 4.f);
			R.Pitch += FMath::FRandRange(-2.f, 2.f);
			ASHBullet::Fire(GetWorld(), BulletClass, Muzzle, R.Vector(), this, i == 0);
		}
	}
	else
	{
		ASHBullet::Fire(GetWorld(), BulletClass, Muzzle, Aim, this);
	}
}

bool ASHGuard::CanSee(const APawn* Player) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GuardSight), false, this);
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 60.f);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Eye, Player->GetActorLocation(), ECC_Visibility, Params);
	return !bHit || Hit.GetActor() == Player;
}

void ASHGuard::UpdateAnim(bool bRunning)
{
	const int8 Want = bRunning ? 1 : 0;
	if (Want == AnimState) return;
	AnimState = Want;

	UAnimSequence* Anim = bRunning ? RunAnim.Get() : IdleAnim.Get();
	if (Anim) GetMesh()->PlayAnimation(Anim, true);
}

void ASHGuard::KnockOut(FVector VelocityChange)
{
	if (!bAlive) return;
	bAlive = false;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	USkeletalMeshComponent* M = GetMesh();
	M->SetCollisionProfileName(TEXT("Ragdoll"));
	M->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	M->SetSimulatePhysics(true);
	M->SetNotifyRigidBodyCollision(true);
	M->WakeAllRigidBodies();
	M->AddImpulseToAllBodiesBelow(VelocityChange, TEXT("pelvis"), true, true);

	if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this)) TS->AddMeter(1.f);
}

void ASHGuard::Revive()
{
	USkeletalMeshComponent* M = GetMesh();
	M->SetSimulatePhysics(false);
	M->SetNotifyRigidBodyCollision(false);
	M->SetCollisionProfileName(TEXT("CharacterMesh"));
	M->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	M->SetRelativeTransform(MeshRelative);
	M->InitAnim(true);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	bAlive = true;
	AnimState = -1;
	UpdateAnim(false);
}

void ASHGuard::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// A flying knocked-out guard knocks out whoever he crashes into ("Human Bowling").
	if (bAlive) return;
	ASHGuard* Other = Cast<ASHGuard>(OtherActor);
	if (!Other || Other == this || !Other->IsAlive()) return;

	const FVector V = GetMesh()->GetPhysicsLinearVelocity(TEXT("pelvis"));
	if (V.Size() > 250.f) Other->KnockOut(V * 0.7f + FVector(0.f, 0.f, 150.f));
}
