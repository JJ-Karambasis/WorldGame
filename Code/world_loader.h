#ifndef WORLD_LOADER_H
#define WORLD_LOADER_H

global const ak_char WORLD_SIGNATURE[] = "WGWORLD";
global const ak_u16 WORLD_MAJOR_VERSION = 1;
global const ak_u16 WORLD_MINOR_VERSION = 0;

#pragma pack(push, 1)

struct world_header
{
    ak_char Signature[AK_Count(WORLD_SIGNATURE)];
    ak_u16 MajorVersion;
    ak_u16 MinorVersion;
    ak_u16 WorldEntityCountA;
    ak_u16 WorldEntityCountB;
};

#pragma pack(pop)

struct world_state_initial
{
    ak_array<entity> Entities[2];        
};

#endif