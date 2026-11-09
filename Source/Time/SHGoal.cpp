#include "SHGoal.h"
#include "SHTimeSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ASHGoal::ASHGoal()
{
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetBoxExtent(FVector(150.f, 150.f, 150.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = Trigger;

	Pad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pad"));
	Pad->SetupAttachment(Trigger);
	Pad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pad->SetRelativeLocation(FVector(0.f, 0.f, -145.f));
	Pad->SetRelativeScale3D(FVector(3.f, 3.f, 0.1f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Pad->SetStaticMesh(Cube.Object);
}

void ASHGoal::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ASHGoal::OnOverlap);
}

void ASHGoal::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this)) TS->Win();
	}
}
