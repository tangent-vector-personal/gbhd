// Copyright 2011 Theresa Foley. All rights reserved.
//
// main.cpp

#include <iostream>
#include <cstdio>

#include "cpu.h"
#include "gpu.h"
#include "memory.h"
#include "options.h"
#include "pad.h"
#include "timer.h"
#include "types.h"

#include "opengl.h"

#include "png.h"

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_time.h>

#include "gb.h"

#if 0
FILE* gLogFile = NULL;

Z80State* gCpu = NULL;
GPUState* gGpu;
Pad* gPad;
TimerState* gTimer;
MemoryState* gMemory;
IRenderer* gRenderer;
MultiRenderer* gMultiRenderer = NULL;

static const int kScaleFactor = 4;

static int gHaveSeenPC[65536] = { 0 };

bool gDumpTilesOnce = false;
bool gPause = false;
bool gTakeScreenShot = false;

void AdvanceFrame()
{
    if( gPause ) return;

//    int frameClock = gCpu->clockM + 17556;
    int remainingCycles = 17556 * 4;

    Z80State* cpu = gCpu;

    do
    {
        int pc = gCpu->pc;
        if( !gHaveSeenPC[pc] )
        {
            gHaveSeenPC[pc]++;
            static FILE* f = fopen("dump.txt", "w");
            gLogFile = f;
        }
        else
        {
            gLogFile = NULL;
        }
        
        int cyclesElapsed = gCpu->Step();
        gGpu->CheckLine( cyclesElapsed );
        gTimer->Inc( cyclesElapsed );

//        gLogFile = NULL;

        switch( pc & 0xf000 )
        {
        // ROM bank 0
        case 0x0000:
        case 0x1000:
        case 0x2000:
        case 0x3000:
        // ROM bak 1
        case 0x4000:
        case 0x5000:
        case 0x6000:
        case 0x7000:
        // External RAM
        case 0xa000:
        case 0xb000:
        // Work RAM
        case 0xc000:
        case 0xd000:
        case 0xe000:
            break;

        // VRAM
        case 0x8000:
        case 0x9000:
            throw 1;
        
        case 0xf000:
            switch( pc & 0x0f00 )
            {
            // Work RAM (echo)
            case 0x000:
            case 0x100:
            case 0x200:
            case 0x300:
            case 0x400:
            case 0x500:
            case 0x600:
            case 0x700:
            case 0x800:
            case 0x900:
            case 0xA00:
            case 0xB00:
            case 0xC00:
            case 0xD00:
                break;
                
            // OAM
            case 0xe00:
                throw 1;
                    
            // Zeropage RAM, I/O, interrupts
            case 0xf00:
                if( pc == 0xffff )
                {
                    throw 1;
                }
                else if( pc > 0xff7f )
                {
                }
                else
                {
                    throw 1;
                }
            }
        }
        
        remainingCycles -= cyclesElapsed;
    }
    while( remainingCycles > 0 );
}

static void DumpPngScreenShot(
    int width,
    int height,
    const Color* pixels )
{
    static int counter = 0;
    
    char name[1024];
    sprintf(name, "shot_%03d.png", counter);
    
    counter++;
    
#if 0
    // Now write out an actual PNG file
    FILE* file = fopen(name, "wb");
    if( file == NULL )
        throw 1;
    
    png_structp pngContext = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        NULL,
        NULL,
        NULL );
    if( pngContext == NULL )
        throw 1;
    
    png_infop pngInfo = png_create_info_struct(pngContext);
    if( pngInfo == NULL )
        throw 1;
    
    if( setjmp(png_jmpbuf(pngContext)) )
        throw 1;
    
    png_init_io(pngContext, file);
    
    // header
    png_set_IHDR(
        pngContext,
        pngInfo,
        width,
        height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);
        
    png_write_info(pngContext, pngInfo);

    std::vector<const Color*> rowPointers(height);
    for( int ii = 0; ii < height; ++ii )
    {
        // we invert the order of rows here to correct for
        // the OpenGL bottom-left origin
        rowPointers[ii] = &pixels[(height - ii - 1)*width];
    }

    // write image data
    png_write_image(pngContext, (png_bytepp) &rowPointers[0]);
    
    png_write_end(pngContext, NULL);
    
    fclose(file);
    
#endif
}

