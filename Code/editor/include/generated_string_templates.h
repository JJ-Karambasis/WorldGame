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

set DLL_NAME=%%WorldName%%.dll
set PDB_NAME=%%WorldName%%_%%RANDOM%%.pdb

set SharedCommon=-DDEVELOPER_BUILD=1 -Od -FC -Zi
set Warnings=-W4 -wd4100 -wd4201 -wd4805 -wd4189 -wd4291 -wd4996 -wd4706 -wd4533 -wd4334 -wd4127
set Common=%%SharedCommon%% %%Warnings%% -nologo -EHsc- %%AKCommonPath%% %%AssetPath%% %%GamePath%% %%GraphicsPath%%

IF NOT EXIST lib (
    mkdir lib
)

pushd lib
del *.pdb > NUL 2> NUL
cl %%Common%% %%Warnings%% -LD ..\%%WorldName%%.cpp -link -opt:ref -incremental:no -pdb:%%PDB_NAME%% -out:%%DLL_NAME%%

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

global const char* Global_WorldSourceFileTemplate =
R"(#include "%.*s.h"

extern "C"
AK_EXPORT WORLD_STARTUP(%.*s_Startup)
{
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
