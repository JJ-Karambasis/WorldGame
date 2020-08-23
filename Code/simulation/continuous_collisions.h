#ifndef CONTINUOUS_COLLISIONS_H
#define CONTINUOUS_COLLISIONS_H

struct toi_result
{
    f32 t;
    entity_id HitEntityID;
    collision_volume* VolumeA;
    collision_volume* VolumeB;    
};

struct continuous_collision
{
    f32 t;
    entity_id HitEntityID;
    penetration Penetration;
};

inline toi_result
InvalidTOIResult()
{
    toi_result Result;
    Result.t = INFINITY;
    Result.HitEntityID = InvalidEntityID();
    return Result;
}

#endif