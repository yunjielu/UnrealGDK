// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/World.h"

#include "MapSettings.generated.h"

// Schema data related to a default Subobject owned by a specific Actor class.
USTRUCT()
struct FMapTestSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = "SpatialGDK", EditAnywhere)
	int32 NumClients = 1;

	UPROPERTY(Category = "SpatialGDK", EditAnywhere)
	int32 NumServers = 1;

	UPROPERTY(Category = "SpatialGDK", EditAnywhere)
	FString LaunchConfig;
};

/**
* Stores the settings for the maps in the project, mainly for the purposes of automated testing.
*/
UCLASS()
class SPATIALGDK_API UMapSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(Category = "SpatialGDK", EditAnywhere, meta = (AllowedClasses = "World"))
	TMap<FSoftObjectPath, FMapTestSettings> SettingsPerMap;
};

