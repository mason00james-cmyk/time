#include "SHBullet.h"
#include "SHTimeSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ASHBullet::ASHBullet()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(6.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.3f, 0.1f, 0.1f)); // stretched along flight direction
	Mesh->CastShadow = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded()) Mesh->SetStaticMesh(SphereMesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMat.Succeeded()) Mesh->SetMaterial(0, ShapeMat.Object);
}

void ASHBullet::BeginPlay()
{
	Super::BeginPlay();
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ASHBullet::OnOverlap);
	if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}
	if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this)) SpawnClock = TS->GameClock;
}

ASHBullet* ASHBullet::Fire(UWorld* World, TSubclassOf<ASHBullet> Class, const FVector& Location, const FVector& Direction, AActor* InShooter)
{
	if (!World) return nullptr;
	UClass* SpawnClass = Class ? Class.Get() : ASHBullet::StaticClass();
	const FTransform SpawnXform(Direction.Rotation(), Location);

	ASHBullet* Bullet = World->SpawnActorDeferred<ASHBullet>(SpawnClass, SpawnXform, InShooter, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Bullet) return nullptr;

	Bullet->Shooter = InShooter;
	Bullet->Velocity = Direction.GetSafeNormal() * Bullet->Speed;
	Bullet->FinishSpawning(SpawnXform);
	return Bullet;
}

void ASHBullet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (!TS || TS->bDead) return;

	if (TS->GameClock - SpawnClock > LifeSeconds)
	{
		Destroy();
		return;
	}

	// DeltaSeconds is already slowed down by time dilation, so bullets crawl in slow-mo.
	FHitResult Hit;
	AddActorWorldOffset(Velocity * DeltaSeconds, true, &Hit);
	if (Hit.bBlockingHit) Destroy();
}

void ASHBullet::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == Shooter) return;
	if (Cast<ASHBullet>(OtherActor)) return;

	if (OtherActor == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this)) TS->PlayerDied();
		Destroy();
	}
}
