#ifndef PLAYER_H
#define PLAYER_H

enum player_state
{
    PLAYER_STATE_NONE,
    PLAYER_STATE_JUMPING,
    PLAYER_STATE_PUSHING
};

struct pushing_object
{
    entity* PlayerEntity;
    v2f Direction;
};

struct player
{
    player_state State;    
    pushing_object PushingObject;
};

COLLISION_EVENT(OnPlayerCollision);

#endif