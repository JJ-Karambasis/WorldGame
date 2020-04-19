#ifndef PLAYER_H
#define PLAYER_H

#define MOVE_ACCELERATION 20.0f
#define MOVE_DAMPING 5.0f

enum player_state
{
    PLAYER_STATE_DEFAULT,
    PLAYER_STATE_PUSHING
};

struct pushing_state
{
    struct box_entity* Object;
    v2f Direction;
};

struct player
{
    v3f Position;
    v2f FacingDirection;
    c4 Color;
    v3f Velocity;
    
    player_state State;
    pushing_state Pushing;    
};

void UpdatePlayer(game* Game, u32 WorldIndex);

#endif