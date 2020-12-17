#ifndef GAME_H
#define GAME_H

#include <engine.h>
#include "include/entity.h"

struct world
{
    ak_pool<entity> EntityStorage[2];
    ak_array<ak_sqtf> OldTransforms[2];
    ak_array<physics_objects> PhysicObjects[2];
    player Players[2];
};

struct world_game : public game
{
    ak_u32 CurrentWorldIndex;
};

#endif
