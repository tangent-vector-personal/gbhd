// Copyright 2011 Theresa Foley. All rights reserved.
//
// gb.h

#include "gb.h"

#include "options.h"
#include "memory.h"
#include "cpu.h"
#include "gpu.h"
#include "timer.h"
#include "pad.h"
#include "opengl.h"

GameBoyState::GameBoyState()
    : _mode(kMode_Empty)
    , _lastAbsTimeNumer(0)
    , _lastAbsTimeDenom(0)
    , _pendingCyclesNumer(0)
    , _pendingCycles(0)

{
    _options = new Options();
    _memory = new MemoryState();
    _cpu = new Z80State( _memory );
    _gpu = new GPUState( *_options, _memory );
    _timer = new TimerState( _memory );
    _pad = new Pad();
    
    _multiRenderer = new MultiRenderer();
    _renderer = _multiRenderer;
    
    _multiRenderer->AddRenderer( new DefaultRenderer() );
    //gMultiRenderer->AddRenderer( new SimpleRenderer() );
    _multiRenderer->AddRenderer( new AccurateRenderer() );

    
    _memory->SetCpu(_cpu);
    _memory->SetGpu(_gpu);
    _memory->SetPad(_pad);
    _memory->SetTimer(_timer);
    
    _gpu->SetRenderer( _renderer );
}

GameBoyState::~GameBoyState()
{
}

static std::string FindPrettyGameName(
    const std::string& mediaPath,
    const std::string& rawName )
{
    static const int kBufferSize = 1024;
    char nameBuffer[kBufferSize];
    sprintf_s(nameBuffer, "%s/names.txt", mediaPath.c_str());
    
    FILE* file = nullptr;
    if (fopen_s(&file, nameBuffer, "r") != 0)
        return rawName;
    if( file == NULL )
        return rawName;
        
    char lineBuffer[kBufferSize];
    while( fgets(lineBuffer, kBufferSize, file) != NULL )
    {
        if( lineBuffer[0] == 0 || lineBuffer[0] == '#' )
            continue;
    
        const char* rawNamePart = lineBuffer;
        
        char* equals = strchr(lineBuffer, '=');
        if( equals == NULL )
            continue;
            
        *equals = 0;
        
        char* end = equals - 1;
        while( end >= rawNamePart && isspace(*end) )
            *end-- = 0;
            
        if( strcmp(rawNamePart, rawName.c_str()) != 0 )
            continue;
            
        char* prettyNamePart = equals + 1;
        
        while( isspace(*prettyNamePart) )
            prettyNamePart++;
        
        end = prettyNamePart + strlen(prettyNamePart);
        while( end >= prettyNamePart && !isprint(*end))
            *end-- = 0;
        
        return prettyNamePart;
    }

    return rawName;
};


void GameBoyState::SetGamePath(const char* path)
{
    // If another game is already open, we need to stop it.
    Stop();
    
    // Try to load a ROM from the given path
    _options->inputFileName = path;
    FILE* file = nullptr;
    if (fopen_s(&file, _options->inputFileName.c_str(), "rb") != 0
        || file == nullptr)
    {
        fprintf(stderr, "Failed to open \"%s\"\n", _options->inputFileName.c_str());
        return;
    }
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    UInt8* buffer = new UInt8[ length ];
    fread(buffer, 1, length, file);
    fclose(file);
    
    // Read game name from ROM:
    static const int kMaxRawGameNameLength = 15;
    char rawGameName[kMaxRawGameNameLength + 1] = { 0 };
    for( int ii = 0; ii < kMaxRawGameNameLength; ++ii )
    {
        static const int kRawGameNameOffset = 0x0134;
        char c = (char) buffer[ kRawGameNameOffset + ii ];
        rawGameName[ii] = c;
        if( c == 0 )
            break;
    }
    _options->rawGameName = rawGameName;
    
    _memory->SetRom( buffer );
    
    _mode = kMode_Off;
        
    // Now reset the state of the machine...
    ReloadMedia();
    Reset();
}

void GameBoyState::SetMediaPath(const char* path)
{
    _options->mediaPath = path;
    
    ReloadMedia();
}

void GameBoyState::ReloadMedia()
{
    _options->prettyGameName = FindPrettyGameName(
        _options->mediaPath,
        _options->rawGameName);

    _gpu->ClearReplacementTiles();
    _gpu->LoadReplacementTiles();
}


