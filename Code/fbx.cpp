#include "fbx.h"

inline void 
FBX_AddNode(node_list* List, FbxNode* Node)
{
    ASSERT(List->Count < MAX_NODE_COUNT);
    List->Ptr[List->Count++] = Node;
}

inline void 
FBX_AddSkeleton(skeleton_list* List, fbx_skeleton* Skeleton)
{
    ASSERT(List->Count < MAX_SKELETON_COUNT);
    List->Ptr[List->Count++] = Skeleton;
}

inline b32
FBX_HasAttribute(FbxNode* Node, FbxNodeAttribute::EType Type)
{
    for(i32 AttributeIndex = 0; AttributeIndex < Node->GetNodeAttributeCount(); AttributeIndex++)
    {
        FbxNodeAttribute* Attribute = Node->GetNodeAttributeByIndex(AttributeIndex);            
        if(Attribute->GetAttributeType() == Type)
            return true;
    }
    return false;
}

void FBX_BuildSkeleton(fbx_skeleton* Skeleton, FbxNode* Joint)
{
    ASSERT(Skeleton->JointCount < 256);
    Skeleton->Joints[Skeleton->JointCount++] = Joint;
    
    for(i32 ChildIndex = 0; ChildIndex < Joint->GetChildCount(); ChildIndex++)
    {
        FbxNode* Child = Joint->GetChild(ChildIndex);
        if(FBX_HasAttribute(Child, FbxNodeAttribute::eSkeleton))
        {
            FBX_BuildSkeleton(Skeleton, Child);
        }
    }
}

u8 FBX_FindJoint(fbx_skeleton* Skeleton, const char* JointName)
{
    ASSERT(Skeleton->JointCount != (u8)-1);
    for(u8 JointIndex = 0; JointIndex < Skeleton->JointCount; JointIndex++)
    {
        FbxNode* Node = Skeleton->Joints[JointIndex];
        if(StringEquals(Node->GetName(), JointName))
            return JointIndex;
    }
    
    return (u8)-1;
}

void FBX_FindJointIndexAndSkeleton(fbx_context* Context, fbx_skeleton** OutSkeleton, u8* OutJointIndex, const char* JointName)
{
    for(u32 SkeletonIndex = 0; SkeletonIndex < Context->Skeletons.Count; SkeletonIndex++)
    {
        u8 FindJointIndex = FBX_FindJoint(Context->Skeletons.Ptr[SkeletonIndex], JointName);
        if(FindJointIndex != (u8)-1)
        {
            *OutSkeleton = Context->Skeletons.Ptr[SkeletonIndex];
            *OutJointIndex = FindJointIndex;
            return;
        }
    }
}

fbx_context FBX_LoadFile(const char* Path)
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
    
    fbx_context Result = {};
    Result.Scene = Scene;
    
    node_list SkeletonNodes = {};
    
    FbxNode* RootNode = Scene->GetRootNode();
    BOOL_CHECK_AND_HANDLE(RootNode, "Scene does not have a root node. Cannot import any mesh.");
    
    node_list NodeStack = {};
    FBX_AddNode(&NodeStack, RootNode);
    
    while(NodeStack.Count)
    {        
        FbxNode* Node = NodeStack.Ptr[--NodeStack.Count];
        
        for(i32 NodeIndex = 0; NodeIndex < Node->GetChildCount(); NodeIndex++)        
            FBX_AddNode(&NodeStack, Node->GetChild(NodeIndex));                    
        
        if(FBX_HasAttribute(Node, FbxNodeAttribute::eSkeleton))
            FBX_AddNode(&SkeletonNodes, Node);
        
        if(FBX_HasAttribute(Node, FbxNodeAttribute::eMesh))
        {
            if(BeginsWith(Node->GetName(), "Convex_Hull"))
                FBX_AddNode(&Result.ConvexHullNodes, Node);
            else                
                FBX_AddNode(&Result.MeshNodes, Node);        
        }
    }
    
    for(u32 SkeletonNodeIndex = 0; SkeletonNodeIndex < SkeletonNodes.Count; SkeletonNodeIndex++)
    {
        //NOTE(EVERYONE): Only process the root nodes since we are going to go down it's hierarchy
        FbxNode* Node = SkeletonNodes.Ptr[SkeletonNodeIndex];
        
        FbxNode* Parent = Node->GetParent();
        if(!FBX_HasAttribute(Parent, FbxNodeAttribute::eSkeleton))
        {
            fbx_skeleton* Skeleton = PushStruct(fbx_skeleton, Clear, 0);            
            FBX_BuildSkeleton(Skeleton, Node);
            FBX_AddSkeleton(&Result.Skeletons, Skeleton);
        }        
    }    
    
    return Result;
    
    handle_error:
    return {};
}

