void OutputWorld(game* Game, ak_char* Path)
{   
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_u32 Size = sizeof(world_header);
    
    world_header Header;
    AK_MemoryCopy(Header.Signature, WORLD_SIGNATURE, sizeof(WORLD_SIGNATURE));
    Header.MajorVersion = WORLD_MAJOR_VERSION;
    Header.MinorVersion = WORLD_MINOR_VERSION;
#if 0 
    AK_ForEach(Entity, &Game->EntityStorage[0])
    {
        switch(Entity->Type)
        {
        }
    }
#endif
    GlobalArena->EndTemp(&TempArena);
}