#ifndef BROAD_PHASE_H
#define BROAD_PHASE_H

struct broad_phase_pair
{
    ak_u64 EntityA;
    ak_u64 EntityB;
    ak_u64 VolumeA;
    ak_u64 VolumeB;
};

typedef ak_fixed_array<broad_phase_pair> broad_phase_pair_list;

#define BROAD_PHASE_PAIR_FILTER_FUNC(name) ak_bool name(struct broad_phase* BroadPhase, broad_phase_pair Pair, void* UserData)
typedef BROAD_PHASE_PAIR_FILTER_FUNC(broad_phase_pair_filter_func);

struct broad_phase
{
    world* World;
    ak_u32 WorldIndex;
    
    broad_phase_pair_list GetAllPairs(ak_arena* Arena, broad_phase_pair_filter_func* FilterFunc=NULL, 
                                      void* UserData=NULL);
    
    broad_phase_pair_list GetPairs(ak_arena* Arena, ak_u64 ID, broad_phase_pair_filter_func* FilterFunc=NULL, void* UserData=NULL);
    
    broad_phase_pair_list FilterPairs(ak_arena* Arena, broad_phase_pair_list List, 
                                      broad_phase_pair_filter_func* FilterFunc, void* UserData=NULL);
};

#endif
