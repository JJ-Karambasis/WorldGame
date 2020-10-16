#include <fbxsdk.h>

struct fbx_skeleton
{
    ak_u32 JointCount;
    FbxNode* Nodes[128];
};

struct fbx_mesh_context
{
    ak_bool ShouldSkip;
    ak_array<FbxNode*> ConvexHullNodes;
    fbx_skeleton Skeleton;    
};

struct control_point_joint_data
{
    ak_u32 Count;
    ak_f32 Weights[4];
    ak_u8 Indices[4];
};

struct fbx_joint_context
{
    ak_bool ShouldSkip;
    control_point_joint_data* JointsData;
};

struct vertex_p3_n3_uv_index
{
    ak_vertex_p3_n3_uv Vertex;
    ak_u32 Index;
};

inline ak_u32 AK_HashFunction(ak_v2f V)
{
    ak_u32 Result = ((AK_HashFunction(V.x) ^ (AK_HashFunction(V.y) << 1)) >> 1);
    return Result;
}

inline ak_u32 AK_HashFunction(ak_v3f V)
{
    ak_u32 Result = AK_HashFunction(V.xy) ^ (AK_HashFunction(V.z) << 1);    
    return Result;
}

inline ak_u32 AK_HashFunction(ak_vertex_p3_n3_uv Vertex)
{
    ak_u32 Result = ((AK_HashFunction(Vertex.P) ^ (AK_HashFunction(Vertex.N) << 1)) >> 1) ^ (AK_HashFunction(Vertex.UV) << 1);
    return Result;
}

inline ak_bool AK_HashCompare(ak_vertex_p3_n3_uv A, ak_vertex_p3_n3_uv B)
{
    ak_bool Result = (A.P == B.P) && (A.N == B.N) && (A.UV == B.UV);
    return Result;
}

global FbxManager* Global_FBXManager = NULL;

inline ak_bool 
FBX_ValidateMappingMode(FbxLayerElement::EMappingMode MappingMode)
{
    ak_bool Result = (MappingMode == FbxGeometryElement::eByPolygonVertex) || (MappingMode == FbxGeometryElement::eByControlPoint);
    return Result;
}

inline ak_bool 
FBX_ValidateReferenceMode(FbxLayerElement::EReferenceMode ReferenceMode)
{
    ak_bool Result = (ReferenceMode == FbxGeometryElement::eDirect) || (ReferenceMode == FbxGeometryElement::eIndexToDirect);
    return Result;
}

inline ak_bool
FBX_HasAttribute(FbxNode* Node, FbxNodeAttribute::EType Type)
{
    for(ak_i32 AttributeIndex = 0; AttributeIndex < Node->GetNodeAttributeCount(); AttributeIndex++)
    {
        FbxNodeAttribute* Attribute = Node->GetNodeAttributeByIndex(AttributeIndex);            
        if(Attribute->GetAttributeType() == Type)
            return true;
    }
    return false;
}

inline ak_bool IsValidConvexHull(FbxNode* Node)
{
    if(!Node->GetMesh()->IsTriangleMesh())
    {
        ConsoleError("Convex hull for mesh is not a triangle mesh. Please triangulate the convex hull manually or via your modeling program's exporter. Skipping Mesh");
        return false;
    }
    
    //TODO(JJ): Need to add a check to determine if your convex hull is actually a convex hull!
    return true;
}

ak_bool AreIdenticalSQTs(ak_sqtf A, ak_sqtf B)
{
    ak_bool Result = (A.Translation == B.Translation) && (A.Orientation == B.Orientation) && (A.Scale == B.Scale);
    return Result;
}

ak_bool AreIdenticalVertices(ak_vertex_p3_n3_uv A, ak_vertex_p3_n3_uv B)
{
    ak_bool Result = (A.P == B.P) && (A.N == B.N) && (A.UV == B.UV);
    return Result;
}

