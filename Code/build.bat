@echo off

set WORLDS_PATH={YOUR_PATH_TO_WORLDS_HERE}
set RECORDINGS_PATH={YOUR_PATH_TO_RECORDINGS_HERE}
set GAME_NAME=World_Game

set AKCommonPath=-I..\code\AKCommon
set AssetPath=-I..\code\assets
set ImGuiPath=-I..\code\imgui
set EditorPath=-I..\code\editor
set GamePath=-I..\code\game
set GraphicsPath=-I..\code\graphics

set SharedCommon=-DDEVELOPER_BUILD=1 -Od -FC -Zi
set Warnings=-W4 -wd4100 -wd4201 -wd4805 -wd4189 -wd4291 -wd4996 -wd4706 -wd4533 -wd4334 -wd4800 -wd4127
set Common=%SharedCommon% %Warnings% -nologo -EHsc- %AKCommonPath% %AssetPath% %GraphicsPath%
set Compiler=cl

IF NOT EXIST ..\bin mkdir ..\bin

set EXE_NAME=%GAME_NAME%.exe
set DLL_NAME=%GAME_NAME%.dll
set DLL_PDB_NAME=%GAME_NAME%_dll_pdb_%RANDOM%_.pdb

set Common=%Common% -D_GAME_NAME_=%GAME_NAME% -D_WORLDS_PATH_=%WORLDS_PATH%

set DemoWindow=-DSHOW_IMGUI_DEMO_WINDOW=0

set COMPILE_PLATFORM=0
set COMPILE_GAME=1
set COMPILE_GRAPHICS=1
set COMPILE_EDITOR=1

pushd ..\Bin
del *.pdb > NUL 2> NUL

if %COMPILE_GRAPHICS% == 1 (
    %Compiler% %Common% %Warnings% -LD -DOS_WINDOWS ..\code\graphics\src\opengl.cpp -link user32.lib gdi32.lib opengl32.lib -opt:ref -incremental:no -pdb:OpenGL.pdb -out:OpenGL.dll
)

if %COMPILE_GAME% == 1 (
    %Compiler% %Common% %Warnings% -LD %EditorPath% %ImGuiPath% -DOS_WINDOWS -DGAME_COMPILED ..\code\game\game.cpp -link -opt:ref -incremental:no -pdb:%DLL_PDB_NAME% -out:%DLL_NAME%
)

if %COMPILE_PLATFORM% == 1 (
    %Compiler% %Common% %Warnings% %GamePath% -DOS_WINDOWS ..\code\platforms\win32\win32_platform.cpp -link user32.lib ole32.lib -opt:ref -out:%EXE_NAME%
) 

if %COMPILE_EDITOR% == 1 (
    %Compiler% %Common% %Warnings% %ImGuiPath% %EditorPath% %GamePath% -LD %DemoWindow% -DOS_WINDOWS -D_RECORDINGS_PATH_=%RECORDINGS_PATH% ..\code\editor\editor.cpp -link -opt:ref -incremental:no -pdb:Editor_dll_pdb.pdb -out:Editor.dll
    %Compiler% %Common% %Warnings% %ImGuiPath% %EditorPath% %GamePath% -DOS_WINDOWS -DDEV_EDITOR ..\code\platforms\win32\win32_platform.cpp -link user32.lib ole32.lib -opt:ref -out:Editor.exe
)

popd