struct vertex_p3_n3_index
{
    vertex_p3_n3 Vertex;
    u32 Index;
};

inline u64 Hash(vertex_p3_n3_index Data, u64 TableSize)
{
    u64 Result = Hash(Data.Vertex, TableSize);    
    return Result;
}

inline b32 operator!=(vertex_p3_n3_index Left, vertex_p3_n3_index Right)
{
    b32 Result = Left.Vertex != Right.Vertex;    
    return Result;
}

global FbxManager* Global_FBXManager;

mesh FBX_LoadFirstMesh(const char* Path)
{
    if(!Global_FBXManager)
    {
        Global_FBXManager = FbxManager::Create();
        
        FbxIOSettings* IOSettings = FbxIOSettings::Create(Global_FBXManager, IOSROOT);
        Global_FBXManager->SetIOSettings(IOSettings);
    }
    
    FbxImporter* Importer = FbxImporter::Create(Global_FBXManager, "");
    
    CONSOLE_LOG("Loading FBX File %s\n", Path);
    
    BOOL_CHECK_AND_HANDLE(Importer->Initialize(Path, -1, Global_FBXManager->GetIOSettings()),
                          "Failed to load fbx file %s message: %s\n", Path, Importer->GetStatus().GetErrorString()); 
    
    i32 MajorVersion, MinorVersion, RevisionVersion;
    Importer->GetFileVersion(MajorVersion, MinorVersion, RevisionVersion);
    
    CONSOLE_LOG("\tVersion of FBX File %d.%d.%d\n", MajorVersion, MinorVersion, RevisionVersion);
    
    FbxScene* Scene = FbxScene::Create(Global_FBXManager, "");
    
    Importer->Import(Scene);
    
    Importer->Destroy();
    
    FbxNode* RootNode = Scene->GetRootNode();
    BOOL_CHECK_AND_HANDLE(RootNode, "Scene does not have a root node. Cannot import any mesh.");
    
    if(RootNode)
    {        
        for(i32 NodeIndex = 0; NodeIndex < RootNode->GetChildCount(); NodeIndex++)
        {
            FbxNode* Node = RootNode->GetChild(NodeIndex);            
            
            v3f Translation = V3(Node->LclTranslation.Get().Buffer());
            v3f Euler = V3(Node->LclRotation.Get().Buffer());
            v3f Scale = V3(Node->LclScaling.Get().Buffer());
            
            sqt Transform = CreateSQT(Translation, Scale, Euler);
            
            for(i32 AttributeIndex = 0; AttributeIndex < Node->GetNodeAttributeCount(); AttributeIndex++)
            {
                FbxNodeAttribute* Attribute = Node->GetNodeAttributeByIndex(AttributeIndex);                
                switch(Attribute->GetAttributeType())
                {
                    case FbxNodeAttribute::eMesh:
                    {
                        FbxMesh* Mesh = Node->GetMesh();
                        
                        FbxGeometryElementNormal* ElementNormals = Mesh->GetElementNormal(0);
                        BOOL_CHECK_AND_HANDLE(ElementNormals, "Mesh did not have any normals.");                        
                        
                        BOOL_CHECK_AND_HANDLE(ElementNormals->GetMappingMode() == FbxGeometryElement::eByPolygonVertex,
                                              "Mapping mode for normals is not supported. Must be by polygon vertex.");                                                                              
                        
                        FbxLayerElement::EReferenceMode ReferenceMode = ElementNormals->GetReferenceMode();
                        BOOL_CHECK_AND_HANDLE((ReferenceMode == FbxGeometryElement::eDirect) || (ReferenceMode == FbxGeometryElement::eIndexToDirect),
                                              "Reference mode for normals is not supported. Must be direct or index to direct.");
                        
                        u32 PolygonCount = Mesh->GetPolygonCount();
                                                
                        u32 IndexCount = PolygonCount*3;
                        u32 VertexCount = 0;
                        
                        vertex_p3_n3* VertexData = PushArray(IndexCount, vertex_p3_n3, Clear, 0);
                        u32* IndexData = PushArray(IndexCount, u32, Clear, 0);
                        
                        hash_table<vertex_p3_n3_index> HashTable = CreateHashTable<vertex_p3_n3_index>(8101);     
                        
                        FbxVector4* ControlPoints = Mesh->GetControlPoints();
                        
                        for(u32 PolygonIndex = 0; PolygonIndex < PolygonCount; PolygonIndex++)
                        {
                            BOOL_CHECK_AND_HANDLE(Mesh->GetPolygonSize(PolygonIndex) == 3, "Mesh had a polygon that did not have 3 vertices. All polygons must be triangles.");
                            
                            for(u32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
                            {
                                u32 VertexId = (PolygonIndex*3) + VertexIndex;
                                
                                i32 ControlPointIndex = Mesh->GetPolygonVertex(PolygonIndex, VertexIndex);
                                i32 NormalIndex = -1;
                                
                                if(ReferenceMode == FbxGeometryElement::eDirect)
                                {                    
                                    NormalIndex = VertexId;
                                }
                                else if(ReferenceMode == FbxGeometryElement::eIndexToDirect)
                                {
                                    NormalIndex = ElementNormals->GetIndexArray().GetAt(VertexId);
                                }
                                INVALID_ELSE;
                                
                                vertex_p3_n3 Vertex = 
                                {
                                    V3(ControlPoints[ControlPointIndex].Buffer()),
                                    V3(ElementNormals->GetDirectArray().GetAt(NormalIndex).Buffer())
                                };
                                
                                vertex_p3_n3_index Entry = {Vertex, VertexCount};
                                if(HashTable[Entry])
                                {
                                    IndexData[VertexId] = HashTable.Table[HashTable.GetHashIndex(Entry)].Key.Index;
                                }
                                else
                                {
                                    VertexData[VertexCount] = Vertex;                                                                                       
                                    IndexData[VertexId] = VertexCount;                                    
                                    VertexCount++;                                    
                                }                                
                            }                            
                        }
                                                
                    } break;
                }
            }
        }
    }
    
    handle_error:
    
    return {};
}