#ifndef DEV_CONTEXT_H
#define DEV_CONTEXT_H

struct dev_input
{
    union
    {
        button Buttons[9];
        struct
        {
            button ToggleDevState;
            button Alt;
            button LMB;
            button MMB;
            button W;
            button E;
            button R;            
            button Delete;
            button Ctl;
        };
    };
    
    ak_v2i MouseDelta;
    ak_v2i MouseCoordinates;
    ak_f32 Scroll;
};

struct dev_mesh
{    
    graphics_mesh_id MeshID;
    ak_u32 IndexCount;
    ak_u32 VertexCount;
    ak_u16 IndexOffSet;
    void* Vertices;
    void* Indices;    
};

struct dev_slim_mesh
{
    graphics_mesh_id MeshID;
    ak_u32 IndexCount;
};

struct dev_capsule_mesh
{    
    graphics_mesh_id MeshID;
    ak_u32 CapIndexCount;
    ak_u32 CapVertexCount;
    ak_u32 BodyIndexCount;    
};

struct dev_context
{
    ak_arena* DevStorage;
    game* Game;
    graphics* Graphics;
    void* PlatformWindow;
    platform_development_update* PlatformUpdate;
    
    dev_input DevInput;
    dev_ui DevUI;
    
    dev_capsule_mesh LineCapsuleMesh;                
    dev_slim_mesh    LineBoxMesh;
    dev_slim_mesh    LineSphereMesh;    
    dev_slim_mesh    TriangleBoxMesh;
    dev_slim_mesh    TriangleSphereMesh;
    dev_slim_mesh    TriangleCylinderMesh;
    dev_slim_mesh    TriangleConeMesh;    
    dev_mesh         TriangleArrowMesh;
    dev_mesh         TriangleCircleMesh;
    dev_mesh         TriangleScaleMesh;
    dev_mesh         TriangleTorusMesh;    
    dev_mesh         TrianglePlaneMesh;
};

void DevContext_Initialize(game* Game, graphics* Graphics, void* PlatformWindow, platform_init_imgui* InitImGui, platform_development_update* PlatformUpdate);
void DevContext_DebugLog(const ak_char* Format, ...);

#endif