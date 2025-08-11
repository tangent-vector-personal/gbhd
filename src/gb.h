// Copyright 2011 Theresa Foley. All rights reserved.
//
// gb.h

#ifndef GBHD_GB_H
#define GBHD_GB_H

#include "keys.h"
#include "types.h"

#ifdef __cplusplus

class Options;
class MemoryState;
class Z80State;
class GPUState;
class TimerState;
class Pad;
class MultiRenderer;
class IRenderer;

//
struct GameBoyState
{
public:
    GameBoyState();
    ~GameBoyState();
    
    void SetGamePath(const char* path);
    void SetMediaPath(const char* path);
    
    void ReloadMedia();

    // Start/stop emulation
    void Start();
    void Stop();
    void Reset();
    
    void Update( UInt64 absTimeNumer, UInt64 absTimeDenom );
    
    void Pause();
    void Resume();
    void TogglePause();
    
    void KeyDown(Key key);
    void KeyUp(Key key);
    
    void Render( int width, int height );
    void ToggleRenderer();
    void DumpTiles();
    
private:
    enum Mode
    {
        kMode_Empty = 0,
        kMode_Off,
        kMode_Running,
        kMode_Paused,
    };
    Mode _mode;

    Options* _options;
    MemoryState* _memory;
    Z80State* _cpu;
    GPUState* _gpu;
    TimerState* _timer;
    Pad* _pad;
    
    MultiRenderer* _multiRenderer;
    IRenderer* _renderer;
    
    UInt64 _lastAbsTimeNumer;
    UInt64 _lastAbsTimeDenom;
    UInt64 _pendingCyclesNumer;
    SInt64 _pendingCycles;
};

extern "C"
{
#endif

// C interface

struct GameBoyState* GameBoyState_Create();
void GameBoyState_Release( struct GameBoyState* gb );

void GameBoyState_SetGamePath( struct GameBoyState* gb, const char* path );
void GameBoyState_SetMediaPath( struct GameBoyState* gb, const char* path );

void GameBoyState_Start( struct GameBoyState* gb );
void GameBoyState_Stop( struct GameBoyState* gb );
void GameBoyState_Reset( struct GameBoyState* gb );

void GameBoyState_Update( struct GameBoyState* gb, UInt64 absTimeNumer, UInt64 absTimeDenom );

void GameBoyState_Pause( struct GameBoyState* gb );
void GameBoyState_Resume( struct GameBoyState* gb );
void GameBoyState_TogglePause( struct GameBoyState* gb );

void GameBoyState_KeyDown( struct GameBoyState* gb, enum Key key );
void GameBoyState_KeyUp( struct GameBoyState* gb, enum Key key );

void GameBoyState_Render( struct GameBoyState* gb, int width, int height );
void GameBoyState_ToggleRenderer( struct GameBoyState* gb );
void GameBoyState_DumpTiles( struct GameBoyState* gb );

#ifdef __cplusplus
}
#endif

#endif // GBHD_GB_H
