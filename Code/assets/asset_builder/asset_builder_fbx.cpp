#include <fbxsdk.h>

struct fbx_skeleton
{
    u32 JointCount;
    FbxNode* Nodes[128];
};

struct fbx_mesh_context
{
    b32 ShouldSkip;
    dynamic_array<FbxNode*> ConvexHullNodes;
    fbx_skeleton Skeleton;    
};

struct control_point_joint_data
{
    u32 Count;
    f32 Weights[4];
    u8 Indices[4];
};

struct fbx_joint_context
{
    b32 ShouldSkip;
    control_point_joint_data* JointsData;
};

struct vertex_p3_n3_uv_index
{
    vertex_p3_n3_uv Vertex;
    u32 Index;
};

inline u64 Hash(vertex_p3_n3_uv_index Data, u64 TableSize)
{
    u64 Result = Hash(Data.Vertex, TableSize);
    return Result;
}

inline b32 operator!=(vertex_p3_n3_uv_index Left, vertex_p3_n3_uv_index Right)
{
    b32 Result = Left.Vertex != Right.Vertex;
    return Result;
}

global FbxManager* Global_FBXManager = NULL;

inline b32 
FBX_ValidateMappingMode(FbxLayerElement::EMappingMode MappingMode)
{
    b32 Result = (MappingMode == FbxGeometryElement::eByPolygonVertex) || (MappingMode == FbxGeometryElement::eByControlPoint);
    return Result;
}

inline b32 
FBX_ValidateReferenceMode(FbxLayerElement::EReferenceMode ReferenceMode)
{
    b32 Result = (ReferenceMode == FbxGeometryElement::eDirect) || (ReferenceMode == FbxGeometryElement::eIndexToDirect);
    return Result;
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

inline b32 IsValidConvexHull(FbxNode* Node)
{
    if(!Node->GetMesh()->IsTriangleMesh())
    {
        ConsoleError("Convex hull for mesh is not a triangle mesh. Please triangulate the convex hull manually or via your modeling program's exporter. Skipping Mesh");
        return false;
    }
    
    //TODO(JJ): Need to add a check to determine if your convex hull is actually a convex hull!
    return true;
}

fbx_mesh_context FBX_GetMeshContext(FbxNode* Node)
{   
    fbx_mesh_context MeshContext = {};
    MeshContext.ConvexHullNodes = CreateDynamicArray<FbxNode*>(16);
    
    u32 ChildCount = Node->GetChildCount();
    for(u32 ChildIndex = 0; ChildIndex < ChildCount; ChildIndex++)
    {
        FbxNode* ChildNode = Node->GetChild(ChildIndex);
        const char* ChildName = ChildNode->GetName();
        
        if(FBX_HasAttribute(ChildNode, FbxNodeAttribute::eMesh) && EndsWith(ChildName, "_ConvexHull"))
        {   
            if(!IsValidConvexHull(ChildNode))
            {
                MeshContext.ShouldSkip = true;
                return MeshContext;
            }
            
            Append(&MeshContext.ConvexHullNodes, ChildNode);
        }
        
        if(FBX_HasAttribute(ChildNode, FbxNodeAttribute::eSkeleton))
        {
            u32 StackCount = 0;
            FbxNode* NodeStack[256];
            
            NodeStack[StackCount++] = ChildNode;
            
            while(StackCount > 0)
            {
                FbxNode* StackNode = NodeStack[StackCount--];    
                
                if(MeshContext.Skeleton.JointCount == ARRAYCOUNT(MeshContext.Skeleton.Nodes))
                {
                    ConsoleError("Skeleton nodes exceeds the maximum value for a skeletal mesh (128). Skipping Mesh");
                    MeshContext.ShouldSkip = true;                    
                    return MeshContext;                    
                }
                
                MeshContext.Skeleton.Nodes[MeshContext.Skeleton.JointCount++] = StackNode;                            
                
                u32 StackNodeChildCount = StackNode->GetChildCount();
                for(u32 StackNodeIndex = 0; StackNodeIndex < StackNodeChildCount; StackNodeIndex++)
                {
                    FbxNode* StackNodeChild = StackNode->GetChild(StackNodeIndex);
                    if(FBX_HasAttribute(StackNodeChild, FbxNodeAttribute::eSkeleton))
                        NodeStack[StackCount++] = StackNodeChild;                                
                }
            }
            
        }
    }                
    
    return MeshContext;
}

u8 FBX_FindJoint(fbx_skeleton* Skeleton, const char* JointName)
{
    ASSERT(Skeleton->JointCount != (u8)-1);
    for(u8 JointIndex = 0; JointIndex < Skeleton->JointCount; JointIndex++)
    {
        FbxNode* Node = Skeleton->Nodes[JointIndex];
        if(StringEquals(Node->GetName(), JointName))
            return JointIndex;
    }
    
    return (u8)-1;
}

fbx_joint_context FBX_GetJointContext(FbxMesh* Mesh, fbx_mesh_context* MeshContext)
{        
    u32 ControlPointCount = Mesh->GetControlPointsCount();
    FbxVector4* ControlPoints = Mesh->GetControlPoints();
    
    fbx_joint_context Result = {};
    
    u32 SkinCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
    
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
                i8 JointIndex = FBX_FindJoint(&MeshContext->Skeleton, Link->GetName());
                if(JointIndex != -1)
                {
                    u32 IndexCount = Cluster->GetControlPointIndicesCount();
                    i32* Indices = Cluster->GetControlPointIndices();
                    f64* Weights = Cluster->GetControlPointWeights();
                    
                    for(u32 Index = 0; Index < IndexCount; Index++)
                    {
                        if(!Result.JointsData)
                            Result.JointsData = PushArray(ControlPointCount, control_point_joint_data, Clear, 0);
                        
                        i32 ControlPointIndex = Indices[Index];
                        
                        control_point_joint_data* JointData = Result.JointsData + ControlPointIndex;
                        if(JointData->Count >= 4)
                        {                                        
                            ConsoleError("A control point has more than 4 binded joints. This is invalid. Skipping Mesh");
                            return Result;
                        }
                        
                        JointData->Weights[JointData->Count] = (f32)Weights[Index];
                        JointData->Indices[JointData->Count] = JointIndex;                        
                        JointData->Count++;
                    }
                }
            }
        }
    }
    
    if(Result.JointsData)
    {
        
        for(u32 ControlPointIndex = 0; ControlPointIndex < ControlPointCount; ControlPointIndex++)
        {
            control_point_joint_data* JointData = Result.JointsData + ControlPointIndex;            
            
            while(JointData->Count < 4)
            {
                JointData->Weights[JointData->Count] = 0.0;
                JointData->Indices[JointData->Count] = 0;
                JointData->Count++;
            }            
        }
    }
    
    return Result;
}

