#ifndef WORLDA_H
#define WORLDA_H
#include <game.h>
#include "generated.h"

struct worlda : public world
{
	entities_a WorldEntitiesA;
	entities_b WorldEntitiesB;
	point_lights_a PointLightsA;
	point_lights_b PointLightsB;
};
extern "C" AK_EXPORT WORLD_STARTUP(WorldA_Startup);
extern "C" AK_EXPORT WORLD_UPDATE(WorldA_Update);
extern "C" AK_EXPORT WORLD_SHUTDOWN(WorldA_Shutdown);
#endif
