@echo off

set Warnings=-W4 -wd4100 -wd4201 -wd4805 -wd4189 -wd4291 -wd4996 -wd4706 -wd4533
set Common=-DDEVELOPER_BUILD=1 -Od -nologo -FC -Z7 -Oi -EHsc- %Warnings% 

IF NOT EXIST ..\din mkdir ..\bin
IF NOT EXIST ..\data mkdir ..\data
IF NOT EXIST ..\data\shaders\vulkan mkdir ..\data\shaders\vulkan

set COMPILE_PLATFORM=1
set COMPILE_GRAPHICS=1
set COMPILE_GAME=1
set COMPILE_SHADERS=1

pushd ..\Bin
del *.pdb > NUL 2> NUL

IF %COMPILE_GAME% == 1 (
    cl %Common% -LD -DOS_WINDOWS ..\code\world_game.cpp -link -opt:ref -incremental:no -pdb:World_Game_%RANDOM%.pdb -out:World_Game.dll
)

IF %COMPILE_GRAPHICS% == 1 (
    cl %Common% -LD -DOS_WINDOWS ..\code\vulkan_graphics.cpp -link user32.lib -opt:ref -incremental:no -pdb:Vulkan_Graphics_%RANDOM%.pdb -out:Vulkan_Graphics.dll
)

IF %COMPILE_PLATFORM% == 1 (
    cl %Common% -DOS_WINDOWS ..\code\win32_world_game.cpp -link user32.lib -opt:ref -out:World_Game.exe
)

popd

pushd ..\data\shaders\vulkan
IF %COMPILE_SHADERS% == 1 (
    glslc -O0 -DVERTEX   -fshader-stage=vertex   ..\..\..\code\shaders\glsl\4.5\opaque_shading.glsl -o opaque_shading_vertex.spv
    glslc -O0 -DFRAGMENT -fshader-stage=fragment ..\..\..\code\shaders\glsl\4.5\opaque_shading.glsl -o opaque_shading_fragment.spv

    glslc -O0 -DVERTEX   -fshader-stage=vertex   ..\..\..\code\shaders\glsl\4.5\debug_volumes.glsl -o debug_volumes_vertex.spv
    glslc -O0 -DFRAGMENT -fshader-stage=fragment ..\..\..\code\shaders\glsl\4.5\debug_volumes.glsl -o debug_volumes_fragment.spv

    glslc -O0 -DVERTEX   -fshader-stage=vertex   ..\..\..\code\shaders\glsl\4.5\quad.glsl -o quad_vertex.spv
    glslc -O0 -DFRAGMENT -fshader-stage=fragment ..\..\..\code\shaders\glsl\4.5\quad.glsl -o quad_fragment.spv

    glslc -O0 -DVERTEX   -fshader-stage=vertex   ..\..\..\code\shaders\glsl\4.5\debug_point.glsl -o debug_point_vertex.spv
    glslc -O0 -DGEOMETRY -fshader-stage=geometry ..\..\..\code\shaders\glsl\4.5\debug_point.glsl -o debug_point_geometry.spv
    glslc -O0 -DFRAGMENT -fshader-stage=fragment ..\..\..\code\shaders\glsl\4.5\debug_point.glsl -o debug_point_fragment.spv

    glslc -O0 -DVERTEX   -fshader-stage=vertex   ..\..\..\code\shaders\glsl\4.5\debug_line.glsl -o debug_line_vertex.spv
    glslc -O0 -DGEOMETRY -fshader-stage=geometry ..\..\..\code\shaders\glsl\4.5\debug_line.glsl -o debug_line_geometry.spv
    glslc -O0 -DFRAGMENT -fshader-stage=fragment ..\..\..\code\shaders\glsl\4.5\debug_line.glsl -o debug_line_fragment.spv

    glslc -O0 -DVERTEX   -fshader-stage=vertex   ..\..\..\code\shaders\glsl\4.5\imgui.glsl -o imgui_vertex.spv
    glslc -O0 -DFRAGMENT -fshader-stage=fragment ..\..\..\code\shaders\glsl\4.5\imgui.glsl -o imgui_fragment.spv
)
popd