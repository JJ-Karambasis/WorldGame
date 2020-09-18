#ifndef INPUT_H
#define INPUT_H
struct button
{
    ak_bool WasDown;
    ak_bool IsDown;
};

struct input
{
    union
    {
        button Buttons[6];
        struct
        {
            button MoveForward;
            button MoveBackward;
            button MoveLeft;
            button MoveRight;    
            button SwitchWorld;
            button Action;
        };
    };                
};

inline ak_bool IsDown(button Button)
{
    ak_bool Result = Button.IsDown;
    return Result;
}

inline ak_bool IsPressed(button Button)
{
    ak_bool Result = IsDown(Button) && !Button.WasDown;
    return Result;
}

inline ak_bool IsReleased(button Button)
{
    ak_bool Result = !IsDown(Button) && Button.WasDown;
    return Result;
}

#endif