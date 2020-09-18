#include "asset_builder.h"
#include "asset_builder_fbx.cpp"
#include "asset_builder_png.cpp"

enum command_argument_type
{    
    COMMAND_ARGUMENT_TYPE_CREATE,
    COMMAND_ARGUMENT_TYPE_DELETE,
    COMMAND_ARGUMENT_COUNT
};

struct file_raii
{
    ak_file_handle* File;    
    
    inline file_raii(ak_char* Path, ak_file_attributes Attributes)
    {
        File = AK_OpenFile(Path, Attributes);        
    }
    
    inline ~file_raii()
    {
        AK_CloseFile(File);        
    }
};

inline ak_bool IsNextArgument(ak_char* Parameter)
{
    if(Parameter[0] == '-')
        return true;
    
    return false;
}

struct command_line
{
    ak_bool IsHelpPresent;
    ak_array<ak_string> Arguments[COMMAND_ARGUMENT_COUNT];
    
    void HandleArgument(command_argument_type Type, ak_char** Args, ak_i32 ArgIndex, ak_i32 ArgCount)
    {
        for(ak_i32 ParameterIndex = ArgIndex+1; ParameterIndex < ArgCount; ParameterIndex++)
        {
            ak_char* Parameter = Args[ParameterIndex];
            if(IsNextArgument(Parameter))
                break;
            
            Arguments[Type].Add(AK_CreateString(Parameter));
        }
    }
    
    void Parse(ak_char** Args, ak_i32 ArgCount)
    {
        for(ak_i32 ArgIndex = 0; ArgIndex < ArgCount; ArgIndex++)
        {
            ak_char* Argument = Args[ArgIndex];
            if(AK_StringEquals(Argument, "-?"))
                IsHelpPresent = true;
            else if(AK_StringEquals(Argument, "-c"))
                HandleArgument(COMMAND_ARGUMENT_TYPE_CREATE, Args, ArgIndex, ArgCount);
            else if(AK_StringEquals(Argument, "-d"))
                HandleArgument(COMMAND_ARGUMENT_TYPE_DELETE, Args, ArgIndex, ArgCount);            
        }
    }
    
    void DisplayHelp()
    {
        ConsoleLog("Option| Info                  | Example");
        ConsoleLog("-c    | Used to create assets | '-c Path\\To\\AssetsFile.fbx'");
        ConsoleLog("-d    | Used to delete assets | '-d AssetType NameOfAsset' or '-d AssetType Slot'");
        ConsoleNewLine();        
        ConsoleLog("AssetTypes:");
        ConsoleLog("\tMesh");
        ConsoleLog("\tTexture");
        ConsoleLog("\tMusic");
        ConsoleLog("\tSoundFX");
        ConsoleLog("\tAnimations");
    }
};

void ParseFile(asset_builder* AssetBuilder, ak_string File, ak_string Ext)
{
    ConsoleLog("-------------------------");
    ConsoleLog("Started Parsing File %s", File.Data);
    
    if(AK_StringEquals(Ext, "fbx"))            
        ParseFBX(AssetBuilder, File);        
    else if(AK_StringEquals(Ext, "png"))
        ParsePNG(AssetBuilder, File);
    else
        ConsoleError("Unknown file extension %s", Ext.Data);
    
    ConsoleLog("Ended Parsing File %s", File.Data);                
}

void CreateAssets(asset_builder* AssetBuilder, ak_array<ak_string>& CreateParameters)
{
    ConsoleLog("Creating assets");
    ConsoleNewLine();
    
    ak_arena* GlobalArena = AK_GetGlobalArena();
    for(ak_u32 FileIndex = 0; FileIndex < CreateParameters.Size; FileIndex++)
    {
        ak_string FileOrDirectory = CreateParameters[FileIndex];
        
        ak_string Ext = AK_GetFileExtension(FileOrDirectory);
        if(AK_StringIsNullOrEmpty(Ext))
        {               
            ak_temp_arena TempArena = GlobalArena->BeginTemp();
            ak_array<ak_string> Results = AK_GetAllFilesInDirectory(FileOrDirectory, GlobalArena);
            AK_ForEach(File, &Results)
            {
                Ext = AK_GetFileExtension(*File);
                ParseFile(AssetBuilder, *File, Ext);
            }            
            GlobalArena->EndTemp(&TempArena);
        }
        else
        {
            ParseFile(AssetBuilder, FileOrDirectory, Ext);
        }
        
        if(FileIndex == (CreateParameters.Size - 1))
            ConsoleLog("-------------------------");
    }   
    ConsoleNewLine();
    ConsoleLog("Finished creating assets");
    ConsoleNewLine();
}