template <typename type>
i32 GetElementIndex(FbxLayerElementTemplate<type>* GeometryElement, FbxLayerElement::EMappingMode MappingMode, FbxLayerElement::EReferenceMode ReferenceMode, 
                    i32 VertexID, i32 ControlPointID)
{
    i32 Result = -1;
    
    if(MappingMode == FbxGeometryElement::eByPolygonVertex)
    {
        if(ReferenceMode == FbxGeometryElement::eDirect)                    
            Result = VertexID;                
        else if(ReferenceMode == FbxGeometryElement::eIndexToDirect)                
            Result = GeometryElement->GetIndexArray().GetAt(VertexID);                
        INVALID_ELSE;
    }
    else if(MappingMode == FbxGeometryElement::eByControlPoint)
    {
        if(ReferenceMode == FbxGeometryElement::eDirect)
            Result = ControlPointID;
        else if(ReferenceMode == FbxGeometryElement::eIndexToDirect)
            Result = GeometryElement->GetIndexArray().GetAt(ControlPointID);
        INVALID_ELSE;        
    }
    INVALID_ELSE;
    
    return Result;
}

inline v4f 
OrthogonalTangent(v3f B, v3f T, v3f N)
{
    v4f Result = V4(Normalize(T - N * Dot(N, T)), (Dot(Cross(N, T), B) < 0.0f) ? -1.0f : 1.0f);
    return Result;
}

inline v3f
FBX_GetPivot(FbxNode* Node)
{    
    v3f Result = V3(Node->GetRotationPivot(FbxNode::eSourcePivot).Buffer());
    return Result;
}

