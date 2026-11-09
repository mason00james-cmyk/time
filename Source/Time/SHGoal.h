// Second Hand - the exit elevator. Step on it to win.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SHGoal.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class TIME_API ASHGoal : public AActor
{
	GENERATED_BODY()

public:
	ASHGoal();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	TObjectPtr<UStaticMeshComponent> Pad;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