void GameBoyState::Start()
{
    if( _mode != kMode_Off )
        return;
    
    _mode = kMode_Running;
}

void GameBoyState::Stop()
{
    switch( _mode )
    {
    case kMode_Running:
    case kMode_Paused:
        break;
    default:
        return;
    }

    _memory->Reset();
    _cpu->Reset();
    _gpu->Reset();
    _timer->Reset();
    _pad->Reset();
    
    _lastAbsTimeNumer = 0;
    _lastAbsTimeDenom = 0;
    _pendingCyclesNumer = 0;
    _pendingCycles = 0;
    
   _mode = kMode_Off;   
}

void GameBoyState::Reset()
{
    Stop();
    Start();
}

void GameBoyState::Update( UInt64 absTimeNumer, UInt64 absTimeDenom )
{
    if( _mode != kMode_Running )
        return;
    
    if( _lastAbsTimeDenom == 0 )
    {
        _lastAbsTimeNumer = absTimeNumer;
        _lastAbsTimeDenom = absTimeDenom;
        return;
    }
    
    if( _lastAbsTimeDenom != absTimeDenom )
    {
        fprintf(stderr, "Internal timing error!\n");
        exit(1);
        return;
    }
    
    UInt64 deltaTimeNumer = absTimeNumer - _lastAbsTimeNumer;
    _lastAbsTimeNumer = absTimeNumer;
    
    // cycles / second ~= 4 * 1024 * 1024 (or so)    

    static const uint64_t kCyclesPerSecond = 4 * 1024 * 1024;

    _pendingCyclesNumer += deltaTimeNumer * kCyclesPerSecond;
    
    _pendingCycles += _pendingCyclesNumer / absTimeDenom;
    _pendingCyclesNumer = _pendingCyclesNumer % absTimeDenom;
  
    // sanity check:
    static int kMaxCyclesPerUpdate = (4 * 1024 * 1024) / 30;
    if( _pendingCycles > kMaxCyclesPerUpdate )
        _pendingCycles = kMaxCyclesPerUpdate;
        
    if( _pendingCycles < -10 )
    {
        int f = 9;
    }
      
    // We now have some number of cycles waiting to be processed
//    fprintf(stderr, "pending cycles: %d\n", pendingCycles);
    while( _pendingCycles > 0 )
    {
        int cyclesElapsed = _cpu->Step();
        _gpu->CheckLine( cyclesElapsed );
        _timer->Inc( cyclesElapsed );
        
        _pendingCycles -= cyclesElapsed;
    }
    
    if( _gpu->flip )
    {
//        glutPostRedisplay();
    }

    
}

void GameBoyState::Pause()
{
    if( _mode != kMode_Running )
        return;
        
    _mode = kMode_Paused;
}

void GameBoyState::Resume()
{
    if( _mode != kMode_Paused )
        return;
        
    _mode = kMode_Running;
}

void GameBoyState::TogglePause()
{
    switch( _mode )
    {
    case kMode_Running:
        Pause();
        break;
    case kMode_Paused:
        Resume();
        break;
    default:
        break;
    }
}


void GameBoyState::KeyDown(Key key)
{
    _pad->KeyDown(key);
}

void GameBoyState::KeyUp(Key key)
{
    _pad->KeyUp(key);
}

