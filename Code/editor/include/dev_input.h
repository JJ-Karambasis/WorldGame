#ifndef DEV_INPUT_H
#define DEV_INPUT_H

struct dev_input
{
    union
    {
        button Buttons[17];
        struct
        {
            button ToggleDevState;            
            button LMB;
            button MMB;
            button W;
            button E;
            button R;  
            button S;
            button L;
            button F;
            button N;
            button D;
            button Z;
            button Y;
            button Delete;
            button Ctrl;
            button Alt;
            button Q;
        };
    };
    
    ak_v2i MouseCoordinates;
    ak_v2i MouseDelta;
    ak_f32 Scroll;
};

inline void UpdateButtons(button* Buttons, ak_u32 ButtonCount)
{
    for(ak_u32 ButtonIndex = 0; ButtonIndex < ButtonCount; ButtonIndex++)        
        Buttons[ButtonIndex].WasDown = Buttons[ButtonIndex].IsDown;             
}

#endif
