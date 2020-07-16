#ifndef PLAYER_H
#define PLAYER_H

#define PLAYER_RADIUS 0.35f
#define PLAYER_HEIGHT 1.3f

struct player
{    
    world_entity_id EntityID;
    v3f Radius;        
    b32 IsJumping;
    //animation_controller AnimationController;        
};

struct jumping_quad
{
    v3f CenterP;
    v2f Dimensions;    
    jumping_quad* OtherQuad;
};

void UpdatePlayer(struct game* Game, struct world_entity* PlayerEntity);

#endif