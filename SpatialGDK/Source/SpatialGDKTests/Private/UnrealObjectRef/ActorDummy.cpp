// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "ActorDummy.h"

bool AActorDummy::IsSupportedForNetworking() const
{
	return true;
}

bool AActorDummy::IsFullNameStableForNetworking() const
{
	return false;
}
