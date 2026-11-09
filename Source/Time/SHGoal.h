// Second Hand - the exit elevator. Locked (red) until every listed guard is knocked out; then step on it to win.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SHGoal.generated.h"

class ASHGuard;
class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class TIME_API ASHGoal : public AActor
{
	GENERATED_BODY()

public:
	ASHGoal();
	virtual void Tick(float DeltaSeconds) override;

	/** Guards that must be knocked out before the exit opens. Empty = always open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal")
	TArray<TObjectPtr<ASHGuard>> RequiredGuards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	TObjectPtr<UStaticMeshComponent> Pad;

	int32 GuardsLeft() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void TryWin();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PadMID;

	int32 LastLeft = -1;
};
