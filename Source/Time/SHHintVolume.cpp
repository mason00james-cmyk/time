#include "SHHintVolume.h"
#include "SHTimeSubsystem.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

ASHHintVolume::ASHHintVolume()
{
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetBoxExtent(FVector(150.f, 300.f, 250.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->bHiddenInGame = true;
	RootComponent = Trigger;
}

void ASHHintVolume::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ASHHintVolume::OnOverlap);
}

void ASHHintVolume::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bShown || !OtherActor || OtherActor != UGameplayStatics::GetPlayerPawn(this, 0)) return;
	if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this))
	{
		if (TS->bRewinding) return;
		bShown = true;
		TS->ShowHint(Message, Seconds);
	}
}