void DeleteAssets(asset_builder* AssetBuilder, ak_array<ak_string>& DeleteParameters)
{
    if(DeleteParameters.Size > 0)
    {
        ConsoleLog("Deleting assets");
        ConsoleNewLine();
        
        if((DeleteParameters.Size % 2) != 0)
        {
            ConsoleError("Deleting assets come in type/name pairs. Found an invalid amount of deletion parameters. Skipping asset deletion");
            return;
        }
        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        
        for(ak_u32 FileIndex = 0; FileIndex < DeleteParameters.Size; FileIndex += 2)
        {            
            ak_temp_arena TempArena = GlobalArena->BeginTemp();
            ak_string Type = AK_ToLower(DeleteParameters[FileIndex], GlobalArena);
            ak_string Name = DeleteParameters[FileIndex+1];
            
            if(AK_StringEquals(Type, "mesh"))
            {
                ConsoleLog("Deleting mesh %s", Name.Data);
                
                mesh_pair* Pair = AssetBuilder->MeshTable.Find(Name.Data);
                if(Pair)
                {                
                    ak_link_entry<mesh_info>* OldMeshInfo = (ak_link_entry<mesh_info>*)Pair->MeshInfo;
                    ak_link_entry<mesh>* OldMesh = (ak_link_entry<mesh>*)Pair->Mesh;
                    
                    AssetBuilder->MeshInfos.Remove(OldMeshInfo);
                    AssetBuilder->Meshes.Remove(OldMesh);                    
                }
                else
                {
                    ConsoleError("Could not find mesh to delete");
                }            
            }
            else if(AK_StringEquals(Type, "texture"))
            {
                ConsoleLog("Deleting texture %s", Name.Data);
                
                texture_pair* Pair = AssetBuilder->TextureTable.Find(Name.Data);
                if(Pair)
                {
                    ak_link_entry<texture_info>* OldTextureInfo = (ak_link_entry<texture_info>*)Pair->TextureInfo;
                    ak_link_entry<texture>* OldTexture = (ak_link_entry<texture>*)Pair->Texture;
                    
                    AssetBuilder->TextureInfos.Remove(OldTextureInfo);
                    AssetBuilder->Textures.Remove(OldTexture);                    
                }
                else
                {
                    ConsoleError("Could not find texture to delete");
                }
            }
            else
            {
                ConsoleError("Invalid asset type %s. Skipping deletion of asset %s", DeleteParameters[FileIndex].Data, Name.Data);
            }        
            
            ConsoleNewLine();
            GlobalArena->EndTemp(&TempArena);
        }
        
        ConsoleLog("Finished deleting assets");
        ConsoleNewLine();    
    }
}

