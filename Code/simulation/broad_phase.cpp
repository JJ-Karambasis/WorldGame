void broad_phase_pair_list::AddPair(sim_entity* A, sim_entity* B, ak_u64 AVolumeID, ak_u64 BVolumeID)
{
    AK_Assert(Count < Capacity, "Index out of bounds");
    Ptr[Count++] = {A, B, AVolumeID, BVolumeID};
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