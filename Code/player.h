#ifndef PLAYER_H
#define PLAYER_H

#define MOVE_ACCELERATION 20.0f
#define MOVE_DAMPING 5.0f

enum player_state
{
    PLAYER_STATE_DEFAULT,
    PLAYER_STATE_PUSHING
};

struct player
{
    v3f Position;
    f32 Radius;
    f32 Height;
    v2f FacingDirection;
    c4 Color;
    v3f Velocity;
    
    player_state State;
    struct box_entity* PushingBlock;
    v2f PushDirection;
};

void UpdatePlayer(game* Game, u32 WorldIndex);

#endif