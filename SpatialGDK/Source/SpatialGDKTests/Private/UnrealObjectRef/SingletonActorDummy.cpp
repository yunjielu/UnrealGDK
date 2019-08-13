// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SingletonActorDummy.h"

bool ASingletonActorDummy::IsSupportedForNetworking() const
{
	return true;
}

bool ASingletonActorDummy::IsFullNameStableForNetworking() const
{
	return false;
}
