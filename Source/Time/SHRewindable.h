// Second Hand - anything that can be "dead" and come back when time rewinds.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SHRewindable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class USHRewindable : public UInterface
{
	GENERATED_BODY()
};

class TIME_API ISHRewindable
{
	GENERATED_BODY()

public:
	virtual bool IsRewindAlive() const { return true; }
	virtual void SetRewindAlive(bool bAlive) {}
};
