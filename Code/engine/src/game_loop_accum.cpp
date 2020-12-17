game_loop_accum InitGameLoopAccum(ak_f32 dtFixed)
{
    game_loop_accum Result = {};
    Result.dtFixed = dtFixed;
    Result.CurrentTime = AK_WallClock();
    return Result;
}

void game_loop_accum::IncrementAccum()
{
    ak_high_res_clock NewTime = AK_WallClock();
    ak_f32 FrameTime = (ak_f32)AK_GetElapsedTime(NewTime, CurrentTime);
    if(FrameTime > 0.05f)
        FrameTime = 0.05f;
    
    CurrentTime = NewTime;
    Accumulator += FrameTime;
}

void game_loop_accum::DecrementAccum()
{
    Accumulator -= dtFixed;
}

ak_bool game_loop_accum::ShouldUpdate()
{
    return Accumulator >= dtFixed;
}

ak_f32 game_loop_accum::GetRemainder()
{
    return Accumulator / dtFixed;
}