b32 ValidateMappingMode(FbxLayerElement::EMappingMode MappingMode)
{
    b32 Result = (MappingMode == FbxGeometryElement::eByPolygonVertex) || (MappingMode == FbxGeometryElement::eByControlPoint);
    return Result;
}

b32 ValidateReferenceMode(FbxLayerElement::EReferenceMode ReferenceMode)
{
    b32 Result = (ReferenceMode == FbxGeometryElement::eDirect) || (ReferenceMode == FbxGeometryElement::eIndexToDirect);
    return Result;
}

convex_hull FBX_LoadFirstConvexHull(fbx_context* Context, arena* Storage)
{
    if(Context->ConvexHullNodes.Count == 0)
        return {};
    
    FbxNode* Node = Context->ConvexHullNodes.Ptr[0];
    
    v3f GeometricTranslation = V3(Node->GetRotationPivot(FbxNode::eSourcePivot).Buffer());    
    m3 Transform = M3(M4(Node->EvaluateGlobalTransform()));
    
    FbxMesh* Mesh = Node->GetMesh();
    
    u32 VertexCount = Mesh->GetControlPointsCount();        
    FbxVector4* ControlPoints = Mesh->GetControlPoints();
    
    u32 FaceCount = Mesh->GetPolygonCount();
    u32 EdgeCount = Mesh->GetMeshEdgeCount()*2;
    
    convex_hull Result = {};
    Result.VertexCount = VertexCount;
    Result.FaceCount = FaceCount;
    Result.EdgeCount = EdgeCount;
    
    Result.Vertices = PushArray(Storage, VertexCount, convex_vertex, Clear, 0);
    Result.Faces = PushArray(Storage, FaceCount, convex_face, Clear, 0);
    Result.Edges = PushArray(Storage, EdgeCount, convex_edge, Clear, 0);
    
    for(u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
    {
        Result.Vertices[VertexIndex].V = (V3(ControlPoints[VertexIndex].Buffer())*Transform) - GeometricTranslation;
        Result.Vertices[VertexIndex].Edge = -1;
    }    
    
    for(u32 FaceIndex = 0; FaceIndex < FaceCount; FaceIndex++)    
        Result.Faces[FaceIndex].Edge = -1;    
    
    for(u32 EdgeIndex = 0; EdgeIndex < EdgeCount; EdgeIndex++)
    {
        Result.Edges[EdgeIndex].Vertex = -1;
        Result.Edges[EdgeIndex].EdgePair = -1;
        Result.Edges[EdgeIndex].Face = -1;
        Result.Edges[EdgeIndex].NextEdge = -1;
    }
    
    hash_map<int_pair, i32> EdgeMap = CreateHashMap<int_pair, i32>(8191);
    
    u32 EdgeIndex = 0;
    for(u32 FaceIndex = 0; FaceIndex < FaceCount; FaceIndex++)
    {
        BOOL_CHECK_AND_HANDLE(Mesh->GetPolygonSize(FaceIndex) == 3, "Convex hull must be triangulated.");    
        convex_face* Face = Result.Faces + FaceIndex;
        
        i32 FaceVertices[3] = 
        {
            Mesh->GetPolygonVertex(FaceIndex, 0),
            Mesh->GetPolygonVertex(FaceIndex, 1),
            Mesh->GetPolygonVertex(FaceIndex, 2)
        };
        
        i32 FaceEdgeIndices[3] = {-1, -1, -1};
        for(u32 i = 2, j = 0; j < 3; i = j++)
        {
            i32 v0 = FaceVertices[i];
            i32 v1 = FaceVertices[j];
            
            convex_vertex* Vertex0 = Result.Vertices + v0;
            convex_vertex* Vertex1 = Result.Vertices + v1;
            
            i32 e0 = -1;
            i32 e1 = -1;
            if(EdgeMap.Find({v0,v1}, &e0))
            {
                e1 = Result.Edges[e0].EdgePair;
            }
            else
            {
                e0 = EdgeIndex++;
                e1 = EdgeIndex++;
                
                Result.Edges[e0].EdgePair = e1;
                Result.Edges[e1].EdgePair = e0;
                
                EdgeMap.Insert({v0, v1}, e0);
                EdgeMap.Insert({v1, v0}, e1);                
            }
            
            convex_edge* Edge0 = Result.Edges + e0;
            convex_edge* Edge1 = Result.Edges + e1;
            
            if(Edge0->Vertex == -1)
                Edge0->Vertex = v1;
            
            if(Edge1->Vertex == -1)
                Edge1->Vertex = v0;
            
            if(Edge0->Face == -1)
                Edge0->Face = FaceIndex;
            
            if(Vertex0->Edge == -1)
                Vertex0->Edge = e0;
            
            if(Face->Edge == -1)
                Face->Edge = e0;
            
            FaceEdgeIndices[i] = e0;           
        }
                
        for(u32 i = 2, j = 0; j < 3; i = j++)
        {
            i32 e0 = FaceEdgeIndices[i];
            i32 e1 = FaceEdgeIndices[j];            
            Result.Edges[e0].NextEdge = e1;
        }
    }
    
    ASSERT(EdgeIndex == EdgeCount);
    return Result;
    
    handle_error:
    return {};
}

mesh FBX_LoadFirstMesh(fbx_context* Context, arena* Storage)
{
    if(Context->MeshNodes.Count == 0)
        return {};
    
    FbxNode* Node = Context->MeshNodes.Ptr[0];
    
    v3f GeometricTranslation = V3(Node->GetRotationPivot(FbxNode::eSourcePivot).Buffer());    
    m3 Transform = M3(M4(Node->EvaluateGlobalTransform()));        
    m3 NormalTransform = Transpose(InverseTransformM3(Transform));
    
    FbxMesh* Mesh = Node->GetMesh();
    
    FbxGeometryElementNormal* ElementNormals = Mesh->GetElementNormal(0);
    BOOL_CHECK_AND_HANDLE(ElementNormals, "Mesh did not have any normals.");                        
    
    FbxLayerElement::EMappingMode NormalMappingMode = ElementNormals->GetMappingMode();
    BOOL_CHECK_AND_HANDLE(ValidateMappingMode(NormalMappingMode), "Mapping mode for normals is not supported. Must be by polygon vertex or control point.");                                                                              
    
    FbxLayerElement::EReferenceMode NormalReferenceMode = ElementNormals->GetReferenceMode();
    BOOL_CHECK_AND_HANDLE(ValidateReferenceMode(NormalReferenceMode), "Reference mode for normals is not supported. Must be direct or index to direct.");        
    
    FbxGeometryElementUV* ElementUVs = Mesh->GetElementUV(0);
    BOOL_CHECK_AND_HANDLE(ElementUVs, "Mesh  did not have any uvs supplied.");
    
    FbxLayerElement::EMappingMode UVMappingMode = ElementUVs->GetMappingMode();
    BOOL_CHECK_AND_HANDLE(ValidateMappingMode(UVMappingMode), "Mapping mode for uvs is not supported. Must be by polygon vertex or control point.");
    
    FbxLayerElement::EReferenceMode UVReferenceMode = ElementUVs->GetReferenceMode();
    BOOL_CHECK_AND_HANDLE(ValidateReferenceMode(UVReferenceMode), "Reference mode for uvs is not supported. Must be direct or index to direct.");    
    
    u32 ControlPointCount = Mesh->GetControlPointsCount();
    FbxVector4* ControlPoints = Mesh->GetControlPoints();                        
    
    u32 SkinCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);                        
    
    control_point_joint_data* JointsData = NULL;
    if(SkinCount > 0)
    {
        JointsData = PushArray(ControlPointCount, control_point_joint_data, Clear, 0);
        
        fbx_skeleton* Skeleton = NULL;        
        for(u32 SkinIndex = 0; SkinIndex < SkinCount; SkinIndex++)
        {
            FbxSkin* Skin = (FbxSkin*)Mesh->GetDeformer(SkinIndex, FbxDeformer::eSkin);
            u32 ClusterCount = Skin->GetClusterCount();
            for(u32 ClusterIndex = 0; ClusterIndex < ClusterCount; ClusterIndex++)
            {
                FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
                FbxNode* Link = Cluster->GetLink();
                if(Link)
                {
                    u8 JointIndex = (u8)-1;
                    if(!Skeleton)
                        FBX_FindJointIndexAndSkeleton(Context, &Skeleton, &JointIndex, Link->GetName()); 
                    else
                        JointIndex = FBX_FindJoint(Skeleton, Link->GetName());
                    
                    if(JointIndex != -1)
                    {                        
                        u32 IndexCount = Cluster->GetControlPointIndicesCount();
                        i32* Indices = Cluster->GetControlPointIndices();
                        f64* Weights = Cluster->GetControlPointWeights();
                        
                        for(u32 Index = 0; Index < IndexCount; Index++)
                        {
                            i32 ControlPointIndex = Indices[Index];
                            
                            control_point_joint_data* JointData = JointsData + ControlPointIndex;
                            
                            ASSERT(JointData->Count < 4);
                            
                            JointData->Weights[JointData->Count] = Weights[Index];
                            JointData->Indices[JointData->Count] = JointIndex;
                            
                            JointData->Count++;
                        }
                    }
                }
            }   
        }
        
        for(u32 ControlPointIndex = 0; ControlPointIndex < ControlPointCount; ControlPointIndex++)
        {
            control_point_joint_data* JointData = JointsData + ControlPointIndex;            
            
            while(JointData->Count < 4)
            {
                JointData->Weights[JointData->Count] = 0.0;
                JointData->Indices[JointData->Count] = 0;
                JointData->Count++;
            }            
        }
    }
    
    u32 PolygonCount = Mesh->GetPolygonCount();
    
    u32 IndexCount = PolygonCount*3;
    u32 VertexCount = 0;    
    u32* IndexData = PushArray(IndexCount, u32, Clear, 0);
    
    mesh Result = {};
    
    hash_table<vertex_p3_n3_uv_index> HashTable = CreateHashTable<vertex_p3_n3_uv_index>(FBX_HASH_SIZE*4);     
    
    void* VertexData;
    if(JointsData)
        VertexData = PushArray(IndexCount, vertex_p3_n3_uv_weights, Clear, 0);
    else
        VertexData = PushArray(IndexCount, vertex_p3_n3_uv, Clear, 0);
    
    for(u32 PolygonIndex = 0; PolygonIndex < PolygonCount; PolygonIndex++)
    {
        BOOL_CHECK_AND_HANDLE(Mesh->GetPolygonSize(PolygonIndex) == 3, "Mesh had a polygon that did not have 3 vertices. All polygons must be triangles.");
        
        
        v3f TriangleP[3];                           
        for(u32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
        {
            u32 VertexId = (PolygonIndex*3) + VertexIndex;
            
            i32 ControlPointIndex = Mesh->GetPolygonVertex(PolygonIndex, VertexIndex);
            i32 NormalIndex = -1;
            i32 UVIndex = -1;
            
            if(NormalMappingMode == FbxGeometryElement::eByPolygonVertex)
            {
                if(NormalReferenceMode == FbxGeometryElement::eDirect)                    
                    NormalIndex = VertexId;                
                else if(NormalReferenceMode == FbxGeometryElement::eIndexToDirect)                
                    NormalIndex = ElementNormals->GetIndexArray().GetAt(VertexId);                
                INVALID_ELSE;
            }
            else if(NormalMappingMode == FbxGeometryElement::eByControlPoint)
            {
                if(NormalReferenceMode == FbxGeometryElement::eDirect)
                    NormalIndex = ControlPointIndex;
                else if(NormalReferenceMode == FbxGeometryElement::eIndexToDirect)
                    NormalIndex = ElementNormals->GetIndexArray().GetAt(ControlPointIndex);
                INVALID_ELSE;
                
            }
            INVALID_ELSE;
            
            if(UVMappingMode == FbxGeometryElement::eByPolygonVertex)
            {
                if(UVReferenceMode == FbxGeometryElement::eDirect)
                    UVIndex = VertexId;
                else if(UVReferenceMode == FbxGeometryElement::eIndexToDirect)
                    UVIndex = ElementUVs->GetIndexArray().GetAt(VertexId);
                INVALID_ELSE;
            }
            else if(UVMappingMode == FbxGeometryElement::eByControlPoint)
            {
                if(UVReferenceMode == FbxGeometryElement::eDirect)
                    UVIndex = ControlPointIndex;
                else if(UVReferenceMode == FbxGeometryElement::eIndexToDirect)
                    UVIndex = ElementUVs->GetIndexArray().GetAt(ControlPointIndex);
                INVALID_ELSE;
            }
            
            TriangleP[VertexIndex] = V3(ControlPoints[ControlPointIndex].Buffer());
            vertex_p3_n3_uv Vertex = 
            {
                TriangleP[VertexIndex],
                V3(ElementNormals->GetDirectArray().GetAt(NormalIndex).Buffer()),
                V2(ElementUVs->GetDirectArray().GetAt(UVIndex).Buffer())
            };
            
            vertex_p3_n3_uv_index Entry = {Vertex, VertexCount};
            if(HashTable[Entry])
            {
                IndexData[VertexId] = HashTable.Table[HashTable.GetHashIndex(Entry)].Key.Index;
            }
            else                
            {
                if(JointsData)
                {
                    control_point_joint_data JointData = JointsData[ControlPointIndex];
                    ((vertex_p3_n3_uv_weights*)VertexData)[VertexCount] = 
                    {
                        Vertex.P, Vertex.N, Vertex.UV,
                        JointData.Indices[0], JointData.Indices[1], JointData.Indices[2], JointData.Indices[3], 
                        (f32)JointData.Weights[0], (f32)JointData.Weights[1], (f32)JointData.Weights[2], (f32)JointData.Weights[3]
                    };
                }
                else
                {
                    ((vertex_p3_n3_uv*)VertexData)[VertexCount] = {Vertex.P, Vertex.N, Vertex.UV};                                                                                       
                }
                
                IndexData[VertexId] = VertexCount;                                    
                VertexCount++;                                    
            }                                
        }                                            
    }                                                
    
    Result.VertexCount = VertexCount;        
    if(JointsData)
    {
        Result.VertexFormat = GRAPHICS_VERTEX_FORMAT_P3_N3_T4_UV_WEIGHTS;
        
        vertex_p3_n3_uv_weights* Src = (vertex_p3_n3_uv_weights*)VertexData;
        v3f* Tangents = PushArray(VertexCount, v3f, Clear, 0);
        v3f* Bitangents = PushArray(VertexCount, v3f, Clear, 0);
        for(u32 TriangleIndex = 0; TriangleIndex < (IndexCount/3); TriangleIndex++)
        {
            u32 VertexIndex0 = IndexData[(TriangleIndex*3)+0];
            u32 VertexIndex1 = IndexData[(TriangleIndex*3)+1];
            u32 VertexIndex2 = IndexData[(TriangleIndex*3)+2];
            
            v3f V0 = Src[VertexIndex0].P;
            v3f V1 = Src[VertexIndex1].P;
            v3f V2 = Src[VertexIndex2].P;
            
            v2f UV0 = Src[VertexIndex0].UV;
            v2f UV1 = Src[VertexIndex1].UV;
            v2f UV2 = Src[VertexIndex2].UV;
            
            v3f Edge0 = V1-V0;
            v3f Edge1 = V2-V0;
            
            float S0 = UV1.x - UV0.x;
            float S1 = UV2.x - UV0.x;
            float T0 = UV1.y - UV0.y;
            float T1 = UV2.y - UV0.y;
            
            float R = 1.0f/(S0*T1 - S1*T0);
            
            v3f TanDir = V3((T1*Edge0.x - T0*Edge1.x)*R,
                            (T1*Edge0.y - T0*Edge1.y)*R,
                            (T1*Edge0.z - T0*Edge1.z)*R);
            
            v3f BiDir = V3((S0*Edge1.x - S1*Edge0.x)*R,
                           (S0*Edge1.y - S1*Edge0.y)*R,
                           (S0*Edge1.z - S1*Edge0.z)*R);
            
            Tangents[VertexIndex0] += TanDir;
            Tangents[VertexIndex1] += TanDir;
            Tangents[VertexIndex2] += TanDir;                   
            
            Bitangents[VertexIndex0] += BiDir;
            Bitangents[VertexIndex1] += BiDir;
            Bitangents[VertexIndex2] += BiDir;            
        }
        
        Result.Vertices = PushArray(Storage, VertexCount, vertex_p3_n3_t4_uv_weights, Clear, 0);
        Src = (vertex_p3_n3_uv_weights*)VertexData;
        vertex_p3_n3_t4_uv_weights* Dst = (vertex_p3_n3_t4_uv_weights*)Result.Vertices;
        for(u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
        {
            vertex_p3_n3_uv_weights* SrcVertex = Src + VertexIndex;
            vertex_p3_n3_t4_uv_weights* DstVertex = Dst + VertexIndex;                                                                        
            
            v3f T = Normalize(Tangents[VertexIndex]);
            
            DstVertex->P = SrcVertex->P;
            DstVertex->N = Normalize(SrcVertex->N);
            DstVertex->T = V4(Normalize(T - DstVertex->N * Dot(DstVertex->N, T)),
                              (Dot(Cross(DstVertex->N, T), Bitangents[VertexIndex]) < 0.0f) ? -1.0f : 1.0f);
            DstVertex->UV = SrcVertex->UV;           
            
            DstVertex->P = DstVertex->P*Transform - GeometricTranslation;
            DstVertex->N = Normalize(DstVertex->N*NormalTransform);
            DstVertex->T.xyz = Normalize(DstVertex->T.xyz*Transform);
            
            CopyMemory(DstVertex->JointI, SrcVertex->JointI, sizeof(DstVertex->JointI));
            CopyMemory(DstVertex->JointW, SrcVertex->JointW, sizeof(DstVertex->JointW));
        }                
    }
    else
    {
        Result.VertexFormat = GRAPHICS_VERTEX_FORMAT_P3_N3_T4_UV;        
        
        vertex_p3_n3_uv* Src = (vertex_p3_n3_uv*)VertexData;
        
        v3f* Tangents = PushArray(VertexCount, v3f, Clear, 0);
        v3f* Bitangents = PushArray(VertexCount, v3f, Clear, 0);        
        for(u32 TriangleIndex = 0; TriangleIndex < (IndexCount/3); TriangleIndex++)
        {
            u32 Index0 = IndexData[(TriangleIndex*3)+0];
            u32 Index1 = IndexData[(TriangleIndex*3)+1];
            u32 Index2 = IndexData[(TriangleIndex*3)+2];
            
            v3f V0 = Src[Index0].P;
            v3f V1 = Src[Index1].P;
            v3f V2 = Src[Index2].P;
            
            v2f UV0 = Src[Index0].UV;
            v2f UV1 = Src[Index1].UV;
            v2f UV2 = Src[Index2].UV;
            
            v3f Edge0 = V1-V0;
            v3f Edge1 = V2-V0;
            
            float S0 = UV1.x - UV0.x;
            float S1 = UV2.x - UV0.x;
            float T0 = UV1.y - UV0.y;
            float T1 = UV2.y - UV0.y;
            
            float R = 1.0f/(S0*T1 - S1*T0);
            
            v3f TanDir = V3((T1*Edge0.x - T0*Edge1.x)*R,
                            (T1*Edge0.y - T0*Edge1.y)*R,
                            (T1*Edge0.z - T0*Edge1.z)*R);
            
            v3f BiDir = V3((S0*Edge1.x - S1*Edge0.x)*R,
                           (S0*Edge1.y - S1*Edge0.y)*R,
                           (S0*Edge1.z - S1*Edge0.z)*R);
            
            Tangents[Index0] += TanDir;
            Tangents[Index1] += TanDir;
            Tangents[Index2] += TanDir;                   
            
            Bitangents[Index0] += BiDir;
            Bitangents[Index1] += BiDir;
            Bitangents[Index2] += BiDir;            
        }
        
        Result.Vertices = PushArray(Storage, VertexCount, vertex_p3_n3_t4_uv, Clear, 0);
        Src = (vertex_p3_n3_uv*)VertexData;
        vertex_p3_n3_t4_uv* Dst = (vertex_p3_n3_t4_uv*)Result.Vertices;
        for(u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
        {
            vertex_p3_n3_uv* SrcVertex = Src + VertexIndex;
            vertex_p3_n3_t4_uv* DstVertex = Dst + VertexIndex;                                                                        
            
            v3f T = Normalize(Tangents[VertexIndex]);
            
            DstVertex->P = SrcVertex->P;
            DstVertex->N = Normalize(SrcVertex->N);
            DstVertex->T = V4(Normalize(T - DstVertex->N * Dot(DstVertex->N, T)),
                              (Dot(Cross(DstVertex->N, T), Bitangents[VertexIndex]) < 0.0f) ? -1.0f : 1.0f);
            DstVertex->UV = SrcVertex->UV;           
            
            DstVertex->P = DstVertex->P*Transform - GeometricTranslation;
            DstVertex->N = Normalize(DstVertex->N*NormalTransform);
            DstVertex->T.xyz = Normalize(DstVertex->T.xyz*Transform);
        }                   
    }
    
    Result.IndexCount = IndexCount;
    if(VertexCount < 0xFFFF)
    {
        Result.IndexFormat = GRAPHICS_INDEX_FORMAT_16_BIT;  
        Result.Indices = PushArray(Storage, IndexCount, u16, NoClear, 0);
        
        u16* Indices = (u16*)Result.Indices;
        for(u32 Index = 0; Index < IndexCount; Index++) Indices[Index] = SafeU16(IndexData[Index]);
    }
    else
    {
        Result.IndexFormat = GRAPHICS_INDEX_FORMAT_32_BIT;
        Result.Indices = PushWriteArray(Storage, IndexData, IndexCount, u32, 0);
    }
    
    return Result;  
    
    handle_error:
    return {};
}

walkable_mesh FBX_LoadFirstWalkableMesh(fbx_context* Context, arena* Storage)
{
    if(Context->MeshNodes.Count == 0)
        return {};
    
    FbxNode* Node = Context->MeshNodes.Ptr[0];
    
    v3f GeometricTranslation = V3(Node->GetRotationPivot(FbxNode::eSourcePivot).Buffer());
    m3 Transform = M3(M4(Node->EvaluateGlobalTransform()));            
    FbxMesh* Mesh = Node->GetMesh();
    
    u32 ControlPointCount = Mesh->GetControlPointsCount();
    FbxVector4* ControlPoints = Mesh->GetControlPoints();                            
    
    walkable_mesh Result = {};    
    
    u32 PolygonCount = Mesh->GetPolygonCount();    
    Result.TriangleCount = PolygonCount;
    Result.Triangles = PushArray(Storage, PolygonCount, walkable_triangle, Clear, 0);
    
    hash_map<i32, edge_triangle_entry> HashMap = CreateHashMap<i32, edge_triangle_entry>(FBX_HASH_SIZE);
    
    for(u32 TriangleIndex = 0; TriangleIndex < Result.TriangleCount; TriangleIndex++)
    {
        BOOL_CHECK_AND_HANDLE(Mesh->GetPolygonSize(TriangleIndex) == 3, "Mesh had a polygon that did not have 3 vertices. All polygons must be triangles.");        
        walkable_triangle* Triangle = Result.Triangles + TriangleIndex;
        
        i32 CPIndex0 = Mesh->GetPolygonVertex(TriangleIndex, 0);
        i32 CPIndex1 = Mesh->GetPolygonVertex(TriangleIndex, 1);
        i32 CPIndex2 = Mesh->GetPolygonVertex(TriangleIndex, 2);
        
        bool Reversed;
        i32 EdgeIndex0 = Mesh->GetMeshEdgeIndex(CPIndex0, CPIndex1, Reversed);
        i32 EdgeIndex1 = Mesh->GetMeshEdgeIndex(CPIndex1, CPIndex2, Reversed);
        i32 EdgeIndex2 = Mesh->GetMeshEdgeIndex(CPIndex2, CPIndex0, Reversed);                
        
        edge_triangle_entry EdgeAdjTriangleEntry;
        if(HashMap.Find(EdgeIndex0, &EdgeAdjTriangleEntry))
        {
            EdgeAdjTriangleEntry.Triangle->AdjTriangles[EdgeAdjTriangleEntry.EdgeIndex] = Triangle;
            Triangle->AdjTriangles[0] = EdgeAdjTriangleEntry.Triangle;
        }
        else
        {
            edge_triangle_entry EdgeTriangleEntry = {0, Triangle};            
            HashMap.Insert(EdgeIndex0, EdgeTriangleEntry);
        }
        
        if(HashMap.Find(EdgeIndex1, &EdgeAdjTriangleEntry))
        {
            EdgeAdjTriangleEntry.Triangle->AdjTriangles[EdgeAdjTriangleEntry.EdgeIndex] = Triangle;
            Triangle->AdjTriangles[1] = EdgeAdjTriangleEntry.Triangle;
        }
        else
        {
            edge_triangle_entry EdgeTriangleEntry = {1, Triangle};            
            HashMap.Insert(EdgeIndex1, EdgeTriangleEntry);
        }
        
        if(HashMap.Find(EdgeIndex2, &EdgeAdjTriangleEntry))
        {
            EdgeAdjTriangleEntry.Triangle->AdjTriangles[EdgeAdjTriangleEntry.EdgeIndex] = Triangle;
            Triangle->AdjTriangles[2] = EdgeAdjTriangleEntry.Triangle;
        }
        else
        {
            edge_triangle_entry EdgeTriangleEntry = {2, Triangle};            
            HashMap.Insert(EdgeIndex2, EdgeTriangleEntry);
        }
        
        for(u32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
        {
            u32 VertexId = (TriangleIndex*3) + VertexIndex;           
            i32 ControlPointIndex = Mesh->GetPolygonVertex(TriangleIndex, VertexIndex);            
            Triangle->P[VertexIndex] = V3(ControlPoints[ControlPointIndex].Buffer())*Transform - GeometricTranslation;            
        }                                    
    }                                                
    
    return Result;  
    
    handle_error:
    return {};
}

skeleton FBX_LoadFirstSkeleton(fbx_context* Context, arena* Storage)
{
    BOOL_CHECK_AND_HANDLE(Context->Skeletons.Count > 0, "No skeletons associated with the fbx file.");
    
    if(Context->Skeletons.Count == 0)
        return {};
    
    fbx_skeleton* FBXSkeleton = Context->Skeletons.Ptr[0];
    skeleton Result = {};
    Result.JointCount = FBXSkeleton->JointCount;
    Result.Joints = PushArray(Storage, Result.JointCount, joint, Clear, 0);
    
    for(u32 JointIndex = 0; JointIndex < Result.JointCount; JointIndex++)
    {
        FbxNode* JointNode = FBXSkeleton->Joints[JointIndex];
        joint* Joint = Result.Joints + JointIndex;
        
        m4 JointToModel = M4(JointNode->EvaluateGlobalTransform());
        Joint->ModelToJoint = InverseTransformM4(JointToModel);                
        Joint->ParentIndex = (u8)-1;
        
        FbxNode* Parent = JointNode->GetParent();
        if(Parent)        
            Joint->ParentIndex = FBX_FindJoint(FBXSkeleton, Parent->GetName());        
    }
        
    return Result;
    
    handle_error:
    return {};
}

animation_clip FBX_LoadSkeletonAnimation(fbx_context* Context, fbx_skeleton* Skeleton, arena* Storage)
{
    FbxScene* Scene = Context->Scene;
    FbxAnimStack* AnimStack = Scene->GetSrcObject<FbxAnimStack>();
    BOOL_CHECK_AND_HANDLE(AnimStack, "No animation associated with the fbx file.");
    
    animation_clip Clip;
    Clip.JointCount = Skeleton->JointCount;
    
    FbxTakeInfo* TakeInfo = Scene->GetTakeInfo(AnimStack->GetName());
    FbxTime Start = TakeInfo->mLocalTimeSpan.GetStart();
    FbxTime End = TakeInfo->mLocalTimeSpan.GetStop();            
    
    Clip.FrameCount = (u32)(End.GetFrameCount(FbxTime::eFrames30) - Start.GetFrameCount(FbxTime::eFrames30) + 1);    
    Clip.Frames = PushArray(Storage, Clip.FrameCount, animation_frame, Clear, 0);
    
    animation_frame* FrameAt = Clip.Frames;
    
    for(FbxLongLong FrameIndex = Start.GetFrameCount(FbxTime::eFrames30); FrameIndex <= End.GetFrameCount(FbxTime::eFrames30); FrameIndex++)
    {
        FbxTime CurrentTime;
        CurrentTime.SetFrame(FrameIndex, FbxTime::eFrames30);
        
        animation_frame* Frame = FrameAt++;
        Frame->JointPoses = PushArray(Storage, Clip.JointCount, joint_pose, Clear, 0);
        
        for(u32 JointIndex = 0; JointIndex < Clip.JointCount; JointIndex++)
        {
            m4 Transform = M4(Skeleton->Joints[JointIndex]->EvaluateLocalTransform(CurrentTime));
            sqt SQT = CreateSQT(Transform);
            
            Frame->JointPoses[JointIndex].Translation = SQT.Translation;
            Frame->JointPoses[JointIndex].Orientation = SQT.Orientation;
        }
    }
    
    return Clip;
    
    handle_error:
    return {};
}

animation_clip FBX_LoadFirstAnimation(fbx_context* Context, arena* Storage)
{
    BOOL_CHECK_AND_HANDLE(Context->Skeletons.Count > 0, "No skeletons associated with the fbx file. Cannot load it's animations");
    
    animation_clip Result = FBX_LoadSkeletonAnimation(Context, Context->Skeletons.Ptr[0], Storage);
    return Result;
    
    handle_error:
    return {};
}