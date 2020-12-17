#ifndef GAME_LOOP_ACCUM_H
#define GAME_LOOP_ACCUM_H

struct game_loop_accum
{
    ak_f32 dtFixed;
    ak_high_res_clock CurrentTime;
    ak_f32 Accumulator;
    
    void IncrementAccum();
    void DecrementAccum();
    ak_bool ShouldUpdate();
    ak_f32 GetRemainder();
};

game_loop_accum InitGameLoopAccum(ak_f32 dtFixed);

#endif
