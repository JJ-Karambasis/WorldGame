#ifndef BROAD_PHASE_H
#define BROAD_PHASE_H

struct broad_phase_pair
{
    sim_entity*  SimEntityA;    
    sim_entity*  SimEntityB;
    collision_volume* VolumeA;
    collision_volume* VolumeB;
};

struct broad_phase_pair_list
{
    broad_phase_pair* Ptr;
    u32 Count;
    u32 Capacity;
    
    inline void AddPair(sim_entity* A, sim_entity* B, collision_volume* AVolume, collision_volume* BVolume)
    {
        ASSERT(Count < Capacity);
        Ptr[Count++] = {A, B, AVolume, BVolume};
    }
    
    inline void AddAllVolumes(sim_entity* SimEntityA, sim_entity* SimEntityB)
    {
        FOR_EACH(EntityVolume, SimEntityA->CollisionVolumes)       
        {
            FOR_EACH(TestEntityVolume, SimEntityB->CollisionVolumes) { AddPair(SimEntityA, SimEntityB, EntityVolume, TestEntityVolume); }             
        }
    }
};

#endif