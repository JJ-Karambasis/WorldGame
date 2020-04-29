#ifndef POOL_H
#define POOL_H

template <typename type>
struct pool_entry
{
    i64 ID;
    type Entry;
};

template <typename type>
struct pool
{
    pool_entry<type>* Entries;
    i32 MaxUsed;
    i64 NextKey;
    i32 FreeHead;
    i32 Capacity;
};

template <typename type>
pool<type> CreatePool(arena* Arena, i32 Capacity)
{
    pool<type> Result = {};
    Result.NextKey = 1;
    Result.FreeHead = -1;
    Result.Capacity = Capacity;
    Result.Entries = PushArray(Arena, Capacity, pool_entry<type>, Clear, 0);
    return Result;
}

template <typename type>
i64 AllocateFromPool(pool<type>* Pool)
{
    i32 Index;
    if(Pool->FreeHead != -1)
    {
        Index = Pool->FreeHead;
    }
    else
    {
        //CONFIRM(JJ): Should we handle a dynamically grow entity pool? Doubt it
        ASSERT(Pool->MaxUsed < Pool->Capacity);
        Index = Pool->MaxUsed++;
    }
    
    pool_entry<type>* Entry = Pool->Entries + Index;    
    if(Pool->FreeHead != -1)    
        Pool->FreeHead = Entry->ID & 0xFFFFFFFF;
    
    Entry->ID = (Pool->NextKey++ << 32) | Index;
    return Entry->ID;
}

inline i32 GetPoolIndex(i64 ID)
{
    i32 Result = (i32)(ID & 0xFFFFFFFF);
    return Result;
}

inline b32 IsAllocatedID(i64 ID)
{
    b32 Result = (ID & 0xFFFFFFFF00000000) != 0;
    return Result;
}

template <typename type>
type* GetByID(pool<type>* Pool, i64 ID)
{
    ASSERT(IsAllocatedID(ID));
    i32 Index = GetPoolIndex(ID);    
    type* Result = &Pool->Entries[Index].Entry;
    return Result;
}

template <typename type>
struct pool_iter
{
    pool<type>* Pool;
    i64 CurrentID;
};

template <typename type>
pool_iter<type> BeginIter(pool<type>* Pool)
{
    pool_iter<type> Result = {};
    Result.Pool = Pool;
    Result.CurrentID = -1;
    return Result;
}

template <typename type>
type* GetFirst(pool_iter<type>* Iter)
{
    ASSERT(Iter->CurrentID == -1);
    
    pool<type>* Pool = Iter->Pool;
    for(i32 Index = 0; Index < Pool->Capacity; Index++)
    {
        pool_entry<type>* Entry = Pool->Entries + Index;
        if(IsAllocatedID(Entry->ID))
        {
            Iter->CurrentID = Entry->ID;
            return &Entry->Entry;
        }
    }
    
    return NULL;
}

template <typename type>
type* GetNext(pool_iter<type>* Iter)
{
    ASSERT(Iter->CurrentID != -1);    
    
    pool<type>* Pool = Iter->Pool;
    for(i32 Index = GetPoolIndex(Iter->CurrentID)+1; Index < Pool->Capacity; Index++)
    {
        pool_entry<type>* Entry = Pool->Entries + Index;
        if(IsAllocatedID(Entry->ID))
        {
            Iter->CurrentID = Entry->ID;
            return &Entry->Entry;
        }
    }
    
    return NULL;
}
                                  

#endif