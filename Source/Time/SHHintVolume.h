// Second Hand - walk into it and a hint box pops up on screen (once).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SHHintVolume.generated.h"

class UBoxComponent;

UCLASS()
class TIME_API ASHHintVolume : public AActor
{
	GENERATED_BODY()

public:
	ASHHintVolume();

	/** Use | to start a new line. The first line is drawn as the title. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	FString Message = TEXT("ROUND|Do the thing.");

	/** Real seconds the hint stays on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	float Seconds = 7.f;

	UPROPERTY(VisibleAnywhere, Category = "Hint")
	TObjectPtr<UBoxComponent> Trigger;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bShown = false;
};
