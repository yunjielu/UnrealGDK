// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "StablyNamedActorDummy.generated.h"

UCLASS()
class AStablyNamedActorDummy : public AActor
{
	GENERATED_BODY()

public:
	virtual bool IsSupportedForNetworking() const;

	virtual bool IsFullNameStableForNetworking() const;
};