ak_bool AreIdenticalVertices(ak_vertex_p3_n3_uv_w A, ak_vertex_p3_n3_uv_w B)
{
    ak_bool Result = ((A.P == B.P) && (A.N == B.N) && (A.UV == B.UV) &&
                      (A.JointI[0] == B.JointI[0]) && (A.JointI[1] == B.JointI[1]) && (A.JointI[2] == B.JointI[2]) && (A.JointI[3] == B.JointI[3]) &&
                      (A.JointW[0] == B.JointW[0]) && (A.JointW[1] == B.JointW[1]) && (A.JointW[2] == B.JointW[2]) && (A.JointW[3] == B.JointW[3]));
    return Result;
}

ak_bool AreConvexHullsIdentical(convex_hull* NewConvexHull, convex_hull* OldConvexHull)
{
    if(!AreIdenticalSQTs(NewConvexHull->Header.Transform, OldConvexHull->Header.Transform))   return false;
    if(NewConvexHull->Header.VertexCount != OldConvexHull->Header.VertexCount) return false;
    if(NewConvexHull->Header.EdgeCount   != OldConvexHull->Header.EdgeCount)   return false;
    if(NewConvexHull->Header.FaceCount   != OldConvexHull->Header.FaceCount)   return false;
    
    for(ak_u32 VertexIndex = 0; VertexIndex < NewConvexHull->Header.VertexCount; VertexIndex++)
    {
        half_vertex* NewVertex = NewConvexHull->Vertices + VertexIndex;
        half_vertex* OldVertex = OldConvexHull->Vertices + VertexIndex;        
        if(*NewVertex != *OldVertex) return false;
    }
    
    for(ak_u32 EdgeIndex = 0; EdgeIndex < NewConvexHull->Header.EdgeCount; EdgeIndex++)
    {
        half_edge* NewEdge = NewConvexHull->Edges + EdgeIndex;
        half_edge* OldEdge = OldConvexHull->Edges + EdgeIndex;
        if(*NewEdge != *OldEdge) return false;
    }
    
    for(ak_u32 FaceIndex = 0; FaceIndex < NewConvexHull->Header.FaceCount; FaceIndex++)
    {
        half_face* NewFace = NewConvexHull->Faces + FaceIndex;
        half_face* OldFace = OldConvexHull->Faces + FaceIndex;
        if(*NewFace != *OldFace) return false;
    }
    
    return true;
}

ak_bool AreMeshesIdentical(mesh* NewMesh, mesh_info* NewMeshInfo, mesh* OldMesh, mesh_info* OldMeshInfo)
{
    if(NewMeshInfo->Header.IsSkeletalMesh  != OldMeshInfo->Header.IsSkeletalMesh)  return false;
    if(NewMeshInfo->Header.IsIndexFormat32 != OldMeshInfo->Header.IsIndexFormat32) return false;
    if(NewMeshInfo->Header.VertexCount     != OldMeshInfo->Header.VertexCount)     return false;
    if(NewMeshInfo->Header.IndexCount      != OldMeshInfo->Header.IndexCount)      return false;
    if(NewMeshInfo->Header.ConvexHullCount != OldMeshInfo->Header.ConvexHullCount) return false;
    
    for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < NewMeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
    {
        convex_hull* NewConvexHull = NewMeshInfo->ConvexHulls + ConvexHullIndex;
        convex_hull* OldConvexHull = OldMeshInfo->ConvexHulls + ConvexHullIndex;        
        if(!AreConvexHullsIdentical(NewConvexHull, OldConvexHull)) return false;
    }
    
    for(ak_u32 VertexIndex = 0; VertexIndex < NewMeshInfo->Header.VertexCount; VertexIndex++)
    {
        if(NewMeshInfo->Header.IsSkeletalMesh)
        {
            ak_vertex_p3_n3_uv_w* NewVertex = (ak_vertex_p3_n3_uv_w*)NewMesh->Vertices + VertexIndex;
            ak_vertex_p3_n3_uv_w* OldVertex = (ak_vertex_p3_n3_uv_w*)OldMesh->Vertices + VertexIndex;            
            if(!AreIdenticalVertices(*NewVertex, *OldVertex)) return false;                
        }
        else
        {
            ak_vertex_p3_n3_uv* NewVertex = (ak_vertex_p3_n3_uv*)NewMesh->Vertices + VertexIndex;
            ak_vertex_p3_n3_uv* OldVertex = (ak_vertex_p3_n3_uv*)OldMesh->Vertices + VertexIndex;
            if(!AreIdenticalVertices(*NewVertex, *OldVertex)) return false;
        }
    }
    
    for(ak_u32 Index = 0; Index < NewMeshInfo->Header.IndexCount; Index++)
    {
        if(NewMeshInfo->Header.IsIndexFormat32)
        {
            ak_u32* NewIndex = (ak_u32*)NewMesh->Indices + Index;
            ak_u32* OldIndex = (ak_u32*)OldMesh->Indices + Index;            
            if(*NewIndex != *OldIndex) return false;
        }
        else
        {
            ak_u16* NewIndex = (ak_u16*)NewMesh->Indices + Index;
            ak_u16* OldIndex = (ak_u16*)OldMesh->Indices + Index;
            if(*NewIndex != *OldIndex) return false;
        }
    }
    
    return true;
}

