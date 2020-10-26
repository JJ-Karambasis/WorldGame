#ifndef DEV_TOOLS_H
#define DEV_TOOLS_H

#if DEVELOPER_BUILD

global struct dev_context* __Internal_Dev_Context__;
#define Dev_SetDeveloperContext(context) __Internal_Dev_Context__ = (dev_context*)context
#define Dev_GetDeveloperContext() (dev_context*)__Internal_Dev_Context__
#define Dev_ShouldPlayGame() (((dev_context*)__Internal_Dev_Context__)->DevUI.PlayGameSettings.PlayGame)
#define Dev_DrawOtherWorld(Render) if(__Internal_Dev_Context__->DevUI.DrawOtherWorld) Render(Game, Graphics, tInterpolated, !Game->CurrentWorldIndex, Dev_GetDeveloperContext())
#define Dev_DebugLog(format, ...) DevContext_DebugLog(format, __VA_ARGS__)
#define Dev_DrawPoint(position, size, color) DevContext_AddPoint(__Internal_Dev_Context__, position, size, color)
#define Dev_DrawSegment(position0, position1, size, color) DevContext_AddSegment(__Internal_Dev_Context__, position0, position1, size, color)
#define Dev_GetCamera(WorldIndex) __Internal_Dev_Context__->DevUI.PlayGameSettings.UseDevCamera ? &__Internal_Dev_Context__->Cameras[WorldIndex] : &Game->World.GraphicsStates[WorldIndex].Camera
#define Dev_DrawGrid(GraphicsState, ViewSettings) if(__Internal_Dev_Context__->DevUI.PlayGameSettings.DrawGrid) __Internal_Dev_Context__->RenderGrid(__Internal_Dev_Context__, GraphicsState, ViewSettings)

#define PLATFORM_INIT_IMGUI(name) void name(void* PlatformWindow, struct ImGuiIO* IO)
#define PLATFORM_DEVELOPMENT_UPDATE(name) void name(ImGuiIO* IO, struct dev_input* DevInput, ak_v2i RenderDim, ak_f32 dt)

typedef PLATFORM_INIT_IMGUI(platform_init_imgui);
typedef PLATFORM_DEVELOPMENT_UPDATE(platform_development_update);

#include "include/dev_ui.h"
#include "include/dev_context.h"
#else

#define Dev_SetDeveloperContext(context) 
#define Dev_GetDeveloperContext() NULL
#define Dev_ShouldPlayGame() true
#define Dev_ShouldPlayGameAndNotUsingDevCamera() true
#define Dev_DrawOtherWorld()
#define Dev_GetCamera(WorldIndex) &Game->World.GraphicsStates[WorldIndex].Camera

#endif

#endif

#ifdef DEV_TOOLS_IMPLEMENTATION

#if DEVELOPER_BUILD
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

using namespace ImGui;
#include "src/dev_draw.cpp"
#include "src/dev_ray.cpp"
#include "src/dev_context.cpp"
#include "src/dev_ui.cpp"
#endif

#endif