#include "engine.h"

#include "src/assets.cpp"
#include <src/graphics_state.cpp>
#include "src/game_loop_accum.cpp"

extern "C"
AK_EXPORT ENGINE_LOAD_ASSETS(Engine_LoadAssets)
{
    return InitAssets(AssetPath);
}

engine* Engine_Initialize(graphics* Graphics, ak_string AssetPath, assets* Assets)
{
    engine* Engine = (engine*)AK_Allocate(sizeof(engine));
    if(!Engine)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    Engine->Scratch = AK_CreateArena(AK_Megabyte(4));
    Engine->Graphics = Graphics;
    Engine->Assets = Assets;
    
    if(!Engine->Assets)
    {
        Engine->Assets = Engine_LoadAssets(AssetPath);
        if(!Engine->Assets)
        {
            //TODO(JJ): Diagnostic and error logging
            AK_InvalidCode();
            return NULL;
        }
    }
    
    return Engine;
}

extern "C"
AK_EXPORT ENGINE_RUN(Engine_Run)
{
    game_code GameCode = {};
    if(!Platform->LoadGameCode(&GameCode))
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    engine* Engine = Engine_Initialize(Graphics, Platform->AssetPath);
    if(!Engine)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    if(!GameCode.Startup(Engine))
    {
        //TODO(JJ): Diagnost ic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    Engine->dtFixed = 1.0f/60.0f;
    
    game_loop_accum GameLoopAccum = InitGameLoopAccum(Engine->dtFixed);
    for(;;)
    {
        ak_temp_arena TempArena = Engine->Scratch->BeginTemp();
        
        GameLoopAccum.IncrementAccum();
        
        while(GameLoopAccum.ShouldUpdate())
        {
            input* Input = &Engine->Input;
            if(!Platform->ProcessMessages(Input))
                return 0;
            
            GameCode.Update(Engine->Game);
            for(ak_u32 ButtonIndex = 0; ButtonIndex < AK_Count(Input->Buttons); ButtonIndex++)        
            {
                Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;             
            }
            
            GameLoopAccum.DecrementAccum();
        }
        
        ak_v2i Resolution = Platform->GetResolution();
        UpdateRenderBuffer(Graphics, &Engine->RenderBuffer, Resolution);
        
        ak_f32 tInterpolated = GameLoopAccum.GetRemainder();
        graphics_state GraphicsState = GameCode.GetGraphicsState(Engine->Game, tInterpolated);
        Engine_Render(Engine, &GraphicsState);
        Platform->ExecuteRenderCommands(Graphics);
        
        Engine->Scratch->EndTemp(&TempArena);
    }
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>