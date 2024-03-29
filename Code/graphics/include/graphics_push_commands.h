#ifndef GRAPHICS_PUSH_COMMANDS_H
#define GRAPHICS_PUSH_COMMANDS_H

//CONFIRM(JJ): Is this alright to be fixed sized?
#define MAX_COMMAND_COUNT 8192

enum push_command_type
{
    PUSH_COMMAND_UNKNOWN,
    PUSH_COMMAND_CLEAR_COLOR,
    PUSH_COMMAND_CLEAR_DEPTH,
    PUSH_COMMAND_CLEAR_COLOR_AND_DEPTH,
    PUSH_COMMAND_DEPTH,
    PUSH_COMMAND_CULL,
    PUSH_COMMAND_WIREFRAME,
    PUSH_COMMAND_BLEND,
    PUSH_COMMAND_SCISSOR,        
    PUSH_COMMAND_SRGB_RENDER_BUFFER_WRITES,
    PUSH_COMMAND_VIEWPORT,
    PUSH_COMMAND_PROJECTION,
    PUSH_COMMAND_VIEW_PROJECTION,
    PUSH_COMMAND_VIEW_POSITION,    
    PUSH_COMMAND_SHADOW_MAP,
    PUSH_COMMAND_OMNI_SHADOW_MAP,
    PUSH_COMMAND_RENDER_BUFFER,
    PUSH_COMMAND_LIGHT_BUFFER,
    PUSH_COMMAND_MATERIAL,    
    PUSH_COMMAND_DRAW_MESH,    
    PUSH_COMMAND_DRAW_SKELETON_MESH,
    PUSH_COMMAND_DRAW_UNLIT_MESH,
    PUSH_COMMAND_DRAW_UNLIT_SKELETON_MESH,
    PUSH_COMMAND_DRAW_LINE_MESH,
    PUSH_COMMAND_DRAW_IMGUI_UI,            
    PUSH_COMMAND_COPY_TO_OUTPUT,
    PUSH_COMMAND_COPY_TO_RENDER_BUFFER
};

struct push_command
{
    push_command_type Type;
};

struct push_command_clear_color : public push_command
{
    ak_f32 R, G, B, A;
};

struct push_command_clear_depth : public push_command
{
    ak_f32 Depth;
};

struct push_command_clear_color_and_depth : public push_command
{
    ak_f32 R, G, B, A;
    ak_f32 Depth;
};

struct push_command_depth : public push_command
{
    ak_bool Enable;
};

struct push_command_cull : public push_command
{
    graphics_cull_mode CullMode;
};

struct push_command_wireframe : public push_command
{
    ak_bool Enable;
};

struct push_command_srgb_render_buffer_writes : public push_command
{
    ak_bool Enable;
};

struct push_command_blend : public push_command
{
    ak_bool Enable;
    graphics_blend SrcGraphicsBlend;
    graphics_blend DstGraphicsBlend;
};

struct push_command_rect : public push_command
{ 
    ak_i32 X, Y;
    ak_i32 Width, Height;
};

struct push_command_4x4_matrix : public push_command
{
    ak_m4f Matrix;
};

struct push_command_view_position : public push_command
{
    ak_v3f Position;    
};

struct push_command_omni_shadow_map : public push_command
{        
    ak_f32 FarPlaneDistance;
};

struct push_command_render_buffer : public push_command
{
    graphics_render_buffer* RenderBuffer;
};

struct push_command_light_buffer : public push_command
{    
    graphics_light_buffer LightBuffer;
};

struct push_command_material : public push_command
{
    graphics_material Material;
};

struct push_command_draw_mesh : public push_command
{
    graphics_mesh_id MeshID;
    ak_m4f WorldTransform;    
    graphics_draw_info DrawInfo;    
};

struct push_command_draw_skeleton_mesh : public push_command
{
    graphics_mesh_id MeshID;
    ak_m4f WorldTransform;    
    graphics_draw_info DrawInfo;        
    ak_m4f Joints[MAX_JOINT_COUNT];
};

struct push_command_draw_unlit_mesh : public push_command
{
    graphics_mesh_id MeshID;
    ak_m4f WorldTransform;
    graphics_diffuse_material_slot DiffuseSlot;
    graphics_transparent_material_slot AlphaSlot;
    graphics_draw_info DrawInfo;
};

struct push_command_draw_unlit_skeleton_mesh : public push_command
{
    graphics_mesh_id MeshID;
    ak_m4f WorldTransform;
    graphics_diffuse_material_slot DiffuseSlot;
    graphics_draw_info DrawInfo;    
    ak_m4f Joints[MAX_JOINT_COUNT];
};

struct push_command_draw_line_mesh : public push_command
{
    graphics_mesh_id MeshID;
    ak_m4f WorldTransform;
    ak_color3f Color;
    graphics_draw_info DrawInfo;        
};

struct push_command_draw_imgui_ui : public push_command
{
    graphics_mesh_id MeshID;
    graphics_texture_id TextureID;    
    graphics_draw_info DrawInfo;
};

struct push_command_copy_to_output: public push_command
{
    graphics_render_buffer* RenderBuffer;
    ak_v2i DstOffset;
    ak_v2i DstResolution;
};

struct push_command_copy_to_render_buffer : public push_command
{
    graphics_render_buffer* SrcRenderBuffer;    
    ak_v2i DstOffset;
    ak_v2i DstResolution;
};

#endif