void ReadMeshInfo(asset_builder* AssetBuilder, mesh_info* MeshInfo, ak_file_handle* FileHandle)
{
    AK_ReadFile(FileHandle, &MeshInfo->Header, sizeof(mesh_info_header));    
    MeshInfo->Name = AssetBuilder->AssetArena->PushArray<ak_char>(MeshInfo->Header.NameLength+1);
    MeshInfo->ConvexHulls = AssetBuilder->AssetArena->PushArray<convex_hull>(MeshInfo->Header.ConvexHullCount);
    
    AK_ReadFile(FileHandle, MeshInfo->Name, sizeof(char)*MeshInfo->Header.NameLength);    
    MeshInfo->Name[MeshInfo->Header.NameLength] = 0;
    
    for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
    {
        convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
        AK_ReadFile(FileHandle, &ConvexHull->Header, sizeof(ConvexHull->Header));
        
        ak_u32 ConvexHullDataSize = 0;
        ConvexHullDataSize += (ConvexHull->Header.VertexCount*sizeof(half_vertex));                
        ConvexHullDataSize += (ConvexHull->Header.EdgeCount*sizeof(half_edge));
        ConvexHullDataSize += (ConvexHull->Header.FaceCount*sizeof(half_face));
        
        void* ConvexHullData = AssetBuilder->AssetArena->Push(ConvexHullDataSize);
        AK_ReadFile(FileHandle, ConvexHullData, ConvexHullDataSize);
        
        ConvexHull->Vertices = (half_vertex*)ConvexHullData;
        ConvexHull->Edges = (half_edge*)(ConvexHull->Vertices + ConvexHull->Header.VertexCount);
        ConvexHull->Faces = (half_face*)(ConvexHull->Edges + ConvexHull->Header.EdgeCount);
    }    
}

void ReadMeshInfos(asset_builder* AssetBuilder, ak_link_entry<mesh_info>* MeshInfos, ak_u32 MeshCount, ak_file_handle* FileHandle)
{
    for(ak_u32 MeshIndex = 0; MeshIndex < MeshCount; MeshIndex++)
    {
        ReadMeshInfo(AssetBuilder, &MeshInfos[MeshIndex].Entry, FileHandle);        
        AssetBuilder->MeshInfos.Push(&MeshInfos[MeshIndex]);        
    }
}

void ReadTextureInfo(asset_builder* AssetBuilder, texture_info* TextureInfo, ak_file_handle* FileHandle)
{
    AK_ReadFile(FileHandle, &TextureInfo->Header, sizeof(texture_info_header));    
    TextureInfo->Name = AssetBuilder->AssetArena->PushArray<char>(TextureInfo->Header.NameLength+1);
    AK_ReadFile(FileHandle, TextureInfo->Name, sizeof(char)*TextureInfo->Header.NameLength);    
    TextureInfo->Name[TextureInfo->Header.NameLength] = 0;            
}

void ReadTextureInfos(asset_builder* AssetBuilder, ak_link_entry<texture_info>* TextureInfos, ak_u32 TextureCount, ak_file_handle* FileHandle)
{
    for(ak_u32 TextureIndex = 0; TextureIndex < TextureCount; TextureIndex++)
    {
        ReadTextureInfo(AssetBuilder, &TextureInfos[TextureIndex].Entry, FileHandle);
        AssetBuilder->TextureInfos.Push(&TextureInfos[TextureIndex]);        
    }
}

void ReadMesh(asset_builder* AssetBuilder, mesh* Mesh, mesh_info* MeshInfo, ak_file_handle* FileHandle)
{
    ak_u32 MeshSize = GetMeshDataSize(MeshInfo);
    void* Data = AssetBuilder->AssetArena->Push(MeshSize);
    Mesh->Vertices = Data;
    Mesh->Indices = ((ak_u8*)Mesh->Vertices + GetVertexStride(MeshInfo)*MeshInfo->Header.VertexCount);   
    AK_ReadFile(FileHandle, Data, MeshSize);    
}

void ReadMeshes(asset_builder* AssetBuilder, ak_link_entry<mesh>* Meshes, ak_link_entry<mesh_info>* MeshInfos, ak_u32 MeshCount, ak_file_handle* FileHandle)
{
    for(ak_u32 MeshIndex = 0; MeshIndex < MeshCount; MeshIndex++)
    {
        ReadMesh(AssetBuilder, &Meshes[MeshIndex].Entry, &MeshInfos[MeshIndex].Entry, FileHandle);
        AssetBuilder->Meshes.Push(&Meshes[MeshIndex]);        
    }
}

void ReadTexture(asset_builder* AssetBuilder, texture* Texture, texture_info* TextureInfo, ak_file_handle* FileHandle)
{
    ak_u32 TextureSize = GetTextureDataSize(TextureInfo);
    void* Data = AssetBuilder->AssetArena->Push(TextureSize);
    Texture->Texels = Data;    
    AK_ReadFile(FileHandle, Data, TextureSize);    
}

