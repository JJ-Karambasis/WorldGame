ak_string GetWorldBuildFile(ak_arena* Scratch, ak_string GameCodePath, ak_string WorldName)
{
    return AK_FormatString(Scratch, Global_WorldBuildFileTemplate, GameCodePath.Length, GameCodePath.Data, WorldName.Length, WorldName.Data);
}

ak_string GetWorldHeaderFile(ak_arena* Scratch, ak_string WorldName)
{
    ak_string WorldNameLower = AK_ToLower(WorldName, Scratch);
    ak_string WorldNameUpper = AK_ToUpper(WorldName, Scratch);
    return AK_FormatString(Scratch, Global_WorldHeaderFileTemplate, 
                           WorldNameUpper.Length, WorldNameUpper.Data, 
                           WorldNameUpper.Length, WorldNameUpper.Data, 
                           WorldNameLower.Length, WorldNameLower.Data, 
                           WorldName.Length, WorldName.Data, 
                           WorldName.Length, WorldName.Data, 
                           WorldName.Length, WorldName.Data);
}

ak_string GetWorldSourceFile(ak_arena* Scratch, ak_string WorldName)
{
    ak_string WorldNameLower = AK_ToLower(WorldName, Scratch);
    return AK_FormatString(Scratch, Global_WorldSourceFileTemplate, 
                           WorldName.Length, WorldName.Data, 
                           WorldName.Length, WorldName.Data, 
                           WorldNameLower.Length, WorldNameLower.Data, 
                           WorldNameLower.Length, WorldNameLower.Data, 
                           WorldNameLower.Length, WorldNameLower.Data, 
                           WorldName.Length, WorldName.Data, 
                           WorldName.Length, WorldName.Data, 
                           WorldName.Length, WorldName.Data, 
                           WorldNameLower.Length, WorldNameLower.Data, 
                           WorldNameLower.Length, WorldNameLower.Data,
                           WorldName.Length, WorldName.Data);
}
