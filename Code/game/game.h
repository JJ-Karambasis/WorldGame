#ifndef GAME_H
#define GAME_H

#include <engine.h>

struct world_game : public game
{
    ak_u32 CurrentWorldIndex;
    player Players[2];
};

#endif