void ReadTextures(asset_builder* AssetBuilder, ak_link_entry<texture>* Textures, ak_link_entry<texture_info>* TextureInfos, ak_u32 TextureCount, ak_file_handle* FileHandle)
{
    for(ak_u32 TextureIndex = 0; TextureIndex < TextureCount; TextureIndex++)
    {
        ReadTexture(AssetBuilder, &Textures[TextureIndex].Entry, &TextureInfos[TextureIndex].Entry, FileHandle);
        AssetBuilder->Textures.Push(&Textures[TextureIndex]);        
    }
}

void ReadAssets(asset_builder* AssetBuilder, ak_string AssetPath)
{
    ConsoleLog("Reading current asset file");
    
    file_raii FileRAII(AssetPath.Data, AK_FILE_ATTRIBUTES_READ);    
    if(!FileRAII.File)
    {
        ConsoleError("Could not open asset file for reading. Application will create a new one instead.");
        return;
    }
    
    asset_header Header = {};
    AK_ReadFile(FileRAII.File, &Header, sizeof(Header));
    
    if(!ValidateSignature(Header))
    {
        ConsoleError("Could not validate the asset file signature. Application will create a new one instead.");
        return;
    }
    
    if(!ValidateVersion(Header))
    {
        //CONFIRM(JJ): Should we try and update the assets even if the asset versions do not matchup? How would this work. Try and match versions and provide update scripts?        
        ConsoleError("The versions of the asset file we are reading and the one we are creating do not match. Applcation will create a new fresh one instead.");
        return;
    }
    
    ak_link_entry<mesh_info>* MeshInfos = AssetBuilder->AssetArena->PushArray<ak_link_entry<mesh_info>>(Header.MeshCount);
    ak_link_entry<texture_info>* TextureInfos = AssetBuilder->AssetArena->PushArray<ak_link_entry<texture_info>>(Header.TextureCount);
    
    ak_link_entry<mesh>* Meshes = AssetBuilder->AssetArena->PushArray<ak_link_entry<mesh>>(Header.MeshCount);
    ak_link_entry<texture>* Textures = AssetBuilder->AssetArena->PushArray<ak_link_entry<texture>>(Header.TextureCount);
    
    ReadMeshInfos(AssetBuilder, MeshInfos, Header.MeshCount, FileRAII.File);
    ReadTextureInfos(AssetBuilder, TextureInfos, Header.TextureCount, FileRAII.File);
    
    ReadMeshes(AssetBuilder, Meshes, MeshInfos, Header.MeshCount, FileRAII.File);
    ReadTextures(AssetBuilder, Textures, TextureInfos, Header.TextureCount, FileRAII.File);
    
    for(ak_u32 MeshIndex = 0; MeshIndex < Header.MeshCount; MeshIndex++)
    {
        mesh_pair Pair = {&MeshInfos[MeshIndex].Entry, &Meshes[MeshIndex].Entry};
        AssetBuilder->MeshTable.Insert(Pair.MeshInfo->Name, Pair);
    }
    
    for(ak_u32 TextureIndex = 0; TextureIndex < Header.TextureCount; TextureIndex++)
    {
        texture_pair Pair = {&TextureInfos[TextureIndex].Entry, &Textures[TextureIndex].Entry};
        AssetBuilder->TextureTable.Insert(Pair.TextureInfo->Name, Pair);
    }
    
    ConsoleLog("Current asset file read successfully");    
}

ak_u32 CalculateConvexHullSize(convex_hull* ConvexHull)
{
    ak_u32 Result = 0;    
    Result += sizeof(ConvexHull->Header);    
    Result += (ConvexHull->Header.VertexCount*sizeof(half_vertex));
    Result += (ConvexHull->Header.EdgeCount*sizeof(half_edge));
    Result += (ConvexHull->Header.FaceCount*sizeof(half_face));
    
    return Result;
}

