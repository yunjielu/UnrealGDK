// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SingletonActorDummy.generated.h"

UCLASS(SpatialType = Singleton)
class ASingletonActorDummy : public AActor
{
	GENERATED_BODY()

public:
	virtual bool IsSupportedForNetworking() const;

	virtual bool IsFullNameStableForNetworking() const;
};