extern "C" void DrawCore( int width, int height );
void DrawCore( int width, int height )
{
    if( gCpu == NULL )
        return;

//    AdvanceFrame();
    gGpu->flip = false;

#if 0

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClearStencil( 0 );
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    static const float kNativeAspectRatio =
        (float) kNativeScreenWidth / (float) kNativeScreenHeight;
    float currentAspectRatio =
        (float) width / (float) height;
    
    float extraW = 0.0f;
    float extraH = 0.0f;
    
    if( currentAspectRatio > kNativeAspectRatio )
    {
        float w = kNativeScreenHeight * currentAspectRatio;
        extraW = (w - kNativeScreenWidth) * 0.5f;
    }
    else
    {
        float h = kNativeScreenWidth / currentAspectRatio;
        extraH = (h - kNativeScreenHeight) * 0.5f;
    }
    
    glOrtho(
        -extraW, kNativeScreenWidth + extraW,
        kNativeScreenHeight + extraH, -extraH,
        0, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    if( !gGpu->TestLcdFlag(GPUState::kLcdFlag_LcdOn) )
    {
        gRenderer->RenderBlankFrame();
        gRenderer->Swap();

    }
    
    gRenderer->PresentGL();

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
    
//    glutSwapBuffers();
#endif
}

extern "C" void UpdateCore(int deltaTime);

void UpdateCore(int deltaTime)
{
    if( gCpu == NULL )
        return;

/*
    static int lastTime = 0;
    int timeInMS = glutGet( GLUT_ELAPSED_TIME );
    if( lastTime == 0 )
    {
        lastTime = timeInMS;
    }
    
    int deltaTime = timeInMS - lastTime;
    lastTime = timeInMS;
*/
    if( deltaTime == 0 )
        return;
    
    if( gPause )
        deltaTime = 0;
    
    // ms / s = 1000 / 1
    // cycles / second ~= 4 * 1024 * 1024 (or so)    
    // pendingCycles = pendingMS * 4M / 1000

    static const uint64_t kCyclesPerSecond = 4 * 1024 * 1024;
    static const uint64_t kMillisecondsPerSecond = 1000;

    static uint64_t pendingCyclesNumer = 0;
    static const uint64_t pendingCyclesDenom = kMillisecondsPerSecond;
    pendingCyclesNumer += deltaTime * kCyclesPerSecond;
    
    static int32_t pendingCycles = 0;
    
    pendingCycles += pendingCyclesNumer / pendingCyclesDenom;
    pendingCyclesNumer = pendingCyclesNumer % pendingCyclesDenom;
  
    // sanity check:
    static int kMaxCyclesPerUpdate = (4 * 1024 * 1024) / 30;
    if( pendingCycles > kMaxCyclesPerUpdate )
        pendingCycles = kMaxCyclesPerUpdate;
      
    // We now have some number of cycles waiting to be processed
//    fprintf(stderr, "pending cycles: %d\n", pendingCycles);
    while( pendingCycles > 0 )
    {
        int cyclesElapsed = gCpu->Step();
        gGpu->CheckLine( cyclesElapsed );
        gTimer->Inc( cyclesElapsed );
        
        pendingCycles -= cyclesElapsed;
    }
    
    if( gGpu->flip )
    {
//        glutPostRedisplay();
    }
}

extern "C" void KeyDownCore(int c);
extern "C" void KeyUpCore(int c);

void KeyDownCore(int c)
{
    if( gCpu == NULL )
        return;

    switch( c )
    {
    case 'a': gPad->KeyDown(kKey_A);        break;
    case 'b': gPad->KeyDown(kKey_B);        break;
    case 's': gPad->KeyDown(kKey_Start);    break;
    case 't': gPad->KeyDown(kKey_Select);   break;

    case 'i': gPad->KeyDown(kKey_Up);       break;
    case 'j': gPad->KeyDown(kKey_Left);     break;
    case 'k': gPad->KeyDown(kKey_Down);     break;
    case 'l': gPad->KeyDown(kKey_Right);    break;
    }
}

void KeyUpCore( int c )
{
    if( gCpu == NULL )
        return;

    switch( c )
    {
    case 'a': gPad->KeyUp(kKey_A);        break;
    case 'b': gPad->KeyUp(kKey_B);        break;
    case 's': gPad->KeyUp(kKey_Start);    break;
    case 't': gPad->KeyUp(kKey_Select);   break;

    case 'i': gPad->KeyUp(kKey_Up);       break;
    case 'j': gPad->KeyUp(kKey_Left);     break;
    case 'k': gPad->KeyUp(kKey_Down);     break;
    case 'l': gPad->KeyUp(kKey_Right);    break;

    case ' ': gPause = !gPause;             break;
     }
}

#if NOPE

void SpecialKeyDownFunc( int k, int x, int y )
{
    switch( k )
    {
    case GLUT_KEY_RIGHT:
        if( gMultiRenderer != NULL )
        {
            gMultiRenderer->NextRenderer();
            glutPostRedisplay();
        }
        break;
        
    case GLUT_KEY_DOWN:
        gTakeScreenShot = true;
        glutPostRedisplay();
        break;

    case GLUT_KEY_UP:
        gDumpTilesOnce = true;
        break;
    }
}
#endif

extern "C" void InitCore(const char* str);
#if Foo
void InitCore(const char* str)
{
    Options* pOptions = new Options();
    Options& options = *pOptions;
//    options.Parse(argc, argv);
    options.inputFileName = str;
    
    FILE* file = fopen(options.inputFileName.c_str(), "rb");
    if( file == NULL )
    {
        fprintf(stderr, "Failed to open \"%s\"\n", options.inputFileName.c_str());
        return 1;
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
    options.rawGameName = rawGameName;
    options.prettyGameName = FindPrettyGameName(rawGameName);

    std::string windowTitle = std::string("gbhd - ") + options.prettyGameName;
//    glutSetWindowTitle(windowTitle.c_str());

    
    MemoryState* memory = new MemoryState(buffer);
    Z80State* z80 = new Z80State(memory);
    GPUState* gpu = new GPUState(options, memory);
    TimerState* timer = new TimerState(memory);
    Pad* pad = new Pad();
    
    gMultiRenderer = new MultiRenderer();
    gRenderer = gMultiRenderer;
    
    gMultiRenderer->AddRenderer( new DefaultRenderer() );
    //gMultiRenderer->AddRenderer( new SimpleRenderer() );
    gMultiRenderer->AddRenderer( new AccurateRenderer() );
    
    memory->SetCpu(z80);
    memory->SetGpu(gpu);
    memory->SetPad(pad);
    memory->SetTimer(timer);
    
    gpu->SetRenderer( gRenderer );
    
    gCpu = z80;
    gGpu = gpu;
    gPad = pad;
    gTimer = timer;
    gMemory = memory;
    
    // Load up any replacement textures needed
    gpu->LoadReplacementTiles();
}
#endif

#endif

SDL_Window* gWindow = nullptr;
GameBoyState* gConsoleState = nullptr;

void handleConsoleKeyEvent(
    SDL_KeyboardEvent const& event,
    Key consoleKey)
{
    switch (event.type)
    {
    default:
        throw std::exception("unexpected");

    case SDL_EVENT_KEY_DOWN:
        GameBoyState_KeyDown(gConsoleState, consoleKey);
        break;

    case SDL_EVENT_KEY_UP:
        GameBoyState_KeyUp(gConsoleState, consoleKey);
        break;
    }
}

void handleKeyEvent(SDL_KeyboardEvent const& event)
{
    switch (event.scancode)
    {
    case SDL_SCANCODE_UP:
        handleConsoleKeyEvent(event, kKey_Up);
        break;

    case SDL_SCANCODE_DOWN:
        handleConsoleKeyEvent(event, kKey_Down);
        break;

    case SDL_SCANCODE_LEFT:
        handleConsoleKeyEvent(event, kKey_Left);
        break;

    case SDL_SCANCODE_RIGHT:
        handleConsoleKeyEvent(event, kKey_Right);
        break;

    case SDL_SCANCODE_B:
        handleConsoleKeyEvent(event, kKey_B);
        break;

    case SDL_SCANCODE_A:
        handleConsoleKeyEvent(event, kKey_A);
        break;

    case SDL_SCANCODE_TAB:
        handleConsoleKeyEvent(event, kKey_Select);
        break;

    case SDL_SCANCODE_RETURN:
        handleConsoleKeyEvent(event, kKey_Start);
        break;

    case SDL_SCANCODE_ESCAPE:
        exit(0);
        break;

    case SDL_SCANCODE_SPACE:
        GameBoyState_TogglePause(gConsoleState);
        break;
    }

}

void handlePlatformEvent(SDL_Event const& event)
{
    switch (event.type)
    {
    default:
        break;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        handleKeyEvent(event.key);
        break;

    case SDL_EVENT_QUIT:
        exit(0);
        return;
    }

}

void handlePlatformEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        handlePlatformEvent(event);
    }
}

