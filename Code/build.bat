@echo off

set Warnings=-W4 -wd4100 -wd4201 -wd4805 -wd4189 -wd4291 -wd4996 -wd4706 -wd4533
set Debug=-Od -DDEBUG_BUILD=1
set Optimal=-Ox

set Common=-DDEVELOPER_BUILD=1 -nologo -FC -Z7 -Oi -EHsc- %Debug% %Warnings% 

IF NOT EXIST ..\Bin mkdir ..\Bin
IF NOT EXIST ..\Data mkdir ..\Data

set COMPILE_PLATFORM=1
set COMPILE_GRAPHICS=1
set COMPILE_GAME=1

pushd ..\Bin
del *.pdb > NUL 2> NUL

IF %COMPILE_GAME% == 1 (
    cl %Common% -LD -DOS_WINDOWS ..\code\world_game.cpp -link -opt:ref -incremental:no -pdb:ProjectWorld_%RANDOM%.pdb -out:World_Game.dll
)

IF %COMPILE_GRAPHICS% == 1 (
    REM cl %Common% -LD -DOS_WINDOWS ..\code\vulkan_graphics.cpp -link -opt:ref -incremental:no -pdb:DXGraphics_%RANDOM%.pdb -out:DXGraphics.dll
)

IF %COMPILE_PLATFORM% == 1 (
    cl %Common% -DOS_WINDOWS ..\code\win32_world_game.cpp -link user32.lib -opt:ref -out:World_Game.exe
)

popd