void broad_phase_pair_list::AddPair(sim_entity* A, sim_entity* B, ak_u64 AVolumeID, ak_u64 BVolumeID)
{
    AK_Assert(Count < Capacity, "Index out of bounds");
    Ptr[Count++] = {A, B, AVolumeID, BVolumeID};
}

void broad_phase_pair_list::AddPair(broad_phase_pair Pair)
{
    AK_Assert(Count < Capacity, "Index out of bounds");
    Ptr[Count++] = Pair;
}

void broad_phase_pair_list::AddAllVolumes(simulation* Simulation, sim_entity* SimEntityA, sim_entity* SimEntityB)
{
    collision_volume* AVolume = Simulation->CollisionVolumeStorage.Get(SimEntityA->CollisionVolumeID);
    while(AVolume)
    {
        collision_volume* BVolume = Simulation->CollisionVolumeStorage.Get(SimEntityB->CollisionVolumeID);
        while(BVolume)
        {
            AddPair(SimEntityA, SimEntityB, AVolume->ID, BVolume->ID);
            BVolume = Simulation->CollisionVolumeStorage.Get(BVolume->NextID);
        }
        
        AVolume = Simulation->CollisionVolumeStorage.Get(AVolume->NextID);
    }        
}

void broad_phase_pair_list::AddPassedVolumes(simulation* Simulation, sim_entity* SimEntityA, sim_entity* SimEntityB, broad_phase_pair_filter_func* FilterFunc, void* UserData)
{
    collision_volume* AVolume = Simulation->CollisionVolumeStorage.Get(SimEntityA->CollisionVolumeID);
    while(AVolume)
    {
        collision_volume* BVolume = Simulation->CollisionVolumeStorage.Get(SimEntityB->CollisionVolumeID);
        while(BVolume)
        {
            broad_phase_pair Pair = {SimEntityA, SimEntityB, AVolume->ID, BVolume->ID};            
            if(FilterFunc(&Pair, UserData))            
                AddPair(Pair);                            
            BVolume = Simulation->CollisionVolumeStorage.Get(BVolume->NextID);
        }
        
        AVolume = Simulation->CollisionVolumeStorage.Get(AVolume->NextID);
    }        
}

BROAD_PHASE_PAIR_FILTER_FUNC(FilterFuncPass) { return true; }
BROAD_PHASE_PAIR_FILTER_FUNC(FilterSimEntityOnlyCollisions)
{
    if(Pair->SimEntityB->ID.Type == SIM_ENTITY_TYPE_SIM_ENTITY)
        return true;
    return false;
}