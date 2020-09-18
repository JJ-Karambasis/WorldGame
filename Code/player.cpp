inline ak_bool CanBePushed(ak_v2f MoveDirection, ak_v2f ObjectDirection)
{            
    ak_bool Result = AK_Dot(ObjectDirection, MoveDirection) > 0.98f;
    return Result;
}

COLLISION_EVENT(OnPlayerCollision)
{
    player* Player = GetUserData(Entity, player);
    simulation* Simulation = GetSimulation(Game, Entity->ID);        
    
    switch(Player->State)
    {
        case PLAYER_STATE_JUMPING:
        {
            Player->State = PLAYER_STATE_NONE;            
        } break;
        
        case PLAYER_STATE_NONE:
        {   
            if(CollidedEntity->Type == ENTITY_TYPE_PUSHABLE)
            {
                rigid_body* PlayerRigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                
                ak_v2f N = AK_Normalize(Normal.xy);                
                ak_v2f MoveDirection = AK_Normalize(PlayerRigidBody->Acceleration.xy);
                if(CanBePushed(MoveDirection, N))                
                {                    
                    Player->State = PLAYER_STATE_PUSHING;
                    pushing_object* PushingObject = GetUserData(CollidedEntity, pushing_object);
                    PushingObject->PlayerID = Entity->ID;                    
                    PushingObject->Direction = N;
                    
                    PlayerRigidBody->Velocity = {};
                }                     
            }
            
        } break;
    }                
}