void simulateAndRenderFrame()
{
    SDL_Time currentInstant;
    SDL_GetCurrentTime(&currentInstant);

    // `SDL_Time` is in units of nanoseconds.
    //
    UInt64 currentInstantNumerator = currentInstant;
    UInt64 currentInstantDenominator = 1'000'000'000ull;

    GameBoyState_Update(
        gConsoleState,
        currentInstantNumerator,
        currentInstantDenominator);

    int windowClientAreaWidth = 0;
    int windowClientAreaHeight = 0;
    SDL_GetWindowSizeInPixels(
        gWindow,
        &windowClientAreaWidth,
        &windowClientAreaHeight);

    GameBoyState_RenderGL(
        gConsoleState,
        windowClientAreaWidth,
        windowClientAreaHeight);
}

void runMainLoop()
{
    for (;;)
    {
        handlePlatformEvents();

        simulateAndRenderFrame();
    }
}

int main(
    int argc,
    char** argv)
{
    SDL_SetAppMetadata("gbhd", "0.0", "com.tess-factor.gbhd");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }


    SDL_WindowFlags windowFlags = 0;
    gWindow = SDL_CreateWindow("gbhd", 160, 144, windowFlags);
    if (!gWindow)
    {
        SDL_Log("failed to create window: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    gConsoleState = GameBoyState_Create();
    if (!gConsoleState)
    {
        SDL_Log("failed to create Game Boy state");
        return EXIT_FAILURE;
    }

    GameBoyState_SetMediaPath(gConsoleState, "./media/");
    GameBoyState_SetGamePath(gConsoleState, "./external/game-boy-test-roms/bully/bully.gb");

    GameBoyState_Reset(gConsoleState);
    GameBoyState_Start(gConsoleState);

    runMainLoop();

    return EXIT_SUCCESS;
}
