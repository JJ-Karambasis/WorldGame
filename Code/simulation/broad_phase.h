#ifndef BROAD_PHASE_H
#define BROAD_PHASE_H

struct broad_phase_pair
{
    sim_entity*  SimEntityA;    
    sim_entity*  SimEntityB;
    ak_u64 AVolumeID;
    ak_u64 BVolumeID;    
};

struct broad_phase_pair_list
{
    broad_phase_pair* Ptr;
    ak_u32 Count;
    ak_u32 Capacity;
    
    void AddPair(sim_entity* A, sim_entity* B, ak_u64 AVolumeID, ak_u64 BVolumeID);    
    void AddAllVolumes(struct simulation* Simulation, sim_entity* SimEntityA, sim_entity* SimEntityB);    
};

#endif