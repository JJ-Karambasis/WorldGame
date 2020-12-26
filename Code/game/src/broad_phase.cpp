#define INTERNAL__BROADPHASE_MAX_CAPACITY 1024

void Internal__AddPairEntry(broad_phase_pair_list* List, broad_phase_pair Pair)
{
    AK_Assert(List->Size < INTERNAL__BROADPHASE_MAX_CAPACITY, "Index out of bounds");
    List->Data[List->Size++] = Pair;
}

BROAD_PHASE_PAIR_FILTER_FUNC(BroadPhase_DefaultFilterFunc)
{
    return true;
}

broad_phase BroadPhase_Begin(world* World, ak_u32 WorldIndex)
{
    broad_phase Result;
    Result.World = World;
    Result.WorldIndex = WorldIndex;
    return Result;
}

broad_phase_pair_list broad_phase::GetPairs(ak_arena* Arena, ak_u64 TestID, broad_phase_pair_filter_func* FilterFunc,
                                            void* UserData)
{
    if(FilterFunc == NULL)
        FilterFunc = BroadPhase_DefaultFilterFunc;
    
    ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
    ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
    
    physics_object* TestObject = PhysicsObjects->Get(AK_PoolIndex(TestID));
    
    broad_phase_pair_list Result = {};
    Result.Data = Arena->PushArray<broad_phase_pair>(INTERNAL__BROADPHASE_MAX_CAPACITY);
    
    for(ak_u32 EntityIndex = 0; EntityIndex < EntityStorage->MaxUsed; EntityIndex++)
    {
        ak_u64 ID = EntityStorage->IDs[EntityIndex];
        if(AK_PoolIsAllocatedID(ID) && (TestID != ID))
        {
            physics_object* Object = PhysicsObjects->Get(AK_PoolIndex(ID));
            
            collision_volume* AVolume = World->CollisionVolumeStorage.Get(TestObject->CollisionVolumeID);
            while(AVolume)
            {
                collision_volume* BVolume = World->CollisionVolumeStorage.Get(Object->CollisionVolumeID);
                while(BVolume)
                {
                    broad_phase_pair Pair = {TestID, ID, AVolume->ID, BVolume->ID};
                    if(FilterFunc(this, Pair, UserData))
                        Internal__AddPairEntry(&Result, Pair);
                    
                    BVolume = World->CollisionVolumeStorage.Get(BVolume->NextID);
                }
                
                AVolume = World->CollisionVolumeStorage.Get(AVolume->NextID);
            }
        }
    }
    
    return Result;
}
