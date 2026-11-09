#include "SHGoal.h"
#include "SHGuard.h"
#include "SHTimeSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ASHGoal::ASHGoal()
{
	PrimaryActorTick.bCanEverTick = true;

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
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Mat.Succeeded()) Pad->SetMaterial(0, Mat.Object);
}

void ASHGoal::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ASHGoal::OnOverlap);
	PadMID = Pad->CreateDynamicMaterialInstance(0);
}

int32 ASHGoal::GuardsLeft() const
{
	int32 N = 0;
	for (const ASHGuard* G : RequiredGuards)
	{
		if (G && G->IsAlive()) ++N;
	}
	return N;
}

void ASHGoal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const int32 Left = GuardsLeft();
	if (Left != LastLeft)
	{
		if (PadMID)
		{
			PadMID->SetVectorParameterValue(TEXT("Color"), Left > 0 ? FLinearColor(0.9f, 0.08f, 0.06f) : FLinearColor(0.1f, 1.f, 0.35f));
		}
		if (LastLeft > 0 && Left == 0)
		{
			if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this)) TS->ShowHint(TEXT("ELEVATOR UNLOCKED|Get to the green pad!"), 4.f);
		}
		LastLeft = Left;
	}

	// If you're already standing on the pad when the last guard drops, you win right away.
	if (Left == 0)
	{
		if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (Trigger->IsOverlappingActor(P)) TryWin();
		}
	}
}

void ASHGoal::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor != UGameplayStatics::GetPlayerPawn(this, 0)) return;

	const int32 Left = GuardsLeft();
	if (Left > 0)
	{
		if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this))
		{
			TS->ShowHint(FString::Printf(TEXT("ELEVATOR LOCKED|Knock out every guard first (%d left)"), Left), 3.f);
		}
		return;
	}
	TryWin();
}

void ASHGoal::TryWin()
{
	if (USHTimeSubsystem* TS = USHTimeSubsystem::Get(this)) TS->Win();
}