ak_u32 CalculateConvexHullsSize(convex_hull* ConvexHulls, ak_u32 ConvexHullCount)
{
    ak_u32 Result = 0;
    for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < ConvexHullCount; ConvexHullIndex++)    
        Result += CalculateConvexHullSize(ConvexHulls + ConvexHullIndex);    
    return Result;
}

ak_u32 CalculateMeshInfoSize(mesh_info* MeshInfo)
{
    ak_u32 Result = 0;
    Result += sizeof(mesh_info_header);
    Result += sizeof(ak_char)*MeshInfo->Header.NameLength;
    Result += CalculateConvexHullsSize(MeshInfo->ConvexHulls, MeshInfo->Header.ConvexHullCount);    
    return Result;
}

ak_u32 CalculateTotalMeshInfoSize(ak_link_list<mesh_info>* MeshInfos)
{
    ak_u32 Result = 0;
    AK_ForEach(MeshInfo, MeshInfos)
        Result += CalculateMeshInfoSize(MeshInfo);
    return Result;
}


ak_u32 CalculateTextureInfoSize(texture_info* TextureInfo)
{
    ak_u32 Result = 0;
    Result += sizeof(texture_info_header);
    Result += sizeof(ak_char)*TextureInfo->Header.NameLength;
    return Result;
}

ak_u32 CalculateTotalTextureInfoSize(ak_link_list<texture_info>* TextureInfos)
{
    ak_u32 Result = 0;
    AK_ForEach(TextureInfo, TextureInfos)
        Result += CalculateTextureInfoSize(TextureInfo);
    return Result;
}

ak_u32 CalculateOffsetToData(asset_builder* AssetBuilder)
{
    ak_u32 Result = 0;
    Result += sizeof(asset_header);
    Result += CalculateTotalMeshInfoSize(&AssetBuilder->MeshInfos);
    Result += CalculateTotalTextureInfoSize(&AssetBuilder->TextureInfos);
    return Result;
}

void WriteConvexHull(ak_file_handle* FileHandle, convex_hull* ConvexHull)
{        
    AK_WriteFile(FileHandle, &ConvexHull->Header, sizeof(ConvexHull->Header));
    AK_WriteFile(FileHandle, ConvexHull->Vertices, sizeof(half_vertex)*ConvexHull->Header.VertexCount);
    AK_WriteFile(FileHandle, ConvexHull->Edges, sizeof(half_edge)*ConvexHull->Header.EdgeCount);
    AK_WriteFile(FileHandle, ConvexHull->Faces, sizeof(half_face)*ConvexHull->Header.FaceCount);    
}

void WriteMeshInfo(ak_file_handle* FileHandle, mesh_info* MeshInfo)
{
    AK_WriteFile(FileHandle, &MeshInfo->Header, sizeof(MeshInfo->Header));
    AK_WriteFile(FileHandle, MeshInfo->Name, sizeof(ak_char)*MeshInfo->Header.NameLength);    
    for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
        WriteConvexHull(FileHandle, MeshInfo->ConvexHulls + ConvexHullIndex);    
}

void WriteMeshInfos(ak_file_handle* FileHandle, ak_link_list<mesh_info>* MeshInfos)
{
    AK_ForEach(MeshInfo, MeshInfos)
        WriteMeshInfo(FileHandle, MeshInfo);    
}

void WriteTextureInfo(ak_file_handle* FileHandle, texture_info* TextureInfo)
{
    AK_WriteFile(FileHandle, &TextureInfo->Header, sizeof(TextureInfo->Header));
    AK_WriteFile(FileHandle, TextureInfo->Name, sizeof(ak_char)*TextureInfo->Header.NameLength);    
}

void WriteTextureInfos(ak_file_handle* FileHandle, ak_link_list<texture_info>* TextureInfos)
{
    AK_ForEach(TextureInfo, TextureInfos)
        WriteTextureInfo(FileHandle, TextureInfo);
}

ak_bool ValidateOffset(ak_file_handle* FileHandle, ak_u64 Offset)
{
    ak_u64 OffsetToTest = AK_GetFilePointer(FileHandle);
    return OffsetToTest == Offset;
}

