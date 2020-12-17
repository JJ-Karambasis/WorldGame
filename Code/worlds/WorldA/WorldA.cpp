#include "WorldA.h"

extern "C"
AK_EXPORT WORLD_STARTUP(WorldA_Startup)
{
	worlda* World = (worlda*)AK_Allocate(sizeof(worlda));
	World->Update = WorldA_Update;
	World->Shutdown = WorldA_Shutdown;
	Game->World = World;
	return true;
}

extern "C"
AK_EXPORT WORLD_UPDATE(WorldA_Update)
{
}

extern "C"
AK_EXPORT WORLD_SHUTDOWN(WorldA_Shutdown)
{
	AK_DeletePool(&Game->World->EntityStorage[0]);
	AK_DeletePool(&Game->World->EntityStorage[1]);
	AK_DeleteArray(&Game->World->OldTransforms[0]);
	AK_DeleteArray(&Game->World->OldTransforms[1]);
	AK_DeleteArray(&Game->World->PhysicObjects[0]);
	AK_DeleteArray(&Game->World->PhysicObjects[1]);
	AK_Free(Game->World);
	Game->World = NULL;
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>
