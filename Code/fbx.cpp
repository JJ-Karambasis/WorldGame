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
            FBX_AddNode(&Result.MeshNodes, Node);        
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

mesh FBX_LoadFirstMesh(fbx_context* Context, arena* Storage)
{
    if(Context->MeshNodes.Count == 0)
        return {};
    
    FbxNode* Node = Context->MeshNodes.Ptr[0];
    
    m4 Transform = M4(Node->EvaluateGlobalTransform());
    m3 NormalTransform = M3(Transpose(InverseTransformM4(Transform)));
        
    FbxMesh* Mesh = Node->GetMesh();
    
    FbxGeometryElementNormal* ElementNormals = Mesh->GetElementNormal(0);
    BOOL_CHECK_AND_HANDLE(ElementNormals, "Mesh did not have any normals.");                        
    
    FbxLayerElement::EMappingMode MappingMode = ElementNormals->GetMappingMode();
    BOOL_CHECK_AND_HANDLE((MappingMode == FbxGeometryElement::eByPolygonVertex) || (MappingMode == FbxGeometryElement::eByControlPoint),
                          "Mapping mode for normals is not supported. Must be by polygon vertex or control point.");                                                                              
    
    FbxLayerElement::EReferenceMode ReferenceMode = ElementNormals->GetReferenceMode();
    BOOL_CHECK_AND_HANDLE((ReferenceMode == FbxGeometryElement::eDirect) || (ReferenceMode == FbxGeometryElement::eIndexToDirect),
                          "Reference mode for normals is not supported. Must be direct or index to direct.");
    
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
    
#define FBX_HASH_TABLE_SIZE 8101
    hash_table<vertex_p3_n3_index> HashTable = CreateHashTable<vertex_p3_n3_index>(FBX_HASH_TABLE_SIZE);     
    
    void* VertexData;
    if(JointsData)
        VertexData = PushArray(IndexCount, vertex_p3_n3_weights, Clear, 0);
    else
        VertexData = PushArray(IndexCount, vertex_p3_n3, Clear, 0);
    
    for(u32 PolygonIndex = 0; PolygonIndex < PolygonCount; PolygonIndex++)
    {
        BOOL_CHECK_AND_HANDLE(Mesh->GetPolygonSize(PolygonIndex) == 3, "Mesh had a polygon that did not have 3 vertices. All polygons must be triangles.");
        
        for(u32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
        {
            u32 VertexId = (PolygonIndex*3) + VertexIndex;
            
            i32 ControlPointIndex = Mesh->GetPolygonVertex(PolygonIndex, VertexIndex);
            i32 NormalIndex = -1;
            
            if(MappingMode == FbxGeometryElement::eByPolygonVertex)
            {
                if(ReferenceMode == FbxGeometryElement::eDirect)                    
                    NormalIndex = VertexId;                
                else if(ReferenceMode == FbxGeometryElement::eIndexToDirect)                
                    NormalIndex = ElementNormals->GetIndexArray().GetAt(VertexId);                
                INVALID_ELSE;
            }
            else if(MappingMode == FbxGeometryElement::eByControlPoint)
            {
                if(ReferenceMode == FbxGeometryElement::eDirect)
                    NormalIndex = ControlPointIndex;
                else if(ReferenceMode == FbxGeometryElement::eIndexToDirect)
                    NormalIndex = ElementNormals->GetIndexArray().GetAt(ControlPointIndex);
                INVALID_ELSE;
                    
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
                if(JointsData)
                {
                    control_point_joint_data JointData = JointsData[ControlPointIndex];
                    ((vertex_p3_n3_weights*)VertexData)[VertexCount] = 
                    {
                        V3(V4(Vertex.P, 1.0f)*Transform), Vertex.N*NormalTransform, 
                        JointData.Indices[0], JointData.Indices[1], JointData.Indices[2], JointData.Indices[3], 
                        (f32)JointData.Weights[0], (f32)JointData.Weights[1], (f32)JointData.Weights[2], (f32)JointData.Weights[3]
                    };
                }
                else
                {
                    ((vertex_p3_n3*)VertexData)[VertexCount] = {V3(V4(Vertex.P, 1.0f)*Transform), Vertex.N*NormalTransform};                                                                                       
                }
                
                IndexData[VertexId] = VertexCount;                                    
                VertexCount++;                                    
            }                                
        }                            
    }                                                
    
    Result.VertexCount = VertexCount;    
    if(JointsData)
    {
        Result.VertexFormat = GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS;
        Result.Vertices = PushWriteArray(Storage, VertexData, VertexCount, vertex_p3_n3_weights, 0);
        
    }
    else
    {
        Result.VertexFormat = GRAPHICS_VERTEX_FORMAT_P3_N3;
        Result.Vertices = PushWriteArray(Storage, VertexData, VertexCount, vertex_p3_n3, 0);
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
            
            Frame->JointPoses[JointIndex].Translation = SQT.Position;
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