void WriteInfos(ak_file_handle* FileHandle, asset_builder* AssetBuilder)
{
    WriteMeshInfos(FileHandle, &AssetBuilder->MeshInfos);
    WriteTextureInfos(FileHandle, &AssetBuilder->TextureInfos);
}

void WriteMesh(ak_file_handle* FileHandle, mesh* Mesh, mesh_info* MeshInfo)
{
    ak_u32 MeshDataSize = GetMeshDataSize(MeshInfo);    
    AK_Assert(ValidateOffset(FileHandle, MeshInfo->Header.OffsetToData), "Asset file offsets are not correct. This is a programming error.");    
    AK_WriteFile(FileHandle, Mesh->Vertices, MeshDataSize);    
}

void WriteMeshData(ak_file_handle* FileHandle, ak_link_list<mesh>* Meshes, ak_link_list<mesh_info>* MeshInfos)
{
    ak_link_list_iter<mesh> MeshIterator = AK_BeginIter(Meshes);
    ak_link_list_iter<mesh_info> MeshInfoIterator = AK_BeginIter(MeshInfos);
    
    mesh* Mesh = MeshIterator.First();
    mesh_info* MeshInfo = MeshInfoIterator.First();    
    
    while(Mesh && MeshInfo)
    {        
        WriteMesh(FileHandle, Mesh, MeshInfo);        
        Mesh = MeshIterator.Next();
        MeshInfo = MeshInfoIterator.Next();              
    }    
}

void WriteTexture(ak_file_handle* FileHandle, texture* Texture, texture_info* TextureInfo)
{
    ak_u32 TextureDataSize = GetTextureDataSize(TextureInfo);    
    AK_Assert(ValidateOffset(FileHandle, TextureInfo->Header.OffsetToData), "Asset file offsets are not correct. This is a programming error.");    
    AK_WriteFile(FileHandle, Texture->Texels, TextureDataSize);    
}

void WriteTextureData(ak_file_handle* FileHandle, ak_link_list<texture>* Textures, ak_link_list<texture_info>* TextureInfos)
{
    ak_link_list_iter<texture> TextureIterator = AK_BeginIter(Textures);
    ak_link_list_iter<texture_info> TextureInfoIterator = AK_BeginIter(TextureInfos);
    
    texture* Texture = TextureIterator.First();
    texture_info* TextureInfo = TextureInfoIterator.First();    
    
    while(Texture && TextureInfo)
    {        
        WriteTexture(FileHandle, Texture, TextureInfo);
        Texture = TextureIterator.Next();
        TextureInfo = TextureInfoIterator.Next();
    }
}

void WriteData(ak_file_handle* FileHandle, asset_builder* AssetBuilder)
{
    WriteMeshData(FileHandle, &AssetBuilder->Meshes, &AssetBuilder->MeshInfos);
    WriteTextureData(FileHandle, &AssetBuilder->Textures, &AssetBuilder->TextureInfos);
}

