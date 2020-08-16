@echo off

IF NOT EXIST ..\data mkdir ..\data
IF NOT EXIST ..\data\frame_recordings mkdir ..\data\frame_recordings

START /D ..\data World_Game.exe