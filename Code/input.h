#ifndef INPUT_H
#define INPUT_H
struct button
{
    b32 WasDown;
    b32 IsDown;
};

struct input
{
    union
    {
        button Buttons[5];
        struct
        {
            button MoveForward;
            button MoveBackward;
            button MoveLeft;
            button MoveRight;    
            button SwitchWorld;
        };
    };        
    v2i MouseDelta;
    f32 dt;
};

inline b32 IsDown(button Button)
{
    b32 Result = Button.IsDown;
    return Result;
}

inline b32 IsPressed(button Button)
{
    b32 Result = IsDown(Button) && !Button.WasDown;
    return Result;
}

inline b32 IsReleased(button Button)
{
    b32 Result = !IsDown(Button) && Button.WasDown;
    return Result;
}

#endif