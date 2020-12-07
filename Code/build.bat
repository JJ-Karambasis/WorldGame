@echo off

set AKCommonPath=-I..\code\AKCommon
set EnginePath=-I..\code\engine


set SharedCommon=-DDEVELOPER_BUILD=1 -Od -FC -Zi
set Warnings=-W4 -wd4100 -wd4201 -wd4805 -wd4189 -wd4291 -wd4996 -wd4706 -wd4533 -wd4334 -wd4127
set Common=%SharedCommon% %Warnings% -nologo -EHsc- -F 1073741824 %AKCommonPath% %EnginePath%
set Compiler=cl

IF NOT EXIST ..\bin mkdir ..\bin
IF NOT EXIST ..\data mkdir ..\data
IF NOT EXIST ..\data\frame_recordings mkdir ..\data\frame_recordings

set GAME_NAME=World_Game
set EXE_NAME=%GAME_NAME%.exe
set DLL_NAME=%GAME_NAME%.dll

set Common=%Common% -D_GAME_NAME_=%GAME_NAME%

set COMPILE_PLATFORM=1
set COMPILE_ENGINE=1
set COMPILE_GAME=1
set COMPILE_GRAPHICS=1

pushd ..\Bin
del *.pdb > NUL 2> NUL

if %COMPILE_GRAPHICS% == 1 (
    %Compiler% %Common% %Warnings% -LD -DOS_WINDOWS ..\code\engine\src\opengl.cpp -link user32.lib gdi32.lib opengl32.lib -opt:ref -incremental:no -pdb:OpenGL_%RANDOM%.pdb -out:OpenGL.dll
)

if %COMPILE_GAME% == 1 (
    %Compiler% %Common% %Warnings% -LD -DOS_WINDOWS ..\code\game\game.cpp -link -opt:ref -incremental:no -out:%DLL_NAME%
)

if %COMPILE_ENGINE% == 1 (
    %Compiler% %Common% %Warnings% -LD -DOS_WINDOWS ..\code\engine\engine.cpp -link -opt:ref -incremental:no -out:Engine.dll
)

if %COMPILE_PLATFORM% == 1 (
    %Compiler% %Common% %Warnings% -DSHOW_IMGUI_DEMO_WINDOW=0 -DOS_WINDOWS ..\code\platforms\win32\win32_platform.cpp -link user32.lib ole32.lib -opt:ref -out:%EXE_NAME%
) 
