#ifndef PLAYER_H
#define PLAYER_H

struct player
{    
    world_entity_id EntityID;
    v3f Radius;
    player_state State;
    pushing_state Pushing;        
    animation_controller AnimationController;        
};

void UpdatePlayer(struct game* Game, struct world_entity* PlayerEntity);

#endif