#include "wav.h"

audio WAV_LoadAudio(char* File, arena* Storage)
{    
    audio Result = {};
    
    buffer FileBuffer = Global_Platform->ReadEntireFile(File);
    BOOL_CHECK_AND_HANDLE(FileBuffer.Data, "Failed to read the wav file: %s\n", File);
    
    riff_header Header;
    riff_iterator Iter = IterateRIFF(FileBuffer, &Header); 
    
    BOOL_CHECK_AND_HANDLE((Header.RiffID == RIFFID_RIFF) && (Header.FileTypeID == WAVE_CHUNK_TYPE_WAVE),
                          "WAV File is not a valid riff file.");
    
    
    samples Samples = {};    
    
    wav_format* WAVFormat = NULL;
    while(IsValid(Iter))
    {
        switch(GetType(Iter))
        {
            case WAVE_CHUNK_TYPE_FMT:
            {
                WAVFormat = (wav_format*)GetChunkData(Iter);
                BOOL_CHECK_AND_HANDLE(WAVFormat->wFormatTag == 1, "Format tag for wav files is not PCM");                
                BOOL_CHECK_AND_HANDLE(WAVFormat->wBitsPerSample == 16, "Bits per sample must be 16");
                BOOL_CHECK_AND_HANDLE(WAVFormat->nSamplesPerSec == 48000, "Samples per second needs to be 48000");                
                BOOL_CHECK_AND_HANDLE((WAVFormat->nChannels == 1) || (WAVFormat->nChannels == 2), "WAV format needs to have 1 (mono) or 2 (stereo) channels.");                
            } break;
            
            case WAVE_CHUNK_TYPE_DATA:
            {
                BOOL_CHECK_AND_HANDLE(WAVFormat, "Wav format was not found. File is corrupted.");
                
                Samples.Data = (i16*)GetChunkData(Iter);
                Samples.Count = GetChunkDataSize(Iter)/(WAVFormat->nChannels*sizeof(i16));
            } break;
        }
        
        Iter = NextChunk(Iter);
    }
    
    BOOL_CHECK_AND_HANDLE(Samples.Data, "Failed to find the data chunk for the wav file.");
    
    Result.ChannelCount = WAVFormat->nChannels;        
    Result.Samples.Count = Samples.Count;
    Result.Samples.Data = (i16*)PushSize(Storage, Samples.Count, Clear, 0);    
    
    for(u32 SampleIndex = 0; SampleIndex < Samples.Count; SampleIndex++)
    {
        i16* DstChannelSamples = GetSamples(&Result.Samples, Result.ChannelCount, SampleIndex);
        i16* SrcChannelSamples = GetSamples(&Samples, Result.ChannelCount, SampleIndex);
        for(u32 ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ChannelIndex++)        
            *DstChannelSamples++ = *SrcChannelSamples++;                    
    }
    
    handle_error:
    Global_Platform->FreeFileMemory(&FileBuffer);
    return Result;
}