void GameBoyState::Render(GBRenderData& outData)
//int width, int height )
{
    switch( _mode )
    {
    case kMode_Running:
    case kMode_Paused:
        break;
    default:
        return;
    }

    _gpu->flip = false;

#if 0
    // TODO: this stuff needs a place to live...
    static const float kNativeAspectRatio =
        (float)kNativeScreenWidth / (float)kNativeScreenHeight;
    float currentAspectRatio =
        (float)width / (float)height;

    float extraW = 0.0f;
    float extraH = 0.0f;

    if (currentAspectRatio > kNativeAspectRatio)
    {
        float w = kNativeScreenHeight * currentAspectRatio;
        extraW = (w - kNativeScreenWidth) * 0.5f;
    }
    else
    {
        float h = kNativeScreenWidth / currentAspectRatio;
        extraH = (h - kNativeScreenHeight) * 0.5f;
    }
#endif

    // If the LCD is disabled, then we want to push a
    // blank frame into the state of the renderer and
    // swap it to be the visible frame.
    //
    if (!_gpu->TestLcdFlag(GPUState::kLcdFlag_LcdOn))
    {
        _renderer->RenderBlankFrame();
        _renderer->Swap();
    }

    _renderer->Present(outData);

#if 0
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClearStencil( 0 );
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    
    glOrtho(
        -extraW, kNativeScreenWidth + extraW,
        kNativeScreenHeight + extraH, -extraH,
        0, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    if( !_gpu->TestLcdFlag(GPUState::kLcdFlag_LcdOn) )
    {
        _renderer->RenderBlankFrame();
        _renderer->Swap();
    }

    _renderer->PresentGL();

    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0, 1);
    if( currentAspectRatio > kNativeAspectRatio )
    {
        glRectf(-extraW, -extraH, 0, kNativeScreenHeight+extraH);
        glRectf(kNativeScreenWidth, -extraH, kNativeScreenWidth+extraW, kNativeScreenHeight+extraH);
    }
    else
    {
        glRectf(-extraW, -extraH, kNativeScreenWidth + extraW, 0);        
        glRectf(-extraW, kNativeScreenHeight, kNativeScreenWidth + extraW, kNativeScreenHeight+extraH);
    }

#if FOO
    if( gDumpTilesOnce )
        gDumpTilesOnce = false;
        
    if( gTakeScreenShot )
    {
    
        glReadBuffer(GL_BACK);
        int width = kNativeScreenWidth * kScaleFactor;
        int height = kNativeScreenHeight * kScaleFactor;
        std::vector<Color> frame( width * height );
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &frame[0] );
        
        DumpPngScreenShot( width, height, &frame[0] );
    
        gTakeScreenShot = false;
    }
#endif
#endif
}

void GameBoyState::ToggleRenderer()
{
    if( _multiRenderer == NULL )
        return;
        
    _multiRenderer->NextRenderer();
}

void GameBoyState::DumpTiles()
{
    _options->dumpTilesOnce = true;
}

// C interface

struct GameBoyState* GameBoyState_Create()
{
    return new GameBoyState();
}

void GameBoyState_Release( struct GameBoyState* gb )
{
    delete gb;
}

void GameBoyState_SetGamePath( struct GameBoyState* gb, const char* path )
{
    if( gb == NULL ) return;
    gb->SetGamePath( path );
}

void GameBoyState_SetMediaPath( struct GameBoyState* gb, const char* path )
{
    if( gb == NULL ) return;
    gb->SetMediaPath( path );
}

void GameBoyState_Start( struct GameBoyState* gb )
{
    if( gb == NULL ) return;
    gb->Start();
}

void GameBoyState_Stop( struct GameBoyState* gb )
{
    if( gb == NULL ) return;
    gb->Stop();
}

void GameBoyState_Reset( struct GameBoyState* gb )
{
    if( gb == NULL ) return;
    gb->Reset();
}

void GameBoyState_Update( struct GameBoyState* gb, UInt64 absTimeNumer, UInt64 absTimeDenom )
{
    if( gb == NULL ) return;
    gb->Update( absTimeNumer, absTimeDenom );
}

void GameBoyState_Pause( struct GameBoyState* gb )
{
    if( gb == NULL ) return;
    gb->Pause();
}

void GameBoyState_Resume( struct GameBoyState* gb )
{
    if( gb == NULL ) return;
    gb->Resume();
}

void GameBoyState_TogglePause( struct GameBoyState* gb )
{
    if( gb == NULL ) return;
    gb->TogglePause();
}

void GameBoyState_KeyDown( struct GameBoyState* gb, enum Key key )
{
    if( gb == NULL ) return;
    gb->KeyDown(key);
}

void GameBoyState_KeyUp( struct GameBoyState* gb, enum Key key )
{
    if( gb == NULL ) return;
    gb->KeyUp(key);
}

GBRenderData GameBoyState_Render( struct GameBoyState* gb/*, int width, int height*/)
{
    GBRenderData data = { 0 };

    if( gb == NULL ) return data;
    gb->Render(data/*width, height*/);
    return data;
}

void GameBoyState_ToggleRenderer( struct GameBoyState* gb )
{
    if( gb == NULL ) return;
    gb->ToggleRenderer();
}

void GameBoyState_DumpTiles( struct GameBoyState* gb )
{
    if( gb == NULL ) return;
    gb->DumpTiles();
}

