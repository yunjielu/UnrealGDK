// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "c_worker.h"

// Check if GDK is compatible with the current version of the Worker SDK
static_assert(WORKER_API_VERSION_STR == "14.2.1", "GDK Version is incompatible with the Worker SDK Version. Please run Setup.bat in the UnrealGDK directory.");
