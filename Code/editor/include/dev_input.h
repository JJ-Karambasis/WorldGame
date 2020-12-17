#ifndef DEV_INPUT_H
#define DEV_INPUT_H

struct dev_input
{
    union
    {
        button Buttons[13];
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
            button Z;
            button Y;
            button Delete;
            button Ctrl;
            button Alt;
        };
    };
    
    ak_v2i LastMouseCoordinates;
    ak_v2i MouseCoordinates;
    ak_f32 Scroll;
};

inline void UpdateButtons(button* Buttons, ak_u32 ButtonCount)
{
    for(ak_u32 ButtonIndex = 0; ButtonIndex < ButtonCount; ButtonIndex++)        
        Buttons[ButtonIndex].WasDown = Buttons[ButtonIndex].IsDown;             
}

#endif