void WriteAssets(asset_builder* AssetBuilder, ak_string AssetPath, ak_string AssetHeaderPath)
{    
    ak_u64 OffsetToData = CalculateOffsetToData(AssetBuilder);        
    AK_ForEach(MeshInfo, &AssetBuilder->MeshInfos)
    {        
        MeshInfo->Header.OffsetToData = OffsetToData;        
        OffsetToData += GetMeshDataSize(MeshInfo);
    }
    
    AK_ForEach(TextureInfo, &AssetBuilder->TextureInfos)
    {
        TextureInfo->Header.OffsetToData = OffsetToData;
        OffsetToData += GetTextureDataSize(TextureInfo);
    }
    
    ak_arena* GlobalArena = AK_GetGlobalArena();
    
    ak_string_builder HeaderBuilder = {};
    
    HeaderBuilder.WriteLine("#ifndef ASSET_HEADER_H");
    HeaderBuilder.WriteLine("#define ASSET_HEADER_H");
    
    //TODO(JJ): Should put more information in this generated comment like
    //1. Date it was generated
    //2. User that generated it
    HeaderBuilder.WriteLine("//NOTE(EVERYONE): This header file was generated by the AssetBuilder");        
    
    HeaderBuilder.NewLine();        
    
    HeaderBuilder.WriteLine("#define INVALID_MESH_ID    ((mesh_asset_id)-1)");
    HeaderBuilder.WriteLine("#define INVALID_TEXTURE_ID ((texture_asset_id)-1)");    
    
    HeaderBuilder.NewLine();
    
    {
        HeaderBuilder.WriteLine("enum mesh_asset_id");
        HeaderBuilder.WriteLine("{");        
        AK_ForEach(MeshInfo, &AssetBuilder->MeshInfos)
        {            
            ak_temp_arena TempArena = GlobalArena->BeginTemp();
            HeaderBuilder.WriteLine("\tMESH_ASSET_ID_%s,", AK_ToUpper(MeshInfo->Name, GlobalArena));
            GlobalArena->EndTemp(&TempArena);
        }        
        HeaderBuilder.WriteLine("\tMESH_ASSET_COUNT");        
        HeaderBuilder.WriteLine("};");
    }
    
    HeaderBuilder.NewLine();
    
    {
        HeaderBuilder.WriteLine("enum texture_asset_id");
        HeaderBuilder.WriteLine("{");
        AK_ForEach(TextureInfo, &AssetBuilder->TextureInfos)
        {
            ak_temp_arena TempArena = GlobalArena->BeginTemp();
            HeaderBuilder.WriteLine("\tTEXTURE_ASSET_ID_%s,", AK_ToUpper(TextureInfo->Name, GlobalArena));            
            GlobalArena->EndTemp(&TempArena);
        }
        HeaderBuilder.WriteLine("\tTEXTURE_ASSET_COUNT");
        HeaderBuilder.WriteLine("};");
    }
    
    HeaderBuilder.NewLine();
    
    HeaderBuilder.WriteLine("#endif");
    
    ConsoleLog("Started opening asset files %s and %s", AssetHeaderPath.Data, AssetPath.Data);
    
    file_raii AssetFileRAII(AssetPath.Data, AK_FILE_ATTRIBUTES_WRITE);
    file_raii AssetHeaderFileRAII(AssetHeaderPath.Data, AK_FILE_ATTRIBUTES_WRITE);
    if(!AssetHeaderFileRAII.File)
    {
        ConsoleError("Could not open the asset header file %s for writing.", AssetHeaderPath.Data);
        return;
    }
    
    if(!AssetFileRAII.File)
    {
        ConsoleError("Could not open the asset file %s for writing.", AssetPath.Data);
        return;
    }
    
    ConsoleLog("Finished opening asset files");
    ConsoleNewLine();
    
    ConsoleLog("Writing asset header file at %s", AssetHeaderPath.Data);    
        
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_string AssetHeaderFileString = HeaderBuilder.PushString(GlobalArena);
    AK_WriteFile(AssetHeaderFileRAII.File, AssetHeaderFileString.Data, AssetHeaderFileString.Length);
    
    GlobalArena->EndTemp(&TempArena);
    
    ConsoleLog("Finished writing asset header file.");
    ConsoleNewLine();
    
    
    ConsoleLog("Writing asset file %s version %d.%d", AssetPath.Data, ASSET_MAJOR_VERSION, ASSET_MINOR_VERSION);
    
    
    asset_header Header = {};    
    AK_CopyArray(Header.Signature, ASSET_SIGNATURE, AK_Count(Header.Signature));    
    Header.MajorVersion = ASSET_MAJOR_VERSION;
    Header.MinorVersion = ASSET_MINOR_VERSION;
    AK_Assert(AssetBuilder->MeshInfos.Size == AssetBuilder->Meshes.Size, "Mesh and mesh info count do not match. This is a programming error.");
    Header.MeshCount = AssetBuilder->MeshInfos.Size;
    Header.TextureCount = AssetBuilder->TextureInfos.Size;
    
    AK_WriteFile(AssetFileRAII.File, &Header, sizeof(Header));    
    WriteInfos(AssetFileRAII.File, AssetBuilder);
    WriteData(AssetFileRAII.File, AssetBuilder);
    
    ConsoleLog("Finished writing asset file");
    ConsoleNewLine();
    
    HeaderBuilder.ReleaseMemory();    
}

