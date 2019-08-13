// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "StablyNamedActorDummy.h"

bool AStablyNamedActorDummy::IsSupportedForNetworking() const
{
	return true;
}

bool AStablyNamedActorDummy::IsFullNameStableForNetworking() const
{
	return true;
}
