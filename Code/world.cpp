inline world* 
GetWorld(game* Game, u32 WorldIndex)
{
    world* World = Game->Worlds + WorldIndex;
    return World;
}

inline world* 
GetCurrentWorld(game* Game)
{
    world* World = GetWorld(Game, Game->CurrentWorldIndex);
    return World;
}

inline b32 
IsCurrentWorldIndex(game* Game, u32 WorldIndex)
{
    b32 Result = Game->CurrentWorldIndex == WorldIndex;
    return Result;
}

void IntegrateWorld(game* Game, u32 WorldIndex)
{
    world* World = GetWorld(Game, WorldIndex);
    
    f32 dt = Game->dt;
    for(entity* Entity = World->Entities.First; Entity; Entity = Entity->Next)
    {
        if(Entity->Simulate)
        {            
            Entity->Velocity.xy *= (1.0f / (1.0f + dt*MOVE_DAMPING));    
            Entity->Velocity.z -= dt*10.0f;            
            Entity->Position += Entity->Velocity*dt;
        }
    }    
}

void UpdateWorld(game* Game, u32 WorldIndex)
{    
    world* World = GetWorld(Game, WorldIndex);
    for(entity* FirstEntity = World->Entities.First; FirstEntity; FirstEntity = FirstEntity->Next)
    {
        if(FirstEntity->Simulate)
        {
            aabb3D FirstAABB = TransformAABB3D(FirstEntity->AABB, FirstEntity->Transform);
            for(entity* SecondEntity = World->Entities.First; SecondEntity; SecondEntity = SecondEntity->Next)
            {
                if(FirstEntity != SecondEntity)
                {                    
                    aabb3D SecondAABB = TransformAABB3D(SecondEntity->AABB, SecondEntity->Transform);
                    penetration_result Penetration = PenetrationTestAABB3D(FirstAABB, SecondAABB);
                    if(Penetration.Hit)
                    {
                        f32 VelocityMagntiude = Dot(Penetration.Normal, FirstEntity->Velocity);
                        
                        FirstEntity->Velocity -= (VelocityMagntiude*Penetration.Normal);                        
                        FirstEntity->Position += (Penetration.Normal*Penetration.Distance);
                        
#if 0
                        if(SecondEntity->Simulate)
                        {
                            SecondEntity->Velocity += VelocityMagntiude*Penetration.Normal;
                            SecondEntity->Position -= (Penetration.Normal*Penetration.Distance);
                        }
#endif
                    }
                }
            }
        }
    }
        
    UpdatePlayer(Game, WorldIndex);
}

#include "player.cpp"