fbx_mesh_context FBX_GetMeshContext(FbxNode* Node)
{   
    fbx_mesh_context MeshContext = {};    
    
    ak_u32 ChildCount = Node->GetChildCount();
    for(ak_u32 ChildIndex = 0; ChildIndex < ChildCount; ChildIndex++)
    {
        FbxNode* ChildNode = Node->GetChild(ChildIndex);
        const ak_char* ChildName = ChildNode->GetName();
        
        if(FBX_HasAttribute(ChildNode, FbxNodeAttribute::eMesh) && AK_StringEndsWith(ChildName, "_ConvexHull"))
        {   
            if(!IsValidConvexHull(ChildNode))
            {
                MeshContext.ShouldSkip = true;
                return MeshContext;
            }
            
            MeshContext.ConvexHullNodes.Add(ChildNode);
        }
        
        if(FBX_HasAttribute(ChildNode, FbxNodeAttribute::eSkeleton))
        {
            ak_u32 StackCount = 0;
            FbxNode* NodeStack[256];
            
            NodeStack[StackCount++] = ChildNode;
            
            while(StackCount > 0)
            {
                FbxNode* StackNode = NodeStack[StackCount--];    
                
                if(MeshContext.Skeleton.JointCount == AK_Count(MeshContext.Skeleton.Nodes))
                {
                    ConsoleError("Skeleton nodes exceeds the maximum value for a skeletal mesh (128). Skipping Mesh");
                    MeshContext.ShouldSkip = true;                    
                    return MeshContext;                    
                }
                
                MeshContext.Skeleton.Nodes[MeshContext.Skeleton.JointCount++] = StackNode;                            
                
                ak_u32 StackNodeChildCount = StackNode->GetChildCount();
                for(ak_u32 StackNodeIndex = 0; StackNodeIndex < StackNodeChildCount; StackNodeIndex++)
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

ak_u8 FBX_FindJoint(fbx_skeleton* Skeleton, const ak_char* JointName)
{
    AK_Assert(Skeleton->JointCount != (ak_u8)-1, "Invalid skeleton");
    for(ak_u8 JointIndex = 0; JointIndex < Skeleton->JointCount; JointIndex++)
    {
        FbxNode* Node = Skeleton->Nodes[JointIndex];
        if(AK_StringEquals(Node->GetName(), JointName))
            return JointIndex;
    }
    
    return (ak_u8)-1;
}

fbx_joint_context FBX_GetJointContext(FbxMesh* Mesh, fbx_mesh_context* MeshContext)
{        
    ak_u32 ControlPointCount = Mesh->GetControlPointsCount();
    FbxVector4* ControlPoints = Mesh->GetControlPoints();
    
    fbx_joint_context Result = {};
    
    ak_u32 SkinCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
    
    for(ak_u32 SkinIndex = 0; SkinIndex < SkinCount; SkinIndex++)
    {                    
        FbxSkin* Skin = (FbxSkin*)Mesh->GetDeformer(SkinIndex, FbxDeformer::eSkin);
        ak_u32 ClusterCount = Skin->GetClusterCount();
        for(ak_u32 ClusterIndex = 0; ClusterIndex < ClusterCount; ClusterIndex++)
        {
            FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
            FbxNode* Link = Cluster->GetLink();
            if(Link)
            {
                ak_i8 JointIndex = FBX_FindJoint(&MeshContext->Skeleton, Link->GetName());
                if(JointIndex != -1)
                {
                    ak_u32 IndexCount = Cluster->GetControlPointIndicesCount();
                    ak_i32* Indices = Cluster->GetControlPointIndices();
                    ak_f64* Weights = Cluster->GetControlPointWeights();
                    
                    for(ak_u32 Index = 0; Index < IndexCount; Index++)
                    {                                                
                        if(!Result.JointsData)
                        {
                            ak_arena* GlobalArena = AK_GetGlobalArena();
                            Result.JointsData = GlobalArena->PushArray<control_point_joint_data>(ControlPointCount);
                        }
                        
                        ak_i32 ControlPointIndex = Indices[Index];
                        
                        control_point_joint_data* JointData = Result.JointsData + ControlPointIndex;
                        if(JointData->Count >= 4)
                        {                                        
                            ConsoleError("A control point has more than 4 binded joints. This is invalid. Skipping Mesh");
                            return Result;
                        }
                        
                        JointData->Weights[JointData->Count] = (ak_f32)Weights[Index];
                        JointData->Indices[JointData->Count] = JointIndex;                        
                        JointData->Count++;
                    }
                }
            }
        }
    }
    
    if(Result.JointsData)
    {
        
        for(ak_u32 ControlPointIndex = 0; ControlPointIndex < ControlPointCount; ControlPointIndex++)
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
ak_i32 GetElementIndex(FbxLayerElementTemplate<type>* GeometryElement, FbxLayerElement::EMappingMode MappingMode, FbxLayerElement::EReferenceMode ReferenceMode, 
                       ak_i32 VertexID, ak_i32 ControlPointID)
{
    ak_i32 Result = -1;
    
    if(MappingMode == FbxGeometryElement::eByPolygonVertex)
    {
        if(ReferenceMode == FbxGeometryElement::eDirect)                    
            Result = VertexID;                
        else if(ReferenceMode == FbxGeometryElement::eIndexToDirect)                
            Result = GeometryElement->GetIndexArray().GetAt(VertexID);                
        AK_INVALID_ELSE;
    }
    else if(MappingMode == FbxGeometryElement::eByControlPoint)
    {
        if(ReferenceMode == FbxGeometryElement::eDirect)
            Result = ControlPointID;
        else if(ReferenceMode == FbxGeometryElement::eIndexToDirect)
            Result = GeometryElement->GetIndexArray().GetAt(ControlPointID);
        AK_INVALID_ELSE;
    }
    AK_INVALID_ELSE;
    
    return Result;
}

inline ak_v3f
FBX_GetPivot(FbxNode* Node)
{    
    ak_v3f Result = AK_V3f(Node->GetRotationPivot(FbxNode::eSourcePivot).Buffer());
    return Result;
}

inline ak_m4f
FBX_GetLocalToParentTransform(FbxNode* Node)
{
    ak_m4f Result = AK_M4f(Node->EvaluateLocalTransform());
    return Result;
}

inline ak_v3f
FBX_GetLocalTranslation(FbxNode* Node)
{
    ak_v3f Result = AK_V3f(Node->EvaluateLocalTranslation());
    return Result;
}

inline ak_m4f 
FBX_GetGlobalTransform(FbxNode* Node)
{
    ak_m4f Result = AK_M4f(Node->EvaluateGlobalTransform());
    return Result;
}

inline ak_v3f
FBX_GetPivotDelta(ak_v3f Pivot, ak_m3f Transform)
{
    ak_v3f Delta = (Pivot.x*Transform.XAxis) + (Pivot.y*Transform.YAxis) + (Pivot.z*Transform.ZAxis);
    return Delta;
}

void ParseFBX(asset_builder* AssetBuilder, ak_string Path)
{
    if(!Global_FBXManager)
    {
        Global_FBXManager = FbxManager::Create();
        FbxIOSettings* IOSettings = FbxIOSettings::Create(Global_FBXManager, IOSROOT);
        Global_FBXManager->SetIOSettings(IOSettings);
    }
    
    ak_arena* GlobalArena = AK_GetGlobalArena();    
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    FbxImporter* Importer = FbxImporter::Create(Global_FBXManager, "");
    
    ConsoleLog("Loading FBX File %s", Path.Data);
    if(Importer->Initialize(Path.Data, -1, Global_FBXManager->GetIOSettings()))
    {
        ak_i32 MajorVersion, MinorVersion, RevisionVersion;
        Importer->GetFileVersion(MajorVersion, MinorVersion, RevisionVersion);        
        ConsoleLog("Version %d.%d.%d", MajorVersion, MinorVersion, RevisionVersion);
        
        FbxScene* Scene = FbxScene::Create(Global_FBXManager, "");
        Importer->Import(Scene);
        
        FbxNode* RootNode = Scene->GetRootNode();
        
        ak_u32 RootChildCount = RootNode->GetChildCount();
        ak_hash_map<ak_vertex_p3_n3_uv, ak_u32> VertexMap = AK_CreateHashMap<ak_vertex_p3_n3_uv, ak_u32>(8191);
        ak_hash_map<ak_pair<ak_i32>, ak_i32> HalfEdgeMap = AK_CreateHashMap<ak_pair<ak_i32>, ak_i32>(8191);
        
        for(ak_u32 RootChildIndex = 0; RootChildIndex < RootChildCount; RootChildIndex++)
        {
            FbxNode* Node = RootNode->GetChild(RootChildIndex);
            
            if(FBX_HasAttribute(Node, FbxNodeAttribute::eMesh))
            {
                FbxMesh* MeshFBX = Node->GetMesh();
                const ak_char* MeshName = Node->GetName();
                
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
                if(MeshContext.ShouldSkip) { AK_DeleteArray(&MeshContext.ConvexHullNodes); continue; }
                
                ConsoleLog("Mesh has %d Convex Hulls", MeshContext.ConvexHullNodes.Size);
                
                fbx_joint_context JointContext = FBX_GetJointContext(MeshFBX, &MeshContext);
                if(JointContext.ShouldSkip) { AK_DeleteArray(&MeshContext.ConvexHullNodes); continue; }                                
                
                ak_u32 TriangleCount = MeshFBX->GetPolygonCount();                
                ak_u32 IndexCount = TriangleCount*3;
                ak_u32 VertexCount = 0;
                
                ak_u32* IndexData = GlobalArena->PushArray<ak_u32>(IndexCount);
                VertexMap.Reset();                
                
                void* VertexData;
                if(JointContext.JointsData)
                    VertexData = GlobalArena->PushArray<ak_vertex_p3_n3_uv_w>(IndexCount);
                else
                    VertexData = GlobalArena->PushArray<ak_vertex_p3_n3_uv>(IndexCount);
                
                FbxVector4* ControlPoints = MeshFBX->GetControlPoints();                                
                for(ak_u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
                {
                    for(ak_u32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
                    {
                        ak_u32 VertexID = (TriangleIndex*3)+VertexIndex;                        
                        ak_i32 ControlPointIndex = MeshFBX->GetPolygonVertex(TriangleIndex, VertexIndex);
                        ak_i32 NormalIndex = GetElementIndex(ElementNormals, NormalMappingMode, NormalReferenceMode, VertexID, ControlPointIndex);
                        ak_i32 UVIndex = GetElementIndex(ElementUVs, UVMappingMode, UVReferenceMode, VertexID, ControlPointIndex);
                        
                        ak_v3f Position = AK_V3f(ControlPoints[ControlPointIndex].Buffer());
                        ak_v3f Normal = AK_V3f(ElementNormals->GetDirectArray().GetAt(NormalIndex).Buffer());
                        ak_v2f UV = AK_V2f(ElementUVs->GetDirectArray().GetAt(UVIndex).Buffer());
                        
                        ak_vertex_p3_n3_uv Vertex = {Position, Normal, UV};
                        
                        
                        ak_u32* Index = VertexMap.Find(Vertex);                       
                        if(Index)
                            IndexData[VertexID] = *Index;
                        else
                        {
                            VertexMap.Insert(Vertex, VertexCount);
                            
                            if(JointContext.JointsData)
                            {
                                ak_u8* JointIndices = JointContext.JointsData[ControlPointIndex].Indices;
                                ak_f32* JointWeights = JointContext.JointsData[ControlPointIndex].Weights;
                                
                                control_point_joint_data JointData = JointContext.JointsData[ControlPointIndex];
                                ((ak_vertex_p3_n3_uv_w*)VertexData)[VertexCount] = 
                                {
                                    Position, Normal, UV, JointIndices[0], JointIndices[1], JointIndices[2], JointIndices[3],
                                    JointWeights[0], JointWeights[1], JointWeights[2], JointWeights[3]
                                };
                            }
                            else
                            {
                                ((ak_vertex_p3_n3_uv*)VertexData)[VertexCount] = Vertex;
                            }
                            
                            IndexData[VertexID] = VertexCount;
                            VertexCount++;
                        }                        
                    }
                }
                
                ak_bool IsSkeletalMesh = false;
                
                ak_u32 VerticesSize;
                if(JointContext.JointsData)
                {
                    IsSkeletalMesh = true;
                    VerticesSize = VertexCount*sizeof(ak_vertex_p3_n3_uv_w);
                }
                else
                    VerticesSize = VertexCount*sizeof(ak_vertex_p3_n3_uv);
                
                ak_bool IsIndexFormat32 = false;
                ak_u32 IndicesSize;
                if(VertexCount >= USHRT_MAX)
                {
                    IsIndexFormat32 = true;
                    IndicesSize = IndexCount*sizeof(ak_u32);
                }
                else
                    IndicesSize = IndexCount*sizeof(ak_u16);
                
                
                ak_u32 MeshSize = sizeof(ak_link_entry<mesh>) + VerticesSize + IndicesSize;
                ak_link_entry<mesh>* MeshLink = (ak_link_entry<mesh>*)AssetBuilder->AssetArena->Push(MeshSize);
                
                ak_link_entry<mesh_info>* MeshInfoLink = AssetBuilder->AssetArena->Push<ak_link_entry<mesh_info>>();
                
                mesh* Mesh = &MeshLink->Entry;
                Mesh->Vertices = (ak_u8*)MeshLink + sizeof(ak_link_entry<mesh>);
                Mesh->Indices = (ak_u8*)Mesh->Vertices + VerticesSize;
                
                mesh_info* MeshInfo = &MeshInfoLink->Entry;
                MeshInfo->Header.IsSkeletalMesh = IsSkeletalMesh;
                MeshInfo->Header.IsIndexFormat32 = IsIndexFormat32;
                MeshInfo->Header.VertexCount = VertexCount;
                MeshInfo->Header.IndexCount = IndexCount;
                MeshInfo->Header.ConvexHullCount = MeshContext.ConvexHullNodes.Size;
                MeshInfo->Header.NameLength = (ak_u32)AK_StringLength(MeshName);
                
                MeshInfo->Name = AssetBuilder->AssetArena->PushArray<char>(MeshInfo->Header.NameLength+1);
                AK_MemoryCopy(MeshInfo->Name, MeshName, sizeof(char)*MeshInfo->Header.NameLength);
                MeshInfo->Name[MeshInfo->Header.NameLength] = 0;
                
                ak_v3f MeshPivot = FBX_GetPivot(Node);
                ak_m4f MeshTransform = FBX_GetGlobalTransform(Node);                                
                ak_m3f MeshModelR = AK_M3(MeshTransform);
                
                ak_v3f MeshDelta = FBX_GetPivotDelta(MeshPivot, MeshModelR);
                                
                ak_m3f InvMeshModelR = AK_InvTransformM3(MeshModelR);
                ak_m3f NormalMeshModelR = AK_Transpose(InvMeshModelR);
                
                for(ak_u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
                {                                                            
                    if(IsSkeletalMesh)
                    {
                        ak_vertex_p3_n3_uv_w* DstVertex = (ak_vertex_p3_n3_uv_w*)Mesh->Vertices + VertexIndex;
                        ak_vertex_p3_n3_uv_w* SrcVertex = (ak_vertex_p3_n3_uv_w*)VertexData + VertexIndex;                                                                        
                        
                        *DstVertex = *SrcVertex;                        
                        DstVertex->P = SrcVertex->P*MeshModelR - MeshPivot;                        
                        DstVertex->N = AK_Normalize(DstVertex->N*NormalMeshModelR);                        
                    }
                    else
                    {
                        ak_vertex_p3_n3_uv* DstVertex = (ak_vertex_p3_n3_uv*)Mesh->Vertices + VertexIndex;
                        ak_vertex_p3_n3_uv* SrcVertex = (ak_vertex_p3_n3_uv*)VertexData + VertexIndex;
                        
                        *DstVertex = *SrcVertex;
                        DstVertex->P = SrcVertex->P*MeshModelR - MeshPivot;                        
                        DstVertex->N = AK_Normalize(DstVertex->N*NormalMeshModelR);                                       
                    }
                }
                
                if(IsIndexFormat32)
                {
                    ak_u32* Indices = (ak_u32*)Mesh->Indices;
                    for(ak_u32 Index = 0; Index < IndexCount; Index++) Indices[Index] = IndexData[Index];
                }
                else
                {
                    ak_u16* Indices = (ak_u16*)Mesh->Indices;
                    for(ak_u32 Index = 0; Index < IndexCount; Index++) Indices[Index] =  AK_SafeU16(IndexData[Index]);
                }
                
                MeshInfo->ConvexHulls = AssetBuilder->AssetArena->PushArray<convex_hull>(MeshInfo->Header.ConvexHullCount);
                for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshContext.ConvexHullNodes.Size; ConvexHullIndex++)
                {
                    convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
                    
                    FbxNode* ConvexHullNode = MeshContext.ConvexHullNodes[ConvexHullIndex];
                    FbxMesh* ConvexHullMesh = ConvexHullNode->GetMesh();
                    
                    ak_v3f LocalTranslation = FBX_GetLocalTranslation(ConvexHullNode);
                    ak_m4f  GlobalTransform = FBX_GetGlobalTransform(ConvexHullNode);
                    ak_m3f GlobalTransformR = AK_M3(GlobalTransform);
                    ak_v3f ConvexPivot = FBX_GetPivot(ConvexHullNode);
                    
                    ak_m3f NormGlobalTransformR = AK_NormalizeM3(GlobalTransformR);                    
                    ak_v3f ConvexDelta = FBX_GetPivotDelta(ConvexPivot, NormGlobalTransformR);                     
                    ak_v3f NewLocalToParentT = (LocalTranslation+ConvexDelta) - MeshPivot;                             
                    
                    FbxVector4* ConvexHullVertices = ConvexHullMesh->GetControlPoints();
                    
                    ConvexHull->Header.Transform = AK_SQT(NewLocalToParentT);                    
                    ConvexHull->Header.VertexCount = ConvexHullMesh->GetControlPointsCount();
                    ConvexHull->Header.EdgeCount = ConvexHullMesh->GetMeshEdgeCount()*2;
                    ConvexHull->Header.FaceCount = ConvexHullMesh->GetPolygonCount();
                    
                    ConvexHull->Vertices = AssetBuilder->AssetArena->PushArray<half_vertex>(ConvexHull->Header.VertexCount);
                    ConvexHull->Edges = AssetBuilder->AssetArena->PushArray<half_edge>(ConvexHull->Header.EdgeCount);
                    ConvexHull->Faces = AssetBuilder->AssetArena->PushArray<half_face>(ConvexHull->Header.FaceCount);
                    
                    for(ak_u32 VertexIndex = 0; VertexIndex < ConvexHull->Header.VertexCount; VertexIndex++)
                    {
                        ConvexHull->Vertices[VertexIndex].V    = AK_V3f(ConvexHullVertices[VertexIndex].Buffer())*GlobalTransformR - ConvexDelta;
                        ConvexHull->Vertices[VertexIndex].Edge = -1;
                    }
                    
                    for(ak_u32 FaceIndex = 0; FaceIndex < ConvexHull->Header.FaceCount; FaceIndex++)
                        ConvexHull->Faces[FaceIndex].Edge = -1;
                    
                    for(ak_u32 EdgeIndex = 0; EdgeIndex < ConvexHull->Header.EdgeCount; EdgeIndex++)
                    {
                        ConvexHull->Edges[EdgeIndex].Vertex   = -1;
                        ConvexHull->Edges[EdgeIndex].EdgePair = -1;
                        ConvexHull->Edges[EdgeIndex].Face     = -1;
                        ConvexHull->Edges[EdgeIndex].NextEdge = -1;
                    }
                    
                    HalfEdgeMap.Reset();                    
                    ak_u32 EdgeIndex = 0;
                    
                    for(ak_u32 FaceIndex = 0; FaceIndex < ConvexHull->Header.FaceCount; FaceIndex++)
                    {
                        half_face* Face = ConvexHull->Faces + FaceIndex;
                        
                        ak_i32 FaceVertices[3] = 
                        {
                            ConvexHullMesh->GetPolygonVertex(FaceIndex, 0),
                            ConvexHullMesh->GetPolygonVertex(FaceIndex, 1),
                            ConvexHullMesh->GetPolygonVertex(FaceIndex, 2)
                        };
                        
                        ak_i32 FaceEdgeIndices[3] = {-1, -1, -1};
                        for(ak_u32 i = 2, j = 0; j < 3; i = j++)
                        {
                            ak_i32 v0 = FaceVertices[i];
                            ak_i32 v1 = FaceVertices[j];
                            
                            half_vertex* Vertex0 = ConvexHull->Vertices + v0;
                            half_vertex* Vertex1 = ConvexHull->Vertices + v1;
                            
                            ak_i32 e0 = -1;
                            ak_i32 e1 = -1;
                            
                            ak_i32* pe0 = HalfEdgeMap.Find({v0,v1});
                            if(pe0)
                            {
                                e0 = *pe0;
                                e1 = ConvexHull->Edges[e0].EdgePair;
                            }
                            else
                            {
                                e0 = EdgeIndex++;
                                e1 = EdgeIndex++;
                                
                                ConvexHull->Edges[e0].EdgePair = e1;
                                ConvexHull->Edges[e1].EdgePair = e0;
                                
                                HalfEdgeMap.Insert({v0, v1}, e0);
                                HalfEdgeMap.Insert({v1, v0}, e1);                
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
                        
                        for(ak_u32 i = 2, j = 0; j < 3; i = j++)
                        {
                            ak_i32 e0 = FaceEdgeIndices[i];
                            ak_i32 e1 = FaceEdgeIndices[j];            
                            ConvexHull->Edges[e0].NextEdge = e1;
                        }
                    }
                }
                
                AK_DeleteArray(&MeshContext.ConvexHullNodes);
                
                mesh_pair* Pair = AssetBuilder->MeshTable.Find(MeshInfo->Name);
                if(Pair)
                {
                    if(!AreMeshesIdentical(Mesh, MeshInfo, Pair->Mesh, Pair->MeshInfo))
                    {
                        ak_bool OverrideMesh = ConsoleTrueFalse("Duplicate meshes found with the same name are not identical. Override the mesh");
                        if(OverrideMesh)
                        {
                            ak_link_entry<mesh_info>* OldMeshInfo = (ak_link_entry<mesh_info>*)Pair->MeshInfo;
                            ak_link_entry<mesh>* OldMesh = (ak_link_entry<mesh>*)Pair->Mesh;
                            
                            AssetBuilder->MeshInfos.Remove(OldMeshInfo);
                            AssetBuilder->Meshes.Remove(OldMesh);
                            
                            AssetBuilder->MeshInfos.Push(MeshInfoLink);
                            AssetBuilder->Meshes.Push(MeshLink);                            
                        }
                    }
                }
                else
                {
                    mesh_pair MeshPair = {MeshInfo, Mesh};
                    AssetBuilder->MeshTable.Insert(MeshInfo->Name, MeshPair);
                    
                    AssetBuilder->MeshInfos.Push(MeshInfoLink);
                    AssetBuilder->Meshes.Push(MeshLink);                    
                }
                
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
    
    GlobalArena->EndTemp(&TempArena);    
}