inline m4
FBX_GetLocalToParentTransform(FbxNode* Node)
{
    m4 Result = M4(Node->EvaluateLocalTransform());
    return Result;
}

inline v3f
FBX_GetPivotDelta(v3f Pivot, m4 Transform)
{
    v3f Delta = (Pivot.x*Transform.XAxis.xyz) + (Pivot.y*Transform.YAxis.xyz) + (Pivot.z*Transform.ZAxis.xyz);
    return Delta;
}

void ParseFBX(asset_builder* AssetBuilder, string Path)
{
    if(!Global_FBXManager)
    {
        Global_FBXManager = FbxManager::Create();
        FbxIOSettings* IOSettings = FbxIOSettings::Create(Global_FBXManager, IOSROOT);
        Global_FBXManager->SetIOSettings(IOSettings);
    }
    
    temp_arena TempArena = BeginTemporaryMemory();
    
    FbxImporter* Importer = FbxImporter::Create(Global_FBXManager, "");
    
    ConsoleLog("Loading FBX File %s", Path);
    if(Importer->Initialize(Path.Data, -1, Global_FBXManager->GetIOSettings()))
    {
        i32 MajorVersion, MinorVersion, RevisionVersion;
        Importer->GetFileVersion(MajorVersion, MinorVersion, RevisionVersion);        
        ConsoleLog("Version %d.%d.%d", MajorVersion, MinorVersion, RevisionVersion);
        
        FbxScene* Scene = FbxScene::Create(Global_FBXManager, "");
        Importer->Import(Scene);
        
        FbxNode* RootNode = Scene->GetRootNode();
        
        u32 RootChildCount = RootNode->GetChildCount();
        
        for(u32 RootChildIndex = 0; RootChildIndex < RootChildCount; RootChildIndex++)
        {
            FbxNode* Node = RootNode->GetChild(RootChildIndex);
            
            if(FBX_HasAttribute(Node, FbxNodeAttribute::eMesh))
            {
                FbxMesh* MeshFBX = Node->GetMesh();
                const char* MeshName = Node->GetName();
                
                ConsoleNewLine();
                ConsoleLog("Processing Mesh %s", MeshName);
                
                if(!MeshFBX->IsTriangleMesh()) { ConsoleError("Mesh is not a triangle mesh. Please triangulate the mesh either manually or via your modeling program's exporter. Skipping Mesh"); continue; }
                
                FbxGeometryElementNormal* ElementNormals = MeshFBX->GetElementNormal(0);
                if(!ElementNormals) { ConsoleError("Mesh does not have normals. Skipping mesh"); continue; }
                
                FbxGeometryElementUV* ElementUVs = MeshFBX->GetElementUV(0);
                if(!ElementUVs) { ConsoleError("Mesh does not have UVs. Skipping mesh"); continue; }                                
                
                FbxLayerElement::EMappingMode NormalMappingMode = ElementNormals->GetMappingMode();
                if(!FBX_ValidateMappingMode(NormalMappingMode)) { ConsoleError("Mapping mode for the mesh normals is not supported. Must be by polygon vertex or control point. Skipping Mesh"); continue; }
                
                FbxLayerElement::EMappingMode UVMappingMode = ElementUVs->GetMappingMode();
                if(!FBX_ValidateMappingMode(UVMappingMode)) { ConsoleError("Mapping mode for the mesh uvs is not supported. Must be by polygon vertex or control point. Skipping Mesh"); continue; }
                
                FbxLayerElement::EReferenceMode NormalReferenceMode = ElementNormals->GetReferenceMode();
                if(!FBX_ValidateReferenceMode(NormalReferenceMode)) { ConsoleError("Reference mode for the mesh normals is not supported. Must be direct or index to direct. Skipping Mesh"); continue; }
                
                FbxLayerElement::EReferenceMode UVReferenceMode = ElementUVs->GetReferenceMode();
                if(!FBX_ValidateReferenceMode(UVReferenceMode)) { ConsoleError("Reference mode for the mesh uvs is not supported. Must be direct or index to direct. Skipping Mesh"); continue; }
                
                fbx_mesh_context MeshContext = FBX_GetMeshContext(Node);
                if(MeshContext.ShouldSkip) { DeleteDynamicArray(&MeshContext.ConvexHullNodes); continue; }
                
                ConsoleLog("Mesh has %d Convex Hulls", MeshContext.ConvexHullNodes.Size);
                
                fbx_joint_context JointContext = FBX_GetJointContext(MeshFBX, &MeshContext);
                if(JointContext.ShouldSkip) { DeleteDynamicArray(&MeshContext.ConvexHullNodes); continue; }                                
                
                u32 TriangleCount = MeshFBX->GetPolygonCount();                
                u32 IndexCount = TriangleCount*3;
                u32 VertexCount = 0;
                
                u32* IndexData = PushArray(IndexCount, u32, Clear, 0);
                hash_map<vertex_p3_n3_uv, u32> VertexMap = CreateHashMap<vertex_p3_n3_uv, u32>(IndexCount*4);                
                
                void* VertexData;
                if(JointContext.JointsData)
                    VertexData = PushArray(IndexCount, vertex_p3_n3_uv_weights, Clear, 0);
                else
                    VertexData = PushArray(IndexCount, vertex_p3_n3_uv, Clear, 0);
                
                FbxVector4* ControlPoints = MeshFBX->GetControlPoints();                                
                for(u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
                {
                    for(u32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
                    {
                        u32 VertexID = (TriangleIndex*3)+VertexIndex;                        
                        i32 ControlPointIndex = MeshFBX->GetPolygonVertex(TriangleIndex, VertexIndex);
                        i32 NormalIndex = GetElementIndex(ElementNormals, NormalMappingMode, NormalReferenceMode, VertexID, ControlPointIndex);
                        i32 UVIndex = GetElementIndex(ElementUVs, UVMappingMode, UVReferenceMode, VertexID, ControlPointIndex);
                        
                        v3f Position = V3(ControlPoints[ControlPointIndex].Buffer());
                        v3f Normal = V3(ElementNormals->GetDirectArray().GetAt(NormalIndex).Buffer());
                        v2f UV = V2(ElementUVs->GetDirectArray().GetAt(UVIndex).Buffer());
                        
                        vertex_p3_n3_uv Vertex = {Position, Normal, UV};
                        
                        u32 Index;
                        if(VertexMap.Find(Vertex, &Index))
                            IndexData[VertexID] = Index;
                        else
                        {
                            VertexMap.Insert(Vertex, VertexCount);
                            
                            if(JointContext.JointsData)
                            {
                                u8* JointIndices = JointContext.JointsData[ControlPointIndex].Indices;
                                f32* JointWeights = JointContext.JointsData[ControlPointIndex].Weights;
                                
                                control_point_joint_data JointData = JointContext.JointsData[ControlPointIndex];
                                ((vertex_p3_n3_uv_weights*)VertexData)[VertexCount] = 
                                {
                                    Position, Normal, UV, JointIndices[0], JointIndices[1], JointIndices[2], JointIndices[3],
                                    JointWeights[0], JointWeights[1], JointWeights[2], JointWeights[3]
                                };
                            }
                            else
                            {
                                ((vertex_p3_n3_uv*)VertexData)[VertexCount] = Vertex;
                            }
                            
                            IndexData[VertexID] = VertexCount;
                            VertexCount++;
                        }                        
                    }
                }
                
                v3f* Tangents   = PushArray(VertexCount, v3f, Clear, 0);
                v3f* Bitangents = PushArray(VertexCount, v3f, Clear, 0);
                
                for(u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
                {
                    u32 Index0 = IndexData[(TriangleIndex*3)+0];
                    u32 Index1 = IndexData[(TriangleIndex*3)+1];
                    u32 Index2 = IndexData[(TriangleIndex*3)+2];
                    
                    v3f P[3];
                    v2f UV[3];
                    
                    if(JointContext.JointsData)
                    {
                        vertex_p3_n3_uv_weights Vertex0 = ((vertex_p3_n3_uv_weights*)VertexData)[Index0];
                        vertex_p3_n3_uv_weights Vertex1 = ((vertex_p3_n3_uv_weights*)VertexData)[Index1];
                        vertex_p3_n3_uv_weights Vertex2 = ((vertex_p3_n3_uv_weights*)VertexData)[Index2];                                                                        
                        
                        P[0] = Vertex0.P;
                        P[1] = Vertex1.P;
                        P[2] = Vertex2.P;
                        
                        UV[0] = Vertex0.UV;
                        UV[1] = Vertex1.UV;
                        UV[2] = Vertex2.UV;
                    }
                    else
                    {
                        vertex_p3_n3_uv Vertex0 = ((vertex_p3_n3_uv*)VertexData)[Index0];
                        vertex_p3_n3_uv Vertex1 = ((vertex_p3_n3_uv*)VertexData)[Index1];
                        vertex_p3_n3_uv Vertex2 = ((vertex_p3_n3_uv*)VertexData)[Index2];                        
                        
                        P[0] = Vertex0.P;
                        P[1] = Vertex1.P;
                        P[2] = Vertex2.P;
                        
                        UV[0] = Vertex0.UV;
                        UV[1] = Vertex1.UV;
                        UV[2] = Vertex2.UV;
                    }
                    
                    v3f Edge0 = P[1]-P[0];
                    v3f Edge1 = P[2]-P[0];
                    
                    float S0 = UV[1].x - UV[0].x;
                    float S1 = UV[2].x - UV[0].x;
                    float T0 = UV[1].y - UV[0].y;
                    float T1 = UV[2].y - UV[0].y;
                    
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
                
                b32 IsSkeletalMesh = false;
                
                ptr VerticesSize;
                if(JointContext.JointsData)
                {
                    IsSkeletalMesh = true;
                    VerticesSize = VertexCount*sizeof(vertex_p3_n3_t4_uv_weights);
                }
                else
                    VerticesSize = VertexCount*sizeof(vertex_p3_n3_t4_uv);
                
                b32 IsIndexFormat32 = false;
                ptr IndicesSize;
                if(VertexCount >= USHRT_MAX)
                {
                    IsIndexFormat32 = true;
                    IndicesSize = IndexCount*sizeof(u32);
                }
                else
                    IndicesSize = IndexCount*sizeof(u16);
                
                
                ptr MeshSize = sizeof(list_entry<mesh>) + VerticesSize + IndicesSize;
                list_entry<mesh>* MeshLink = (list_entry<mesh>*)PushSize(&AssetBuilder->AssetArena, MeshSize, Clear, 0);
                
                list_entry<mesh_info>* MeshInfoLink = PushStruct(&AssetBuilder->AssetArena, list_entry<mesh_info>, Clear, 0);
                
                mesh* Mesh = &MeshLink->Entry;
                Mesh->Vertices = (u8*)MeshLink + sizeof(list_entry<mesh>);
                Mesh->Indices = (u8*)Mesh->Vertices + VerticesSize;
                
                mesh_info* MeshInfo = &MeshInfoLink->Entry;
                MeshInfo->Header.IsSkeletalMesh = IsSkeletalMesh;
                MeshInfo->Header.IsIndexFormat32 = IsIndexFormat32;
                MeshInfo->Header.VertexCount = VertexCount;
                MeshInfo->Header.IndexCount = IndexCount;
                MeshInfo->Header.ConvexHullCount = MeshContext.ConvexHullNodes.Size;
                MeshInfo->Header.NameLength = (u32)LiteralStringLength(MeshName);
                
                MeshInfo->Name = PushArray(&AssetBuilder->AssetArena, MeshInfo->Header.NameLength+1, char, Clear, 0);
                CopyMemory(MeshInfo->Name, MeshName, sizeof(char)*MeshInfo->Header.NameLength);
                MeshInfo->Name[MeshInfo->Header.NameLength] = 0;
                
                for(u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
                {
                    v3f T = Normalize(Tangents[VertexIndex]);
                    v3f B = Bitangents[VertexIndex];
                    
                    if(IsSkeletalMesh)
                    {
                        vertex_p3_n3_t4_uv_weights* DstVertex = (vertex_p3_n3_t4_uv_weights*)Mesh->Vertices + VertexIndex;
                        vertex_p3_n3_uv_weights* SrcVertex = (vertex_p3_n3_uv_weights*)VertexData + VertexIndex;                                                                        
                        
                        DstVertex->P = SrcVertex->P;
                        DstVertex->N = Normalize(SrcVertex->N);
                        DstVertex->UV = SrcVertex->UV;
                        DstVertex->T = OrthogonalTangent(B, T, DstVertex->N);
                        CopyMemory(DstVertex->JointI, SrcVertex->JointI, sizeof(DstVertex->JointI));
                        CopyMemory(DstVertex->JointW, SrcVertex->JointW, sizeof(DstVertex->JointW));                        
                    }
                    else
                    {
                        vertex_p3_n3_t4_uv* DstVertex = (vertex_p3_n3_t4_uv*)Mesh->Vertices + VertexIndex;
                        vertex_p3_n3_uv* SrcVertex = (vertex_p3_n3_uv*)VertexData + VertexIndex;
                        
                        DstVertex->P = SrcVertex->P;
                        DstVertex->N = Normalize(SrcVertex->N);
                        DstVertex->UV = SrcVertex->UV;
                        DstVertex->T = OrthogonalTangent(B, T, DstVertex->N);                        
                    }
                }
                
                if(IsIndexFormat32)
                {
                    u32* Indices = (u32*)Mesh->Indices;
                    for(u32 Index = 0; Index < IndexCount; Index++) Indices[Index] = IndexData[Index];
                }
                else
                {
                    u16* Indices = (u16*)Mesh->Indices;
                    for(u32 Index = 0; Index < IndexCount; Index++) Indices[Index] =  SafeU16(IndexData[Index]);
                }
                
                v3f MeshPivot = FBX_GetPivot(Node);
                m4  MeshTransform = FBX_GetLocalToParentTransform(Node);                                
                v3f MeshDelta = FBX_GetPivotDelta(MeshPivot, MeshTransform);
                
                m3 MeshModelR = M3(MeshTransform);
                m3 NormalMeshModelR = Transpose(InverseTransformM3(MeshModelR));
                
                for(u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
                {
                    if(IsSkeletalMesh)
                    {
                        vertex_p3_n3_t4_uv_weights* Vertex = (vertex_p3_n3_t4_uv_weights*)Mesh->Vertices + VertexIndex;
                        Vertex->P = Vertex->P*MeshModelR - MeshPivot;
                        Vertex->N = Normalize(Vertex->N*NormalMeshModelR);
                        Vertex->T.xyz = Normalize(Vertex->T.xyz*MeshModelR);
                    }
                    else
                    {
                        vertex_p3_n3_t4_uv* Vertex = (vertex_p3_n3_t4_uv*)Mesh->Vertices + VertexIndex;
                        Vertex->P = Vertex->P*MeshModelR - MeshPivot;
                        Vertex->N = Normalize(Vertex->N*NormalMeshModelR);
                        Vertex->T.xyz = Normalize(Vertex->T.xyz*MeshModelR);                                                
                    }
                }
                
                MeshInfo->ConvexHulls = PushArray(&AssetBuilder->AssetArena, MeshInfo->Header.ConvexHullCount, convex_hull, Clear, 0);                
                for(u32 ConvexHullIndex = 0; ConvexHullIndex < MeshContext.ConvexHullNodes.Size; ConvexHullIndex++)
                {
                    convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
                    
                    FbxNode* ConvexHullNode = MeshContext.ConvexHullNodes[ConvexHullIndex];
                    FbxMesh* ConvexHullMesh = ConvexHullNode->GetMesh();
                    
                    v3f ConvexPivot = FBX_GetPivot(ConvexHullNode);
                    m4  ConvexTransform = FBX_GetLocalToParentTransform(ConvexHullNode);                                
                    v3f ConvexDelta = FBX_GetPivotDelta(ConvexPivot, ConvexTransform);
                    
                    m3 ConvexHullModelR = M3(ConvexTransform);                    
                    
                    v3f NewLocalToParentT = (ConvexTransform.Translation.xyz - MeshDelta) + ConvexDelta;                    
                                        
                    FbxVector4* ConvexHullVertices = ConvexHullMesh->GetControlPoints();
                    
                    ConvexHull->Header.Transform = CreateSQT(NewLocalToParentT);                    
                    ConvexHull->Header.VertexCount = ConvexHullMesh->GetControlPointsCount();
                    ConvexHull->Header.EdgeCount = ConvexHullMesh->GetMeshEdgeCount()*2;
                    ConvexHull->Header.FaceCount = ConvexHullMesh->GetPolygonCount();
                    
                    ConvexHull->Vertices = PushArray(&AssetBuilder->AssetArena, ConvexHull->Header.VertexCount, half_vertex, Clear, 0);
                    ConvexHull->Edges = PushArray(&AssetBuilder->AssetArena, ConvexHull->Header.EdgeCount, half_edge, Clear, 0);
                    ConvexHull->Faces = PushArray(&AssetBuilder->AssetArena, ConvexHull->Header.FaceCount, half_face, Clear, 0);
                    
                    for(u32 VertexIndex = 0; VertexIndex < ConvexHull->Header.VertexCount; VertexIndex++)
                    {
                        ConvexHull->Vertices[VertexIndex].V    = V3(ConvexHullVertices[VertexIndex].Buffer())*ConvexHullModelR - ConvexPivot;
                        ConvexHull->Vertices[VertexIndex].Edge = -1;
                    }
                    
                    for(u32 FaceIndex = 0; FaceIndex < ConvexHull->Header.FaceCount; FaceIndex++)
                        ConvexHull->Faces[FaceIndex].Edge = -1;
                    
                    for(u32 EdgeIndex = 0; EdgeIndex < ConvexHull->Header.EdgeCount; EdgeIndex++)
                    {
                        ConvexHull->Edges[EdgeIndex].Vertex   = -1;
                        ConvexHull->Edges[EdgeIndex].EdgePair = -1;
                        ConvexHull->Edges[EdgeIndex].Face     = -1;
                        ConvexHull->Edges[EdgeIndex].NextEdge = -1;
                    }
                    
                    hash_map<int_pair, i32> EdgeMap = CreateHashMap<int_pair, i32>((ConvexHull->Header.EdgeCount+ConvexHull->Header.VertexCount)*3);
                    u32 EdgeIndex = 0;
                    
                    for(u32 FaceIndex = 0; FaceIndex < ConvexHull->Header.FaceCount; FaceIndex++)
                    {
                        half_face* Face = ConvexHull->Faces + FaceIndex;
                        
                        i32 FaceVertices[3] = 
                        {
                            ConvexHullMesh->GetPolygonVertex(FaceIndex, 0),
                            ConvexHullMesh->GetPolygonVertex(FaceIndex, 1),
                            ConvexHullMesh->GetPolygonVertex(FaceIndex, 2)
                        };
                        
                        i32 FaceEdgeIndices[3] = {-1, -1, -1};
                        for(u32 i = 2, j = 0; j < 3; i = j++)
                        {
                            i32 v0 = FaceVertices[i];
                            i32 v1 = FaceVertices[j];
                            
                            half_vertex* Vertex0 = ConvexHull->Vertices + v0;
                            half_vertex* Vertex1 = ConvexHull->Vertices + v1;
                            
                            i32 e0 = -1;
                            i32 e1 = -1;
                            if(EdgeMap.Find({v0,v1}, &e0))
                            {
                                e1 = ConvexHull->Edges[e0].EdgePair;
                            }
                            else
                            {
                                e0 = EdgeIndex++;
                                e1 = EdgeIndex++;
                                
                                ConvexHull->Edges[e0].EdgePair = e1;
                                ConvexHull->Edges[e1].EdgePair = e0;
                                
                                EdgeMap.Insert({v0, v1}, e0);
                                EdgeMap.Insert({v1, v0}, e1);                
                            }
                            
                            half_edge* Edge0 = ConvexHull->Edges + e0;
                            half_edge* Edge1 = ConvexHull->Edges + e1;
                            
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
                            ConvexHull->Edges[e0].NextEdge = e1;
                        }
                    }
                }
                
                DeleteDynamicArray(&MeshContext.ConvexHullNodes);
                
                AddToList(&AssetBuilder->MeshInfos, MeshInfoLink);
                AddToList(&AssetBuilder->Meshes, MeshLink);
                
                ConsoleLog("Successful processed mesh!");
            }
        }        
    }
    else
    {
        ConsoleError("Failed to initialize the FBX file. Message %s", Importer->GetStatus().GetErrorString());
    }
    
    ConsoleNewLine();
    ConsoleLog("Finished loading fbx file");
    
    Importer->Destroy();
    
    EndTemporaryMemory(&TempArena);
}