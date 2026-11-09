#include "SHMuzzleFlash.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ASHMuzzleFlash::ASHMuzzleFlash()
{
	PrimaryActorTick.bCanEverTick = true;

	Burst = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Burst"));
	RootComponent = Burst;
	Burst->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Burst->CastShadow = false;
	Burst->SetRelativeScale3D(FVector(0.35f, 0.2f, 0.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) Burst->SetStaticMesh(Sphere.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Mat.Succeeded()) Burst->SetMaterial(0, Mat.Object);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Burst);
	Light->SetIntensityUnits(ELightUnits::Lumens);
	Light->SetIntensity(6000.f);
	Light->SetAttenuationRadius(450.f);
	Light->SetLightColor(FLinearColor(1.f, 0.7f, 0.3f));
	Light->SetCastShadows(false);
}

void ASHMuzzleFlash::Spawn(UWorld* World, const FVector& Location, const FRotator& Facing)
{
	if (!World) return;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ASHMuzzleFlash* F = World->SpawnActor<ASHMuzzleFlash>(ASHMuzzleFlash::StaticClass(), Location, Facing, P))
	{
		if (UMaterialInstanceDynamic* MID = F->Burst->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.85f, 0.4f));
		}
	}
}

void ASHMuzzleFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds; // game time: slow-mo stretches the flash
	const float A = FMath::Clamp(1.f - Age / Duration, 0.f, 1.f);
	Light->SetIntensity(6000.f * A);
	SetActorScale3D(FVector(0.35f, 0.2f, 0.2f) * (0.4f + A));
	if (A <= 0.f) Destroy();
}
