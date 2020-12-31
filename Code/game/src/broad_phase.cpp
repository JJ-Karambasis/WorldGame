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

BROAD_PHASE_PAIR_FILTER_FUNC(BroadPhase_FilterStaticEntitiesFunc)
{
    ak_pool<entity>* EntityStorage = &BroadPhase->World->EntityStorage[BroadPhase->WorldIndex];
    ak_bool AreStatic = (EntityStorage->Get(Pair.EntityA)->Type == ENTITY_TYPE_STATIC ||
                         EntityStorage->Get(Pair.EntityB)->Type == ENTITY_TYPE_STATIC);
    return !AreStatic;
}

BROAD_PHASE_PAIR_FILTER_FUNC(BroadPhase_FilterOnlyStaticEntitiesFunc)
{
    ak_pool<entity>* EntityStorage = &BroadPhase->World->EntityStorage[BroadPhase->WorldIndex];
    ak_bool AreStatic = (EntityStorage->Get(Pair.EntityA)->Type == ENTITY_TYPE_STATIC ||
                         EntityStorage->Get(Pair.EntityB)->Type == ENTITY_TYPE_STATIC);
    return AreStatic;
}

broad_phase BroadPhase_Begin(world* World, ak_u32 WorldIndex)
{
    broad_phase Result;
    Result.World = World;
    Result.WorldIndex = WorldIndex;
    return Result;
}

global ak_hash_map<ak_pair<ak_u32>, ak_bool> Internal__CollisionMap;

broad_phase_pair_list broad_phase::GetAllPairs(ak_arena* Arena, broad_phase_pair_filter_func* FilterFunc, 
                                               void* UserData)
{
    ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
    ak_pool<collision_volume>* CollisionVolumes = &World->CollisionVolumeStorage;
    ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
    
    if(!Internal__CollisionMap.Slots)
        Internal__CollisionMap = AK_CreateHashMap<ak_pair<ak_u32>, ak_bool>(8191);
    
    if(FilterFunc == NULL)
        FilterFunc = BroadPhase_DefaultFilterFunc;
    
    broad_phase_pair_list Result = {};
    Result.Data = Arena->PushArray<broad_phase_pair>(INTERNAL__BROADPHASE_MAX_CAPACITY);
    
    for(ak_u32 EntityIndex = 0; EntityIndex < EntityStorage->MaxUsed; EntityIndex++)
    {
        ak_u64 ID = EntityStorage->IDs[EntityIndex];
        if(AK_PoolIsAllocatedID(ID))
        {
            for(ak_u32 TestEntityIndex = 0; TestEntityIndex < EntityStorage->MaxUsed; TestEntityIndex++)
            {
                ak_u64 TestID = EntityStorage->IDs[TestEntityIndex];
                if(AK_PoolIsAllocatedID(TestID))
                {
                    if(TestID != ID)
                    {
                        ak_u32 AIndex = AK_PoolIndex(ID);
                        ak_u32 BIndex = AK_PoolIndex(TestID);
                        
                        ak_pair<ak_u32> Pair = {AIndex, BIndex};
                        if(!Internal__CollisionMap.Find(Pair))
                        {
                            Internal__CollisionMap.Insert(Pair, true);
                            
                            physics_object* Object = PhysicsObjects->Get(AIndex);
                            physics_object* TestObject = PhysicsObjects->Get(BIndex);
                            
                            collision_volume* AVolume = CollisionVolumes->Get(Object->CollisionVolumeID);
                            while(AVolume)
                            {
                                collision_volume* BVolume = CollisionVolumes->Get(TestObject->CollisionVolumeID);
                                while(BVolume)
                                {
                                    broad_phase_pair BPPair = {TestID, ID, AVolume->ID, BVolume->ID};
                                    if(FilterFunc(this, BPPair, UserData))
                                        Internal__AddPairEntry(&Result, BPPair);
                                    
                                    BVolume = World->CollisionVolumeStorage.Get(BVolume->NextID);
                                }
                                
                                AVolume = World->CollisionVolumeStorage.Get(AVolume->NextID);
                            }
                        }
                    }
                }
            }
        }
    }
    
    Internal__CollisionMap.Reset();
    
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

broad_phase_pair_list broad_phase::FilterPairs(ak_arena* Arena, broad_phase_pair_list List, broad_phase_pair_filter_func* FilterFunc, void* UserData)
{
    broad_phase_pair_list Result = {};
    Result.Data = Arena->PushArray<broad_phase_pair>(INTERNAL__BROADPHASE_MAX_CAPACITY);
    
    AK_ForEach(Pair, &List)
    {
        if(FilterFunc(this, *Pair, UserData))
            Internal__AddPairEntry(&Result, *Pair);
    }
    
    return Result;
}