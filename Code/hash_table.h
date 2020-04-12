#ifndef HASH_TABLE_H
#define HASH_TABLE_H

struct uint_pair
{
    u32 Pair01;
    u32 Pair02;
};

inline u64 Hash(uint_pair Pair, u64 TableSize)
{
    u64 Result = BijectiveMap(Pair.Pair01, Pair.Pair02) % TableSize;
    return Result;
}

inline b32 operator!=(uint_pair Left, uint_pair Right)
{
    b32 Result = (Left.Pair01 != Right.Pair01) || (Left.Pair02 != Right.Pair02);
    return Result;
}

template <typename type>
struct hash_entry
{
    b32 Found;
    type Key;
};

template <typename type>
struct hash_table
{
    u32 TableSize;
    hash_entry<type>* Table;
    
    inline u64 GetHashIndex(type Key)
    {
        u64 HashIndex = Hash(Key, TableSize);
        if(Table[HashIndex].Found && (Table[HashIndex].Key != Key))
        {
            //NOTE(EVERYONE): We are linear probing to resolve has collisions for now. May be customizable at some point
            HashIndex++;
        }
        return HashIndex;
    }
    
    inline b32 operator[](type Key)
    {
        u64 HashIndex = GetHashIndex(Key);
        
        b32 Result = Table[HashIndex].Found;
        if(!Result)
        {
            Table[HashIndex].Key = Key;
            Table[HashIndex].Found = true;
        }
        
        return Result;
    }
};

template <typename type>
inline hash_table<type> CreateHashTable(u32 TableSize)
{
    hash_table<type> Result;
    Result.TableSize = TableSize;
    Result.Table = PushArray(TableSize, hash_entry<type>, Clear, 0);
    return Result;
}

#endif