/* Original Author: Armand (JJ) Karambasis */
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

#include "graphics_2.cpp"

void DevelopmentTick(dev_context* DevContext, game* Game, graphics* Graphics)
{
    if(!DevContext->Initialized)
    {
        Platform_InitImGui(DevContext->PlatformData);        
        DevContext->Initialized = true;
    }
    
    dev_input* Input = &DevContext->Input;    
    
    if(IsPressed(Input->ToggleDevState))
    {
        DevContext->InDevelopmentMode = !DevContext->InDevelopmentMode;
        if(IsInDevelopmentMode(DevContext))
        {
            DevContext->Camera.Position = Game->Camera.Position;
            DevContext->Camera.AngularVelocity = {};
            DevContext->Camera.Orientation = IdentityM3();
            DevContext->Camera.FocalPoint = Game->Camera.FocalPoint;
            DevContext->Camera.Distance = Magnitude(DevContext->Camera.FocalPoint-DevContext->Camera.Position);
        }
    }
    
    if(IsInDevelopmentMode(DevContext))
    {        
        Platform_DevUpdate(DevContext->PlatformData, Graphics->RenderDim, Game->dt);
        
        camera* Camera = &DevContext->Camera;
        
        if(IsDown(Input->Alt))
        {                                    
            if(IsDown(Input->LMB))
            {
                Camera->AngularVelocity.x += (Input->MouseDelta.y*Game->dt*CAMERA_ANGULAR_ACCELERATION);
                Camera->AngularVelocity.y += (Input->MouseDelta.x*Game->dt*CAMERA_ANGULAR_ACCELERATION);                                        
            }
            
            if(IsDown(Input->MMB))
            {
                Camera->Velocity.x += (Input->MouseDelta.x*Game->dt*CAMERA_LINEAR_ACCELERATION);
                Camera->Velocity.y += (Input->MouseDelta.y*Game->dt*CAMERA_LINEAR_ACCELERATION);                                        
            }
            
            if(Abs(Input->Scroll) > 0.0f)            
                Camera->Velocity.z -= Input->Scroll*Game->dt*CAMERA_SCROLL_ACCELERATION;                                            
        }                
        
        Camera->AngularVelocity *= (1.0f / (1.0f+Game->dt*CAMERA_ANGULAR_DAMPING));            
        v3f Eulers = (Camera->AngularVelocity*Game->dt);            
        
        quaternion Orientation = Normalize(RotQuat(Camera->Orientation.XAxis, Eulers.pitch)*RotQuat(Camera->Orientation.YAxis, Eulers.yaw));
        Camera->Orientation *= ToMatrix3(Orientation);
        
        Camera->Velocity.xy *= (1.0f /  (1.0f+Game->dt*CAMERA_LINEAR_DAMPING));            
        v2f Vel = Camera->Velocity.xy*Game->dt;
        v3f Delta = Vel.x*Camera->Orientation.XAxis - Vel.y*Camera->Orientation.YAxis;
        
        Camera->FocalPoint += Delta;
        Camera->Position += Delta;
        
        Camera->Velocity.z *= (1.0f/ (1.0f+Game->dt*CAMERA_SCROLL_DAMPING));            
        Camera->Distance += Camera->Velocity.z*Game->dt;            
        
        if(Camera->Distance < CAMERA_MIN_DISTANCE)
            Camera->Distance = CAMERA_MIN_DISTANCE;
        
        Camera->Position = Camera->FocalPoint + (Camera->Orientation.ZAxis*Camera->Distance);
        
        if(Graphics->Initialized)
        {                        
#if 0
            ImGui::NewFrame();
            
            ImGui::Begin("Dev Editor");
            
            local bool Open; 
            ImGui::ShowDemoWindow(&Open);
            
            ImGui::End();
            ImGui::Render();
#endif
            
            m4 Perspective = PerspectiveM4(CAMERA_FIELD_OF_VIEW, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), CAMERA_ZNEAR, CAMERA_ZFAR);
            m4 CameraView = InverseTransformM4(Camera->Position, Camera->Orientation);        
            
            PushClear(Graphics, Black());        
            
            PushProjection(Graphics, Perspective); 
            PushCameraView(Graphics, CameraView);
            
            world* World = GetCurrentWorld(Game);
            for(world_entity* Entity = GetFirstEntity(&World->EntityPool); Entity; Entity = GetNextEntity(&World->EntityPool, Entity))
            {
                if(!Game->Assets->BoxGraphicsMesh)        
                    Game->Assets->BoxGraphicsMesh = LoadGraphicsMesh(Game->Assets, "Box.obj");
                
                ASSERT(Game->Assets->BoxGraphicsMesh);
                PushDrawShadedColoredMesh(Graphics, Game->Assets->BoxGraphicsMesh, Entity->Transform, Entity->Color); 
            }
            
        }                
    }
    
    Input->MouseDelta = {};
    Input->Scroll = 0.0f;
    for(u32 ButtonIndex = 0; ButtonIndex < ARRAYCOUNT(Input->Buttons); ButtonIndex++)
        Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;    
}
