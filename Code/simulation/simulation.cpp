#include "gjk.cpp"
#include "epa.cpp"
#include "collision_volumes.cpp"
#include "closest_points.cpp"
#include "rays.cpp"
#include "continuous_collisions.cpp"
#include "penetration.cpp"

continuous_collision DetectStaticContinuousCollisions(game* Game, entity_id EntityID)
{    
    continuous_collision Result = {};
    Result.t = INFINITY;    
    
    toi_result TOIResult = FindStaticTOI(Game, EntityID);
    if(!IsInvalidEntityID(TOIResult.HitEntityID))
    {
        Result.t = TOIResult.t;
        Result.HitEntityID = TOIResult.HitEntityID;
        Result.Penetration = GetPenetration(Game, EntityID, TOIResult.HitEntityID, TOIResult.VolumeA, TOIResult.VolumeB, TOIResult.t);
    }
    
    return Result;
}