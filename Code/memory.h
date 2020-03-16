#ifndef MEMORY_H
#define MEMORY_H

#ifdef CopyMemory
#undef CopyMemory
#endif

inline void CopyMemory(void* Dest, void* Src, ptr Size)
{
    u8* DestAt = (u8*)Dest;
    u8* SrcAt = (u8*)Src;
    while(Size--)
        *DestAt++ = *SrcAt++;
}

inline void ClearMemory(void* Dest, ptr Size)
{
    u8* DestAt = (u8*)Dest;
    while(Size--)
        *DestAt++ = 0;
}

enum clear_flag
{
    __ClearFlagClear__,
    __ClearFlagNoClear__
};

struct arena_block
{        
    ptr Used;
    u8* Memory;
    
    arena_block* Prev;
    arena_block* Next;
};

struct arena
{   
    ptr BlockSize;    
    arena_block* First;
    arena_block* Last;
    arena_block* Current;                
    u32 TemporaryArenas;
};

struct temp_arena
{
    arena* Arena;
    arena_block* BlockMarker;
    ptr BlockUsedMarker;
};

struct buffer
{
    u8* Data;
    ptr Size;
};

#define PushSize( ... ) VA_SELECT( PushSize, __VA_ARGS__)
#define PushSize_3( Size, Clear, Alignment ) _Push_(__Global_Default_Arena__, Size, __ClearFlag##Clear##__, Alignment)
#define PushSize_4( Arena, Size, Clear, Alignment ) _Push_(Arena, Size, __ClearFlag##Clear##__, Alignment)

#define PushStruct( ... ) VA_SELECT(PushStruct, __VA_ARGS__)
#define PushStruct_3( Type, Clear, Alignment) (Type*)_Push_(__Global_Default_Arena__, sizeof(Type), __ClearFlag##Clear##__, Alignment)
#define PushStruct_4( Arena, Type, Clear, Alignment) (Type*)_Push_(Arena, sizeof(Type), __ClearFlag##Clear##__, Alignment)

#define PushArray( ... ) VA_SELECT(PushArray, __VA_ARGS__)
#define PushArray_4( Count, Type, Clear, Alignment) (Type*)_Push_(__Global_Default_Arena__, sizeof(Type)*Count, __ClearFlag##Clear##__, Alignment)
#define PushArray_5( Arena, Count, Type, Clear, Alignment) (Type*)_Push_(Arena, sizeof(Type)*Count, __ClearFlag##Clear##__, Alignment)

#define PushWriteSize( ... ) VA_SELECT( PushWriteSize, __VA_ARGS__)
#define PushWriteSize_4( Data, Size, Clear, Alignment ) _PushWrite_(__Global_Default_Arena__, Size, Data, __ClearFlag##Clear##__, Alignment)
#define PushWriteSize_5( Arena, Data, Size, Clear, Alignment ) _PushWrite_(Arena, Size, Data, __ClearFlag##Clear##__, Alignment)

#define PushWriteStruct( ... ) VA_SELECT(PushWriteStruct, __VA_ARGS__)
#define PushWriteStruct_3( Data, Type, Alignment) (Type*)_PushWrite_(__Global_Default_Arena__, sizeof(Type), Data, Alignment)
#define PushWriteStruct_4( Arena, Data, Type, Alignment) (Type*)_PushWrite_(Arena, sizeof(Type), Data, Alignment)

#define PushWriteArray( ... ) VA_SELECT(PushWriteArray, __VA_ARGS__)
#define PushWriteArray_4( Data, Count, Type, Alignment) (Type*)_PushWrite_(__Global_Default_Arena__, sizeof(Type)*Count, Data, Alignment)
#define PushWriteArray_5( Arena, Data, Count, Type, Alignment) (Type*)_PushWrite_(Arena, sizeof(Type)*Count, Data, Alignment)

#define CopyArray(dst, src, count, type) CopyMemory(dst, src, sizeof(type)*count)
#define ClearStruct(data, type) ClearMemory(data, sizeof(type))
#define ClearArray(data, count, type) ClearMemory(data, sizeof(type)*count)

#ifndef DEFAULT_BLOCK_ARENA_SIZE
#define DEFAULT_BLOCK_ARENA_SIZE MEGABYTE(1)
#endif

#define ALLOCATE_MEMORY(name) void* name(ptr Capacity)
#define FREE_MEMORY(name) void name(void* Memory)

typedef ALLOCATE_MEMORY(allocate_memory);
typedef FREE_MEMORY(free_memory);

