#include "asset_builder.h"
#include "asset_builder_fbx.cpp"
#include "asset_builder_png.cpp"

enum command_argument_type
{    
    COMMAND_ARGUMENT_TYPE_CREATE,
    COMMAND_ARGUMENT_TYPE_DELETE,
    COMMAND_ARGUMENT_COUNT
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

void CreateAssets(asset_builder* AssetBuilder, dynamic_array<string>& CreateParameters)
{
    ConsoleLog("Creating assets");
    ConsoleNewLine();
    for(u32 FileIndex = 0; FileIndex < CreateParameters.Size; FileIndex++)
    {
        string File = CreateParameters[FileIndex];
        
        ConsoleLog("-------------------------");
        ConsoleLog("Started Parsing File %s", File.Data);
        
        string Ext = GetFileExtension(File);
        if(StringEquals(Ext, "fbx"))            
            ParseFBX(AssetBuilder, File);        
        else if(StringEquals(Ext, "png"))
            ParsePNG(AssetBuilder, File);
        else
            ConsoleError("Unknown file extension %s", Ext.Data);
        
        ConsoleLog("Ended Parsing File %s", File.Data);                
        
        if(FileIndex == (CreateParameters.Size - 1))
            ConsoleLog("-------------------------");
    }   
    ConsoleNewLine();
    ConsoleLog("Finished creating assets");
    ConsoleNewLine();
}

void DeleteAssets(asset_builder* AssetBuilder, dynamic_array<string>* DeleteParameters)
{
}

void ReadAssets(asset_builder* AssetBuilder, string AssetPath)
{
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

void WriteConvexHull(FILE* AssetFile, convex_hull* ConvexHull)
{
    fwrite(&ConvexHull->Header,  sizeof(ConvexHull->Header), 1, AssetFile);
    fwrite(ConvexHull->Vertices, sizeof(half_vertex), ConvexHull->Header.VertexCount, AssetFile);
    fwrite(ConvexHull->Edges,    sizeof(half_edge),   ConvexHull->Header.EdgeCount,   AssetFile);
    fwrite(ConvexHull->Faces,    sizeof(half_face),   ConvexHull->Header.FaceCount,   AssetFile);    
}

void WriteMeshInfo(FILE* AssetFile, mesh_info* MeshInfo)
{
    fwrite(&MeshInfo->Header, sizeof(MeshInfo->Header), 1, AssetFile);
    fwrite(MeshInfo->Name, sizeof(char), MeshInfo->Header.NameLength, AssetFile);
    for(u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
        WriteConvexHull(AssetFile, MeshInfo->ConvexHulls + ConvexHullIndex);    
}

void WriteMeshInfos(FILE* AssetFile, list<mesh_info>* MeshInfos)
{
    FOR_EACH(MeshInfo, MeshInfos)
        WriteMeshInfo(AssetFile, MeshInfo);    
}

void WriteTextureInfo(FILE* AssetFile, texture_info* TextureInfo)
{
    fwrite(&TextureInfo->Header, sizeof(TextureInfo->Header), 1, AssetFile);
    fwrite(TextureInfo->Name, sizeof(char), TextureInfo->Header.NameLength, AssetFile);
}

void WriteTextureInfos(FILE* AssetFile, list<texture_info>* TextureInfos)
{
    FOR_EACH(TextureInfo, TextureInfos)
        WriteTextureInfo(AssetFile, TextureInfo);
}

void WriteInfos(FILE* AssetFile, asset_builder* AssetBuilder)
{
    WriteMeshInfos(AssetFile, &AssetBuilder->MeshInfos);
    WriteTextureInfos(AssetFile, &AssetBuilder->TextureInfos);
}

void WriteMesh(FILE* AssetFile, mesh* Mesh, mesh_info* MeshInfo)
{
    ptr MeshDataSize = GetMeshDataSize(MeshInfo);
    fwrite(Mesh->Vertices, MeshDataSize, 1, AssetFile);
}

void WriteMeshData(FILE* AssetFile, list<mesh>* Meshes, list<mesh_info>* MeshInfos)
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

void WriteTexture(FILE* AssetFile, texture* Texture, texture_info* TextureInfo)
{
    ptr TextureDataSize = GetTextureDataSize(TextureInfo);
    fwrite(Texture->Texels, TextureDataSize, 1, AssetFile);
}

void WriteTextureData(FILE* AssetFile, list<texture>* Textures, list<texture_info>* TextureInfos)
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

void WriteData(FILE* AssetFile, asset_builder* AssetBuilder)
{
    WriteMeshData(AssetFile, &AssetBuilder->Meshes, &AssetBuilder->MeshInfos);
    WriteTextureData(AssetFile, &AssetBuilder->Textures, &AssetBuilder->TextureInfos);
}

void WriteAssets(asset_builder* AssetBuilder, string AssetPath, string AssetHeaderPath)
{
    temp_arena TempArena = BeginTemporaryMemory();
    
    ptr OffsetToData = CalculateOffsetToData(AssetBuilder);        
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
    
    FILE* AssetFile = fopen(AssetPath.Data, "wb");
    FILE* AssetHeaderFile = fopen(AssetHeaderPath.Data, "w");    
    if(!AssetHeaderFile)
    {
        ConsoleError("Could not open the asset header file %s for writing.", AssetHeaderPath.Data);
        return;
    }
    
    if(!AssetFile)
    {
        ConsoleError("Could not open the asset file %s for writing.", AssetPath.Data);
        return;
    }
    
    ConsoleLog("Finished opening asset files");
    ConsoleNewLine();
    
    ConsoleLog("Writing asset header file at %s", AssetHeaderPath.Data);    
    
    string AssetHeaderFileString = HeaderBuilder.GetString();
    fwrite(AssetHeaderFileString.Data, AssetHeaderFileString.Length, 1, AssetHeaderFile);
    
    fclose(AssetHeaderFile);
    ConsoleLog("Finished writing asset header file.");
    ConsoleNewLine();
    
    
    ConsoleLog("Writing asset file %s version %d.%d", AssetPath.Data, ASSET_MAJOR_VERSION, ASSET_MINOR_VERSION);
    
    
    asset_header Header = {};    
    ArrayCopy(Header.Signature, ASSET_SIGNATURE, char, ARRAYCOUNT(Header.Signature));
    Header.MajorVersion = ASSET_MAJOR_VERSION;
    Header.MinorVersion = ASSET_MINOR_VERSION;
    ASSERT(AssetBuilder->MeshInfos.Count == AssetBuilder->Meshes.Count);
    Header.MeshCount = AssetBuilder->MeshInfos.Count;
    
    fwrite(&Header, sizeof(Header), 1, AssetFile);    
    WriteInfos(AssetFile, AssetBuilder);
    WriteData(AssetFile, AssetBuilder);
    
    fclose(AssetFile);
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