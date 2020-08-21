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
    platform_file_handle File;    
    
    inline file_raii(char* Path, platform_file_attributes Attributes)
    {
        File = FileOpen(Path, Attributes);        
    }
    
    inline ~file_raii()
    {
        FileClose(&File);        
    }
};

inline b32 IsNextArgument(char* Parameter)
{
    if(Parameter[0] == '-')
        return true;
    
    return false;
}

struct command_line
{
    b32 IsHelpPresent;
    dynamic_array<string> Arguments[COMMAND_ARGUMENT_COUNT];
    
    void HandleArgument(command_argument_type Type, char** Args, i32 ArgIndex, i32 ArgCount)
    {
        for(i32 ParameterIndex = ArgIndex+1; ParameterIndex < ArgCount; ParameterIndex++)
        {
            char* Parameter = Args[ParameterIndex];
            if(IsNextArgument(Parameter))
                break;
            
            if(!IsInitialized(&Arguments[Type]))
                Arguments[Type] = CreateDynamicArray<string>();
            
            Append(&Arguments[Type], LiteralString(Parameter));
        }
    }
    
    void Parse(char** Args, i32 ArgCount)
    {
        for(i32 ArgIndex = 0; ArgIndex < ArgCount; ArgIndex++)
        {
            char* Argument = Args[ArgIndex];
            if(StringEquals(Argument, "-?"))
                IsHelpPresent = true;
            else if(StringEquals(Argument, "-c"))
                HandleArgument(COMMAND_ARGUMENT_TYPE_CREATE, Args, ArgIndex, ArgCount);
            else if(StringEquals(Argument, "-d"))
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

void ParseFile(asset_builder* AssetBuilder, string File, string Ext)
{
    ConsoleLog("-------------------------");
    ConsoleLog("Started Parsing File %s", File.Data);
    
    if(StringEquals(Ext, "fbx"))            
        ParseFBX(AssetBuilder, File);        
    else if(StringEquals(Ext, "png"))
        ParsePNG(AssetBuilder, File);
    else
        ConsoleError("Unknown file extension %s", Ext.Data);
    
    ConsoleLog("Ended Parsing File %s", File.Data);                
}

void CreateAssets(asset_builder* AssetBuilder, dynamic_array<string>& CreateParameters)
{
    ConsoleLog("Creating assets");
    ConsoleNewLine();
    for(u32 FileIndex = 0; FileIndex < CreateParameters.Size; FileIndex++)
    {
        string FileOrDirectory = CreateParameters[FileIndex];
        
        string Ext = GetFileExtension(FileOrDirectory);
        if(IsInvalidString(Ext))
        {   
            directory_result Results = GetAllFilesInDirectory(FileOrDirectory);
            FOR_EACH(File, &Results)
            {
                Ext = GetFileExtension(*File);
                ParseFile(AssetBuilder, *File, Ext);
            }            
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

void DeleteAssets(asset_builder* AssetBuilder, dynamic_array<string>& DeleteParameters)
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
        
        for(u32 FileIndex = 0; FileIndex < DeleteParameters.Size; FileIndex += 2)
        {        
            string Type = ToLower(DeleteParameters[FileIndex]);
            string Name = DeleteParameters[FileIndex+1];
            
            if(StringEquals(Type, "mesh"))
            {
                ConsoleLog("Deleting mesh %s", Name.Data);
                
                mesh_pair Pair;
                if(AssetBuilder->MeshTable.Find(Name.Data, &Pair))
                {                
                    list_entry<mesh_info>* OldMeshInfo = (list_entry<mesh_info>*)Pair.MeshInfo;
                    list_entry<mesh>* OldMesh = (list_entry<mesh>*)Pair.Mesh;
                    
                    RemoveFromList(&AssetBuilder->MeshInfos, OldMeshInfo);
                    RemoveFromList(&AssetBuilder->Meshes, OldMesh);                                
                }
                else
                {
                    ConsoleError("Could not find mesh to delete");
                }            
            }
            else if(StringEquals(Type, "texture"))
            {
                ConsoleLog("Deleting texture %s", Name.Data);
                
                texture_pair Pair;
                if(AssetBuilder->TextureTable.Find(Name.Data, &Pair))
                {
                    list_entry<texture_info>* OldTextureInfo = (list_entry<texture_info>*)Pair.TextureInfo;
                    list_entry<texture>* OldTexture = (list_entry<texture>*)Pair.Texture;
                    
                    RemoveFromList(&AssetBuilder->TextureInfos, OldTextureInfo);
                    RemoveFromList(&AssetBuilder->Textures, OldTexture);
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
        }
        
        ConsoleLog("Finished deleting assets");
        ConsoleNewLine();    
    }
}

void ReadMeshInfo(asset_builder* AssetBuilder, mesh_info* MeshInfo, platform_file_handle* File)
{
    FileRead(File, &MeshInfo->Header, sizeof(mesh_info_header));    
    MeshInfo->Name = PushArray(&AssetBuilder->AssetArena, MeshInfo->Header.NameLength+1, char, Clear, 0);
    MeshInfo->ConvexHulls = PushArray(&AssetBuilder->AssetArena, MeshInfo->Header.ConvexHullCount, convex_hull, Clear, 0);
    
    FileRead(File, MeshInfo->Name, sizeof(char)*MeshInfo->Header.NameLength);    
    MeshInfo->Name[MeshInfo->Header.NameLength] = 0;
    
    for(u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
    {
        convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
        FileRead(File, &ConvexHull->Header, sizeof(ConvexHull->Header));
        
        u32 ConvexHullDataSize = 0;
        ConvexHullDataSize += (ConvexHull->Header.VertexCount*sizeof(half_vertex));                
        ConvexHullDataSize += (ConvexHull->Header.EdgeCount*sizeof(half_edge));
        ConvexHullDataSize += (ConvexHull->Header.FaceCount*sizeof(half_face));
        
        void* ConvexHullData = PushSize(&AssetBuilder->AssetArena, ConvexHullDataSize, Clear, 0);
        FileRead(File, ConvexHullData, ConvexHullDataSize);
        
        ConvexHull->Vertices = (half_vertex*)ConvexHullData;
        ConvexHull->Edges = (half_edge*)(ConvexHull->Vertices + ConvexHull->Header.VertexCount);
        ConvexHull->Faces = (half_face*)(ConvexHull->Edges + ConvexHull->Header.EdgeCount);
    }    
}

void ReadMeshInfos(asset_builder* AssetBuilder, list_entry<mesh_info>* MeshInfos, u32 MeshCount, platform_file_handle* File)
{
    for(u32 MeshIndex = 0; MeshIndex < MeshCount; MeshIndex++)
    {
        ReadMeshInfo(AssetBuilder, &MeshInfos[MeshIndex].Entry, File);        
        AddToList(&AssetBuilder->MeshInfos, &MeshInfos[MeshIndex]);        
    }
}

void ReadTextureInfo(asset_builder* AssetBuilder, texture_info* TextureInfo, platform_file_handle* File)
{
    FileRead(File, &TextureInfo->Header, sizeof(texture_info_header));    
    TextureInfo->Name = PushArray(&AssetBuilder->AssetArena, TextureInfo->Header.NameLength+1, char, Clear, 0);
    FileRead(File, TextureInfo->Name, sizeof(char)*TextureInfo->Header.NameLength);    
    TextureInfo->Name[TextureInfo->Header.NameLength] = 0;            
}

void ReadTextureInfos(asset_builder* AssetBuilder, list_entry<texture_info>* TextureInfos, u32 TextureCount, platform_file_handle* File)
{
    for(u32 TextureIndex = 0; TextureIndex < TextureCount; TextureIndex++)
    {
        ReadTextureInfo(AssetBuilder, &TextureInfos[TextureIndex].Entry, File);
        AddToList(&AssetBuilder->TextureInfos, &TextureInfos[TextureIndex]);
    }
}

void ReadMesh(asset_builder* AssetBuilder, mesh* Mesh, mesh_info* MeshInfo, platform_file_handle* File)
{
    u32 MeshSize = GetMeshDataSize(MeshInfo);
    void* Data = PushSize(&AssetBuilder->AssetArena, MeshSize, Clear, 0);
    Mesh->Vertices = Data;
    Mesh->Indices = ((u8*)Mesh->Vertices + GetVertexStride(MeshInfo)*MeshInfo->Header.VertexCount);   
    FileRead(File, Data, MeshSize);    
}

void ReadMeshes(asset_builder* AssetBuilder, list_entry<mesh>* Meshes, list_entry<mesh_info>* MeshInfos, u32 MeshCount, platform_file_handle* File)
{
    for(u32 MeshIndex = 0; MeshIndex < MeshCount; MeshIndex++)
    {
        ReadMesh(AssetBuilder, &Meshes[MeshIndex].Entry, &MeshInfos[MeshIndex].Entry, File);
        AddToList(&AssetBuilder->Meshes, &Meshes[MeshIndex]);
    }
}

void ReadTexture(asset_builder* AssetBuilder, texture* Texture, texture_info* TextureInfo, platform_file_handle* File)
{
    u32 TextureSize = GetTextureDataSize(TextureInfo);
    void* Data = PushSize(&AssetBuilder->AssetArena, TextureSize, Clear, 0);
    Texture->Texels = Data;    
    FileRead(File, Data, TextureSize);    
}

void ReadTextures(asset_builder* AssetBuilder, list_entry<texture>* Textures, list_entry<texture_info>* TextureInfos, u32 TextureCount, platform_file_handle* File)
{
    for(u32 TextureIndex = 0; TextureIndex < TextureCount; TextureIndex++)
    {
        ReadTexture(AssetBuilder, &Textures[TextureIndex].Entry, &TextureInfos[TextureIndex].Entry, File);
        AddToList(&AssetBuilder->Textures, &Textures[TextureIndex]);
    }
}

void ReadAssets(asset_builder* AssetBuilder, string AssetPath)
{
    ConsoleLog("Reading current asset file");
    
    file_raii FileRAII(AssetPath.Data, PLATFORM_FILE_ATTRIBUTES_READ);    
    if(!FileRAII.File.IsValid())
    {
        ConsoleError("Could not open asset file for reading. Application will create a new one instead.");
        return;
    }
    
    asset_header Header = {};
    FileRead(&FileRAII.File, &Header, sizeof(Header));
    
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
    
    list_entry<mesh_info>* MeshInfos = PushArray(&AssetBuilder->AssetArena, Header.MeshCount, list_entry<mesh_info>, Clear, 0);
    list_entry<texture_info>* TextureInfos = PushArray(&AssetBuilder->AssetArena, Header.TextureCount, list_entry<texture_info>, Clear, 0);
    
    list_entry<mesh>* Meshes = PushArray(&AssetBuilder->AssetArena, Header.MeshCount, list_entry<mesh>, Clear, 0);
    list_entry<texture>* Textures = PushArray(&AssetBuilder->AssetArena, Header.TextureCount, list_entry<texture>, Clear, 0);
    
    ReadMeshInfos(AssetBuilder, MeshInfos, Header.MeshCount, &FileRAII.File);
    ReadTextureInfos(AssetBuilder, TextureInfos, Header.TextureCount, &FileRAII.File);
    
    ReadMeshes(AssetBuilder, Meshes, MeshInfos, Header.MeshCount, &FileRAII.File);
    ReadTextures(AssetBuilder, Textures, TextureInfos, Header.TextureCount, &FileRAII.File);
    
    for(u32 MeshIndex = 0; MeshIndex < Header.MeshCount; MeshIndex++)
    {
        mesh_pair Pair = {&MeshInfos[MeshIndex].Entry, &Meshes[MeshIndex].Entry};
        AssetBuilder->MeshTable.Insert(Pair.MeshInfo->Name, Pair);
    }
    
    for(u32 TextureIndex = 0; TextureIndex < Header.TextureCount; TextureIndex++)
    {
        texture_pair Pair = {&TextureInfos[TextureIndex].Entry, &Textures[TextureIndex].Entry};
        AssetBuilder->TextureTable.Insert(Pair.TextureInfo->Name, Pair);
    }
    
    ConsoleLog("Current asset file read successfully");    
}

ptr CalculateConvexHullSize(convex_hull* ConvexHull)
{
    ptr Result = 0;    
    Result += sizeof(ConvexHull->Header);    
    Result += (ConvexHull->Header.VertexCount*sizeof(half_vertex));
    Result += (ConvexHull->Header.EdgeCount*sizeof(half_edge));
    Result += (ConvexHull->Header.FaceCount*sizeof(half_face));
    
    return Result;
}

ptr CalculateConvexHullsSize(convex_hull* ConvexHulls, u32 ConvexHullCount)
{
    ptr Result = 0;
    for(u32 ConvexHullIndex = 0; ConvexHullIndex < ConvexHullCount; ConvexHullIndex++)    
        Result += CalculateConvexHullSize(ConvexHulls + ConvexHullIndex);    
    return Result;
}

ptr CalculateMeshInfoSize(mesh_info* MeshInfo)
{
    ptr Result = 0;
    Result += sizeof(mesh_info_header);
    Result += sizeof(char)*MeshInfo->Header.NameLength;
    Result += CalculateConvexHullsSize(MeshInfo->ConvexHulls, MeshInfo->Header.ConvexHullCount);    
    return Result;
}

ptr CalculateTotalMeshInfoSize(list<mesh_info>* MeshInfos)
{
    ptr Result = 0;
    FOR_EACH(MeshInfo, MeshInfos)
        Result += CalculateMeshInfoSize(MeshInfo);
    return Result;
}


ptr CalculateTextureInfoSize(texture_info* TextureInfo)
{
    ptr Result = 0;
    Result += sizeof(texture_info_header);
    Result += sizeof(char)*TextureInfo->Header.NameLength;
    return Result;
}

ptr CalculateTotalTextureInfoSize(list<texture_info>* TextureInfos)
{
    ptr Result = 0;
    FOR_EACH(TextureInfo, TextureInfos)
        Result += CalculateTextureInfoSize(TextureInfo);
    return Result;
}

ptr CalculateOffsetToData(asset_builder* AssetBuilder)
{
    ptr Result = 0;
    Result += sizeof(asset_header);
    Result += CalculateTotalMeshInfoSize(&AssetBuilder->MeshInfos);
    Result += CalculateTotalTextureInfoSize(&AssetBuilder->TextureInfos);
    return Result;
}

void WriteConvexHull(platform_file_handle* AssetFile, convex_hull* ConvexHull)
{
    FileWrite(AssetFile, &ConvexHull->Header, sizeof(ConvexHull->Header));
    FileWrite(AssetFile, ConvexHull->Vertices, sizeof(half_vertex)*ConvexHull->Header.VertexCount);
    FileWrite(AssetFile, ConvexHull->Edges, sizeof(half_edge)*ConvexHull->Header.EdgeCount);
    FileWrite(AssetFile, ConvexHull->Faces, sizeof(half_face)*ConvexHull->Header.FaceCount);    
}

void WriteMeshInfo(platform_file_handle* AssetFile, mesh_info* MeshInfo)
{
    FileWrite(AssetFile, &MeshInfo->Header, sizeof(MeshInfo->Header));
    FileWrite(AssetFile, MeshInfo->Name, sizeof(char)*MeshInfo->Header.NameLength);    
    for(u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
        WriteConvexHull(AssetFile, MeshInfo->ConvexHulls + ConvexHullIndex);    
}

void WriteMeshInfos(platform_file_handle* AssetFile, list<mesh_info>* MeshInfos)
{
    FOR_EACH(MeshInfo, MeshInfos)
        WriteMeshInfo(AssetFile, MeshInfo);    
}

void WriteTextureInfo(platform_file_handle* AssetFile, texture_info* TextureInfo)
{
    FileWrite(AssetFile, &TextureInfo->Header, sizeof(TextureInfo->Header));
    FileWrite(AssetFile, TextureInfo->Name, sizeof(char)*TextureInfo->Header.NameLength);    
}

void WriteTextureInfos(platform_file_handle* AssetFile, list<texture_info>* TextureInfos)
{
    FOR_EACH(TextureInfo, TextureInfos)
        WriteTextureInfo(AssetFile, TextureInfo);
}

b32 ValidateOffset(platform_file_handle* File, u64 Offset)
{
    u64 OffsetToTest = FileGetPointer(File);
    return OffsetToTest == Offset;
}

void WriteInfos(platform_file_handle* AssetFile, asset_builder* AssetBuilder)
{
    WriteMeshInfos(AssetFile, &AssetBuilder->MeshInfos);
    WriteTextureInfos(AssetFile, &AssetBuilder->TextureInfos);
}

void WriteMesh(platform_file_handle* AssetFile, mesh* Mesh, mesh_info* MeshInfo)
{
    u32 MeshDataSize = GetMeshDataSize(MeshInfo);
    
    ASSERT(ValidateOffset(AssetFile, MeshInfo->Header.OffsetToData));
    
    FileWrite(AssetFile, Mesh->Vertices, MeshDataSize);    
}

void WriteMeshData(platform_file_handle* AssetFile, list<mesh>* Meshes, list<mesh_info>* MeshInfos)
{
    list_iterator<mesh> MeshIterator = BeginIter(Meshes);
    list_iterator<mesh_info> MeshInfoIterator = BeginIter(MeshInfos);
    
    mesh* Mesh = GetFirst(&MeshIterator);
    mesh_info* MeshInfo = GetFirst(&MeshInfoIterator);
    
    while(Mesh && MeshInfo)
    {
        WriteMesh(AssetFile, Mesh, MeshInfo);        
        Mesh = GetNext(&MeshIterator);
        MeshInfo = GetNext(&MeshInfoIterator);
    }    
}

void WriteTexture(platform_file_handle* AssetFile, texture* Texture, texture_info* TextureInfo)
{
    u32 TextureDataSize = GetTextureDataSize(TextureInfo);    
    ASSERT(ValidateOffset(AssetFile, TextureInfo->Header.OffsetToData));    
    FileWrite(AssetFile, Texture->Texels, TextureDataSize);    
}

void WriteTextureData(platform_file_handle* AssetFile, list<texture>* Textures, list<texture_info>* TextureInfos)
{
    list_iterator<texture> TextureIterator = BeginIter(Textures);
    list_iterator<texture_info> TextureInfoIterator = BeginIter(TextureInfos);
    
    texture* Texture = GetFirst(&TextureIterator);
    texture_info* TextureInfo = GetFirst(&TextureInfoIterator);
    
    while(Texture && TextureInfo)
    {
        WriteTexture(AssetFile, Texture, TextureInfo);
        Texture = GetNext(&TextureIterator);
        TextureInfo = GetNext(&TextureInfoIterator);
    }
}

void WriteData(platform_file_handle* AssetFile, asset_builder* AssetBuilder)
{
    WriteMeshData(AssetFile, &AssetBuilder->Meshes, &AssetBuilder->MeshInfos);
    WriteTextureData(AssetFile, &AssetBuilder->Textures, &AssetBuilder->TextureInfos);
}

void WriteAssets(asset_builder* AssetBuilder, string AssetPath, string AssetHeaderPath)
{
    temp_arena TempArena = BeginTemporaryMemory();
    
    u64 OffsetToData = CalculateOffsetToData(AssetBuilder);        
    FOR_EACH(MeshInfo, &AssetBuilder->MeshInfos)
    {        
        MeshInfo->Header.OffsetToData = OffsetToData;        
        OffsetToData += GetMeshDataSize(MeshInfo);
    }
    
    FOR_EACH(TextureInfo, &AssetBuilder->TextureInfos)
    {
        TextureInfo->Header.OffsetToData = OffsetToData;
        OffsetToData += GetTextureDataSize(TextureInfo);
    }
    
    string_stream HeaderBuilder = CreateStringStream(MEGABYTE(1));
    
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
        FOR_EACH(MeshInfo, &AssetBuilder->MeshInfos)
        {
            HeaderBuilder.WriteLine("\tMESH_ASSET_ID_%s,", ToUpper(MeshInfo->Name));
        }        
        HeaderBuilder.WriteLine("\tMESH_ASSET_COUNT");        
        HeaderBuilder.WriteLine("};");
    }
    
    HeaderBuilder.NewLine();
    
    {
        HeaderBuilder.WriteLine("enum texture_asset_id");
        HeaderBuilder.WriteLine("{");
        FOR_EACH(TextureInfo, &AssetBuilder->TextureInfos)
        {
            HeaderBuilder.WriteLine("\tTEXTURE_ASSET_ID_%s,", ToUpper(TextureInfo->Name));            
        }
        HeaderBuilder.WriteLine("\tTEXTURE_ASSET_COUNT");
        HeaderBuilder.WriteLine("};");
    }
    
    HeaderBuilder.NewLine();
    
    HeaderBuilder.WriteLine("#endif");
    
    ConsoleLog("Started opening asset files %s and %s", AssetHeaderPath.Data, AssetPath.Data);
        
    file_raii AssetFileRAII(AssetPath.Data, PLATFORM_FILE_ATTRIBUTES_WRITE);
    file_raii AssetHeaderFileRAII(AssetHeaderPath.Data, PLATFORM_FILE_ATTRIBUTES_WRITE);
    if(!AssetHeaderFileRAII.File.IsValid())
    {
        ConsoleError("Could not open the asset header file %s for writing.", AssetHeaderPath.Data);
        return;
    }
    
    if(!AssetFileRAII.File.IsValid())
    {
        ConsoleError("Could not open the asset file %s for writing.", AssetPath.Data);
        return;
    }
    
    ConsoleLog("Finished opening asset files");
    ConsoleNewLine();
    
    ConsoleLog("Writing asset header file at %s", AssetHeaderPath.Data);    
    
    string AssetHeaderFileString = HeaderBuilder.GetString();
    FileWrite(&AssetHeaderFileRAII.File, AssetHeaderFileString.Data, (u32)AssetHeaderFileString.Length);
    
    ConsoleLog("Finished writing asset header file.");
    ConsoleNewLine();
    
    
    ConsoleLog("Writing asset file %s version %d.%d", AssetPath.Data, ASSET_MAJOR_VERSION, ASSET_MINOR_VERSION);
    
    
    asset_header Header = {};    
    ArrayCopy(Header.Signature, ASSET_SIGNATURE, char, ARRAYCOUNT(Header.Signature));
    Header.MajorVersion = ASSET_MAJOR_VERSION;
    Header.MinorVersion = ASSET_MINOR_VERSION;
    ASSERT(AssetBuilder->MeshInfos.Count == AssetBuilder->Meshes.Count);
    Header.MeshCount = AssetBuilder->MeshInfos.Count;
    Header.TextureCount = AssetBuilder->TextureInfos.Count;
    
    FileWrite(&AssetFileRAII.File, &Header, sizeof(Header));    
    WriteInfos(&AssetFileRAII.File, AssetBuilder);
    WriteData(&AssetFileRAII.File, AssetBuilder);
        
    ConsoleLog("Finished writing asset file");
    ConsoleNewLine();
    
    DeleteStringStream(&HeaderBuilder);
    
    EndTemporaryMemory(&TempArena);
}

int main(i32 ArgCount, char** Args)
{ 
    arena TemporaryArena = CreateArena(MEGABYTE(32));
    InitMemory(&TemporaryArena, AllocateMemory, FreeMemory);
    
    command_line CommandLine = {};
    if(ArgCount < 2)
    {
        ConsoleLog("No arguments were specified.");
        ConsoleNewLine();
        CommandLine.DisplayHelp();
        return 0;
    }
    
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
        
        string ProgramPath = GetProgramPath(&TemporaryArena);
        string AssetPath = Concat(ProgramPath, "WorldGame.assets");    
        string AssetHeaderPath = Concat(ProgramPath-1, FormatString("%c..%ccode%cassets%casset_header.h", 
                                                                    OS_PATH_DELIMITER, OS_PATH_DELIMITER, OS_PATH_DELIMITER, OS_PATH_DELIMITER));
        
        asset_builder AssetBuilder = {};
        AssetBuilder.AssetArena = CreateArena(MEGABYTE(128));        
                
        AssetBuilder.MeshTable = CreateHashMap<char*, mesh_pair>(8191, StringEquals, &AssetBuilder.AssetArena);
        AssetBuilder.TextureTable = CreateHashMap<char*, texture_pair>(8191, StringEquals, &AssetBuilder.AssetArena);
        
        if(FileExists(AssetPath))
            ReadAssets(&AssetBuilder, AssetPath);
        
        DeleteAssets(&AssetBuilder, CommandLine.Arguments[COMMAND_ARGUMENT_TYPE_DELETE]);
        CreateAssets(&AssetBuilder, CommandLine.Arguments[COMMAND_ARGUMENT_TYPE_CREATE]);
        
        if(FileExists(AssetPath))
        {
            string BackupAssetPath = Concat(ProgramPath, "WorldGame_Backup.assets");
            if(FileExists(BackupAssetPath))
                FileRemove(BackupAssetPath);                                                
            FileRename(AssetPath, BackupAssetPath);            
        }
        
        if(FileExists(AssetHeaderPath))
        {
            string BackupHeaderPath = Concat(ProgramPath-1, FormatString("%c..%ccode%cassets%casset_header_Backup.h", 
                                                                         OS_PATH_DELIMITER, OS_PATH_DELIMITER, OS_PATH_DELIMITER, OS_PATH_DELIMITER));
            if(FileExists(BackupHeaderPath))
                FileRemove(BackupHeaderPath);
            
            FileRename(AssetHeaderPath, BackupHeaderPath);            
        }
        
        WriteAssets(&AssetBuilder, AssetPath, AssetHeaderPath);
        
        ConsoleLog("Finished building assets");
    }
    
    return 0;
}