global allocate_memory* __Internal_Allocate__;
global free_memory* __Internal_Free__;
global struct arena* __Global_Default_Arena__; 

arena CreateArena(ptr AllocationSize);

inline void InitMemory(arena* DefaultArena, allocate_memory* Allocate = malloc, free_memory* Free = free)
{
    __Internal_Allocate__ = Allocate;
    __Internal_Free__ = Free;    
    __Global_Default_Arena__ = DefaultArena;
}

inline arena CreateArena(ptr BlockSize)
{
    arena Result = {};
    Result.BlockSize = BlockSize;    
    return Result;
}

arena_block* CreateBlock(ptr BlockSize)
{
    arena_block* Block = (arena_block*)__Internal_Allocate__(BlockSize+sizeof(arena_block));
    Block->Memory = (u8*)(Block+1);    
    Block->Prev = Block->Next = 0;
    Block->Used = 0;
    return Block;
}

arena_block* GetBlock(arena* Arena, ptr Size, i32 Alignment)
{
    arena_block* Block = Arena->Current;
    if(!Block)
        return NULL;
    
    ptr CurrentUsed = Block->Used;
    
    if(Alignment != 0)
    {
        ASSERT((Alignment & (Alignment-1)) == 0);
        CurrentUsed = ALIGN(CurrentUsed, Alignment);
    }
    
    if(CurrentUsed+Size > Arena->BlockSize)
    {
        Block = Block->Next;
        return Block;
    }            
    return Block;        
}

void AddBlockToList(arena* Arena, arena_block* Block)
{
    arena_block* Last = Arena->Last;    
    if(!Last)
    {
        ASSERT(!Arena->First);
        Arena->First = Arena->Last = Block;
    }
    else
    {
        Last->Next = Block;
        Block->Prev = Last;
        Arena->Last = Block;
    }
}

inline void* _Push_(arena* Arena, ptr Size, clear_flag Flags, i32 Alignment = 0)
{   
    //IMPORTANT(EVERYONE): We currently do not support allocations larger than the initialize block size for now
    ASSERT(Size <= Arena->BlockSize);
    arena_block* Block = GetBlock(Arena, Size, Alignment); 
    if(!Block)
    {
        Block = CreateBlock(Arena->BlockSize);
        AddBlockToList(Arena, Block);        
    }
    Arena->Current = Block;
    
    if(Alignment != 0)        
        Block->Used = ALIGN(Block->Used, Alignment);        
    
    void* Result = Block->Memory + Block->Used;
    Block->Used += Size;
    
    if(Flags == __ClearFlagClear__)
        ClearMemory(Result, Size);
    
    return Result;
}

inline void* _PushWrite_(arena* Arena, ptr Size, void* Data, i32 Alignment = 0)
{
    void* Memory = _Push_(Arena, Size, __ClearFlagNoClear__, Alignment);
    CopyMemory(Memory, Data, Size);
    return Memory;
}

inline void ResetArena(arena* Arena = __Global_Default_Arena__)
{
    for(arena_block* Block = Arena->First; Block; Block = Block->Next)    
        Block->Used = 0;        
    Arena->Current = Arena->First;
}

inline arena* GetDefaultArena()
{
    return __Global_Default_Arena__;
}

inline void SetDefaultArena(arena* Arena)
{
    __Global_Default_Arena__ = Arena;
}

inline temp_arena BeginTemporaryMemory(arena* Arena = __Global_Default_Arena__)
{
    temp_arena Result;
    Result.Arena = Arena;
    if(!Arena->Current)
    {
        ASSERT(!Arena->First && !Arena->Last);
        Arena->Current = CreateBlock(Arena->BlockSize);
        AddBlockToList(Arena, Arena->Current);
    }
    
    Result.BlockMarker = Arena->Current;
    Result.BlockUsedMarker = Arena->Current->Used;
    Arena->TemporaryArenas++;
    return Result;
}

inline void EndTemporaryMemory(temp_arena* TempArena)
{
    TempArena->Arena->Current = TempArena->BlockMarker;
    TempArena->Arena->Current->Used = TempArena->BlockUsedMarker;
    for(arena_block* Block = TempArena->Arena->Current->Next; Block; Block = Block->Next)
        Block->Used = 0;    
    TempArena->Arena->TemporaryArenas--;
}

inline b32 CheckArena(arena* Arena)
{
    return Arena->TemporaryArenas == 0;
}

#define CHECK_ARENA(Arena) ASSERT(CheckArena(Arena)) 

#endif