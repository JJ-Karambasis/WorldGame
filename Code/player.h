#ifndef PLAYER_H
#define PLAYER_H

struct player
{    
    world_entity_id EntityID;
    v3f Radius;        
    b32 IsJumping;
    //animation_controller AnimationController;        
};

struct collision_result
{
    b32 FoundCollision;    
    v3f ContactPoint;
    plane3D SlidingPlane;
    f32 t;
};

struct jumping_quad
{
    v3f CenterP;
    v2f Dimensions;    
    jumping_quad* OtherQuad;
};

void UpdatePlayer(struct game* Game, struct world_entity* PlayerEntity);

#endif