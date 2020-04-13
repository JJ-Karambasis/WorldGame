#ifndef PLAYER_H
#define PLAYER_H

struct player
{
    v3f Position;
    f32 Radius;
    f32 Height;
    v2f FacingDirection;
    c4 Color;
    v3f Velocity;        
};

void UpdatePlayer(game* Game, u32 WorldIndex);

#endif