int main(ak_i32 ArgCount, ak_char** Args)
{     
    command_line CommandLine = {};
    if(ArgCount < 2)
    {
        ConsoleLog("No arguments were specified.");
        ConsoleNewLine();
        CommandLine.DisplayHelp();
        return 0;
    }
    
    AK_SetGlobalArena(AK_CreateArena(AK_Megabyte(4)));
    
    CommandLine.Parse(Args, ArgCount);
    if(CommandLine.IsHelpPresent)
    {
        CommandLine.DisplayHelp();
    }
    else
    {
        if((CommandLine.Arguments[COMMAND_ARGUMENT_TYPE_CREATE].Size == 0) &&
           (CommandLine.Arguments[COMMAND_ARGUMENT_TYPE_DELETE].Size == 0))
        {
            ConsoleLog("You are not deleting or creating any assets");
            ConsoleNewLine();
            CommandLine.DisplayHelp();
            return 0;
        }                
        
        asset_builder AssetBuilder = {};
        AssetBuilder.AssetArena = AK_CreateArena(AK_Megabyte(128));        
        
        ak_string ProgramPath = AK_GetExecutablePath(AssetBuilder.AssetArena);
        ak_string AssetPath = AK_StringConcat(ProgramPath, "WorldGame.assets", AssetBuilder.AssetArena);    
        ak_string AssetHeaderPath = AK_StringConcat(AK_OffsetLength(ProgramPath, -1), 
                                                    AK_FormatString(AssetBuilder.AssetArena, "%c..%ccode%cassets%casset_header.h", 
                                                                    AK_OS_PATH_DELIMITER, AK_OS_PATH_DELIMITER, AK_OS_PATH_DELIMITER, AK_OS_PATH_DELIMITER), 
                                                    AssetBuilder.AssetArena);
        
        AssetBuilder.MeshTable = AK_CreateHashMap<ak_char*, mesh_pair>(8191);
        AssetBuilder.TextureTable = AK_CreateHashMap<ak_char*, texture_pair>(8191);
        
        if(AK_FileExists(AssetPath))
            ReadAssets(&AssetBuilder, AssetPath);
        
        DeleteAssets(&AssetBuilder, CommandLine.Arguments[COMMAND_ARGUMENT_TYPE_DELETE]);
        CreateAssets(&AssetBuilder, CommandLine.Arguments[COMMAND_ARGUMENT_TYPE_CREATE]);
        
        if(AK_FileExists(AssetPath))
        {
            ak_string BackupAssetPath = AK_StringConcat(ProgramPath, "WorldGame_Backup.assets", AssetBuilder.AssetArena);
            if(AK_FileExists(BackupAssetPath))
                AK_FileRemove(BackupAssetPath);                                                
            AK_FileRename(AssetPath, BackupAssetPath);            
        }
        
        if(AK_FileExists(AssetHeaderPath))
        {
            ak_string BackupHeaderPath = AK_StringConcat(AK_OffsetLength(ProgramPath, -1), 
                                                         AK_FormatString(AssetBuilder.AssetArena, "%c..%ccode%cassets%casset_header_Backup.h", 
                                                                         AK_OS_PATH_DELIMITER, AK_OS_PATH_DELIMITER, AK_OS_PATH_DELIMITER, AK_OS_PATH_DELIMITER), 
                                                         AssetBuilder.AssetArena);
            if(AK_FileExists(BackupHeaderPath))
                AK_FileRemove(BackupHeaderPath);
            
            AK_FileRename(AssetHeaderPath, BackupHeaderPath);            
        }
        
        WriteAssets(&AssetBuilder, AssetPath, AssetHeaderPath);
        
        ConsoleLog("Finished building assets");
    }
    
    return 0;
}