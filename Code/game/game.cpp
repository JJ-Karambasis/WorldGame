extern "C"
AK_EXPORT GAME_STARTUP(Game_Startup)
{
    world_game* Game = (world_game*)AK_Allocate(sizeof(world_game));
    return Game;
}

extern "C"
AK_EXPORT GAME_TICK(Game_Tick)
{
    world_game* WorldGame = (world_game*)Game;
}