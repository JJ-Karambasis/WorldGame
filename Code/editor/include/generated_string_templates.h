#ifndef GENERATED_STRING_TEMPLATES_H
#define GENERATED_STRING_TEMPLATES_H

//TODO(JJ): Add vcvarsall bat file calling via editors build.bat
global const char* Global_WorldBuildFileTemplate = R"(
@echo off

set GameCodePath=%.*s
set WorldName=%.*s

set AKCommonPath=-I%%GameCodePath%%\AKCommon
set GamePath=-I%%GameCodePath%%\game
set AssetPath=-I%%GameCodePath%%\assets
set GraphicsPath=-I%%GameCodePath%%\graphics
set EditorPath=-I%%GameCodePath%%\editor

set DLL_NAME=%%WorldName%%.dll
set PDB_NAME=%%WorldName%%_%%RANDOM%%.pdb

set SharedCommon=-DDEVELOPER_BUILD=1 -Od -FC -Zi
set Warnings=-W4 -wd4100 -wd4201 -wd4805 -wd4189 -wd4291 -wd4996 -wd4706 -wd4533 -wd4334 -wd4127
set Common=%%SharedCommon%% %%Warnings%% -nologo -EHsc- %%AKCommonPath%% %%AssetPath%% %%GamePath%% %%GraphicsPath%% %%EditorPath%% -DGAME_COMPILED

set LibPath=%%~dp0\lib
set SourcePath=%%~dp0\%%WorldName%%.cpp

IF NOT EXIST %%LibPath%% (
    mkdir %%LibPath%%
)

pushd %%LibPath%%
del *.pdb > NUL 2> NUL
cl %%Common%% %%Warnings%% -LD %%SourcePath%% -link -opt:ref -incremental:no -pdb:%%PDB_NAME%% -out:%%DLL_NAME%%

)";

global const char* Global_WorldHeaderFileTemplate = 
R"(#ifndef %.*s_H
#define %.*s_H

#include <game.h>
#include "generated.h"

 struct %.*s : public world
{
    entities_a WorldEntitiesA;
    entities_b WorldEntitiesB;
    point_lights_a PointLightsA;
    point_lights_b PointLightsB;
};

extern "C" AK_EXPORT WORLD_STARTUP(%.*s_Startup);
extern "C" AK_EXPORT WORLD_UPDATE(%.*s_Update);
extern "C" AK_EXPORT WORLD_SHUTDOWN(%.*s_Shutdown);

#endif

)";

//TODO(JJ): We do not want to use AK_Allocate, we want to use something that we can control a bit better just in case an exception occurs during startup and we want to free the memory
global const char* Global_WorldSourceFileTemplate =
R"(#include "%.*s.h"

extern "C"
AK_EXPORT WORLD_STARTUP(%.*s_Startup)
{
//TODO(JJ): We do not want to use AK_Allocate, we want to use something that we can control a
//bit better just in case an exception occurs during startup and we want to free the memory
    %.*s* World = (%.*s*)AK_Allocate(sizeof(%.*s));
    World->Update = %.*s_Update;
    World->Shutdown = %.*s_Shutdown;
    Game->World = World;
    return true;
}

extern "C"
AK_EXPORT WORLD_UPDATE(%.*s_Update)
{
    %.*s* World = (%.*s*)Game->World;
}

extern "C" 
AK_EXPORT WORLD_SHUTDOWN(%.*s_Shutdown)
{
    Game->WorldShutdownCommon(Game);
    Game->World = NULL;
}

#include <assets.cpp>
#include <game_common_source.cpp>
#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>

)";

ak_string GetWorldBuildFile(ak_arena* Scratch, ak_string GameCodePath, ak_string WorldName);
ak_string GetWorldHeaderFile(ak_arena* Scratch, ak_string WorldName);
ak_string GetWorldSourceFile(ak_arena* Scratch, ak_string WorldName);

#endif
