#include "SHTurret.h"
#include "SHBullet.h"
#include "SHTimeSubsystem.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ASHTurret::ASHTurret()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->SetRelativeScale3D(FVector(0.4f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Body->SetStaticMesh(Cube.Object);

	Muzzle = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Body);
	Muzzle->SetRelativeLocation(FVector(80.f, 0.f, 0.f)); // in front of the (scaled) cube
	Muzzle->SetUsingAbsoluteScale(true);
}

void ASHTurret::BeginPlay()
{
	Super::BeginPlay();
	if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this))
	{
		NextShotTime = TS->GameClock + StartDelay;
	}
}

void ASHTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (!TS || TS->bDead) return;
	if (TS->GameClock < NextShotTime) return;
	NextShotTime = TS->GameClock + FireInterval;

	const FVector From = Muzzle->GetComponentLocation();
	FVector Dir = Muzzle->GetForwardVector();
	if (bAimAtPlayer)
	{
		if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			Dir = (Player->GetActorLocation() + FVector(0.f, 0.f, 30.f) - From).GetSafeNormal();
		}
	}
	if (Spread > 0.f)
	{
		FRotator R = Dir.Rotation();
		R.Yaw += FMath::FRandRange(-Spread, Spread);
		Dir = R.Vector();
	}

	ASHBullet::Fire(GetWorld(), BulletClass, From, Dir, this);
}
