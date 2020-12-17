@echo off

set WorldPath=%1
set WorldName=%2

set AKCommonPath=-I..\code\AKCommon
set EnginePath=-I..\code\engine
set ImGuiPath=-I..\code\imgui
set EditorPath=-I..\code\editor
set GamePath=-I..\code\game
set GraphicsPath=-I..\code\graphics
set AssetPath=-I..\code\assets

set SharedCommon=-DDEVELOPER_BUILD=1 -Od -FC -Zi
set Warnings=-W4 -wd4100 -wd4201 -wd4805 -wd4189 -wd4291 -wd4996 -wd4706 -wd4533 -wd4334 -wd4127
set Common=%SharedCommon% %Warnings% -nologo -EHsc- %AKCommonPath% %AssetPath% %EnginePath% %GamePath% %GraphicsPath%
set Compiler=cl

IF NOT EXIST ..\bin mkdir ..\bin
IF NOT EXIST ..\data mkdir ..\data

set DLL_NAME=%WorldName%.dll
set DLL_PDB_NAME=%WorldName%_dll_pdb_%RANDOM%_.pdb

set Common=%Common% -D_GAME_NAME_=%GAME_NAME%

pushd ..\Bin
%Compiler% %Common% %Warnings% -LD -DOS_WINDOWS %WorldPath%\%WorldName%\%WorldName%.cpp -link -opt:ref -incremental:no -pdb:%DLL_PDB_NAME% -out:%DLL_NAME%