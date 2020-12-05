@echo off

set FBXPath=-I..\code\fbxsdk
set AKCommonPath=-I..\code\AKCommon
set AssetsPath=-I..\code\assets

set SharedCommon=-DDEVELOPER_BUILD=1 -Od -FC -Zi
set Warnings=-W4 -wd4100 -wd4201 -wd4805 -wd4189 -wd4291 -wd4996 -wd4706 -wd4533 -wd4334 -wd4127
set Common=%SharedCommon% %Warnings% -nologo -EHsc- -F 1073741824 %AKCommonPath% %AssetsPath%
set Compiler=cl

IF NOT EXIST ..\bin mkdir ..\bin
IF NOT EXIST ..\data mkdir ..\data
IF NOT EXIST ..\data\frame_recordings mkdir ..\data\frame_recordings

set COMPILE_PLATFORM=1
set COMPILE_GRAPHICS=0
set COMPILE_GAME=1
set COMPILE_ASSET_BUILDER=0
set COMPILE_GRAPHICS_POC_RAYTRACE=0
set COMPILE_GRAPHICS_ILLUMINATION_POC=0

pushd ..\Bin
del *.pdb > NUL 2> NUL

if %COMPILE_ASSET_BUILDER% == 1 (
    %Compiler% %Common% %Warnings% %FBXPath% -DOS_WINDOWS ..\code\assets\asset_builder\asset_builder.cpp -link libfbxsdk-mt.lib -opt:ref -incremental:no -out:Asset_Builder.exe
)

if %COMPILE_GAME% == 1 (
    %Compiler% %Common% %Warnings% -LD -DOS_WINDOWS ..\code\world_game.cpp -link libfbxsdk-mt.lib -opt:ref -incremental:no -pdb:World_Game_%RANDOM%.pdb -out:World_Game.dll
)

if %COMPILE_GRAPHICS% == 1 (
    %Compiler% %Common% %Warnings% -LD -DOS_WINDOWS ..\code\opengl.cpp -link user32.lib gdi32.lib opengl32.lib -opt:ref -incremental:no -pdb:OpenGL_%RANDOM%.pdb -out:OpenGL.dll
)

if %COMPILE_PLATFORM% == 1 (
    %Compiler% %Common% %Warnings% -DSHOW_IMGUI_DEMO_WINDOW=0 -DOS_WINDOWS ..\code\win32_world_game.cpp -link libfbxsdk-mt.lib user32.lib ole32.lib -opt:ref -out:World_Game.exe
) 

if %COMPILE_GRAPHICS_POC_RAYTRACE% == 1 (
    %Compiler% %Common% %Warnings% -O2 -DOS_WINDOWS ..\code\graphics_poc\win32_graphics_ray_trace.cpp -link user32.lib Gdi32.lib -opt:ref -out:RayTrace_POC.exe
)

set SHADER_COMMON=-Od -Zi -nologo
set VERTEX_SHADER_COMMON=-T vs_5_1 %SHADER_COMMON%
set PIXEL_SHADER_COMMON=-T ps_5_1 %SHADER_COMMON%
set COMPUTE_SHADER_COMMON=-T cs_5_1 %SHADER_COMMON%
    
if %COMPILE_GRAPHICS_ILLUMINATION_POC% == 1 (    
    %Compiler% %Common% %Warnings% -O2 -Oi -DOS_WINDOWS ..\code\graphics_poc\win32_dx12_illumination_poc.cpp -link user32.lib Gdi32.lib -opt:ref -out:Illumination_POC.exe
    fxc %VERTEX_SHADER_COMMON% -E ObjectVertexShader -Fo ObjectVertexShader.cso ..\code\graphics_poc\shaders\object_vertex_shader.hlsl
    fxc %VERTEX_SHADER_COMMON% -E ShadowMapVertexShader -Fo ShadowMapVertexShader.cso ..\code\graphics_poc\shaders\shadow_map_vertex_shader.hlsl
    fxc %VERTEX_SHADER_COMMON% -E DebugPrimitiveVertexShader -Fo DebugPrimitiveVertexShader.cso ..\code\graphics_poc\shaders\debug_primitive_vertex_shader.hlsl
    fxc %PIXEL_SHADER_COMMON% -E ObjectPixelShader -Fo ObjectPixelShader.cso ..\code\graphics_poc\shaders\object_pixel_shader.hlsl
    fxc %PIXEL_SHADER_COMMON% -E ShadowMapPixelShader -Fo ShadowMapPixelShader.cso ..\code\graphics_poc\shaders\shadow_map_pixel_shader.hlsl   
    fxc %PIXEL_SHADER_COMMON% -E DebugPrimitivePixelShader -Fo DebugPrimitivePixelShader.cso ..\code\graphics_poc\shaders\debug_primitive_pixel_shader.hlsl
    fxc %COMPUTE_SHADER_COMMON% -E ShadeSurfelsComputeShader -Fo ShadeSurfelsComputeShader.cso ..\code\graphics_poc\shaders\shade_surfels_compute_shader.hlsl
    fxc %COMPUTE_SHADER_COMMON% -DIRRADIANCE_BLENDING=1 -E ProbeBlendingComputeShader -Fo IrradianceProbeBlendingComputeShader.cso ..\code\graphics_poc\shaders\probe_blending_compute_shader.hlsl
    fxc %COMPUTE_SHADER_COMMON% -DIRRADIANCE_BLENDING=0 -E ProbeBlendingComputeShader -Fo DistanceProbeBlendingComputeShader.cso ..\code\graphics_poc\shaders\probe_blending_compute_shader.hlsl
    fxc %COMPUTE_SHADER_COMMON% -E ProbeBorderUpdateRowComputeShader -Fo ProbeBorderUpdateRowComputeShader.cso ..\code\graphics_poc\shaders\probe_border_update_compute_shader.hlsl
    fxc %COMPUTE_SHADER_COMMON% -E ProbeBorderUpdateColumnComputeShader -Fo ProbeBorderUpdateColumnComputeShader.cso ..\code\graphics_poc\shaders\probe_border_update_compute_shader.hlsl
)

popd