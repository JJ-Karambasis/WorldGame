#ifndef CONTINUOUS_COLLISIONS_H
#define CONTINUOUS_COLLISIONS_H

struct toi_result
{
    f32 t;
    sim_entity* HitEntity;
    collision_volume* VolumeA;
    collision_volume* VolumeB;    
};

struct continuous_collision
{
    f32 t;
    sim_entity* HitEntity;
    penetration Penetration;
};

inline toi_result
InvalidTOIResult()
{
    toi_result Result;
    Result.t = INFINITY;
    Result.HitEntity = NULL;
    return Result;
}

#endif