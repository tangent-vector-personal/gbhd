// Copyright 2011 Theresa Foley. All rights reserved.
//
// gpu.h
#ifndef gbemu_gpu_h
#define gbemu_gpu_h

#include "gb.h"
#include "memory.h"
#include "options.h"
#include "types.h"

#include <map>
#include <string>
#include <vector>

struct Color
{
    UInt8 r, g, b, a;
};

static const int kNativeScreenWidth = 160;
static const int kNativeScreenHeight = 144;

enum TileImageLayer
{
    kTileImageLayer_Background = 0,
    kTileImageLayer_Foreground,
    kTileImageLayerCount,
};

class TileCacheImage
{
public:
    TileCacheImage();
    void Acquire();
    void Release();
    
    void SetImageData(int width, int height, const Color* data );

    void ClearReplacementImage();

    Color pixels[8][8];
    
    uint32_t GetTextureID();
    
private:
    ~TileCacheImage();

    uint32_t _referenceCount;
    uint32_t textureID;
};

class RectF
{
public:
    RectF();
    RectF( float left, float top, float right, float bottom );

    union
    {
        struct
        {
            float left;
            float top;
            float right;
            float bottom;
        };
        float values[4];
    };
};

class TileCacheSubImage
{
public:
    TileCacheSubImage();
    TileCacheSubImage(
        TileCacheImage* image,
        const RectF& rect );
    
    TileCacheImage* image;
    RectF rect;
        
};

class TileCacheNode
{
public:
    TileCacheNode();

    TileCacheNode* GetChildUInt8( UInt8 value );
    TileCacheNode* GetChildUInt4( UInt8 value );
    
    TileCacheSubImage GetSubImage( TileImageLayer layer ) { return images[layer]; }
    void SetSubImage( TileImageLayer layer, const TileCacheSubImage& image ) { this->images[layer] = image; }
    
    void ClearReplacementTiles();

private:

    TileCacheNode* children[16];
    TileCacheSubImage images[kTileImageLayerCount];
};

class IRenderer;

class GPUState
{
public:
    GPUState( const Options& options, MemoryState* memory );
    
    void SetRenderer( IRenderer* renderer )
    {
        _renderer = renderer;
    }

    void Reset();
    
    UInt8 ReadUInt8( UInt16 addr );
    void WriteUInt8( UInt16 addr, UInt8 value );
    
    void CheckLine( UInt8 m );
    void RenderLine();
    
    bool flip;
  
    UInt8 vram[8192];
    UInt8 oam[160];
    
    void LoadReplacementTiles();
    void ClearReplacementTiles();

public:
    enum LcdMode
    {
        kLcdMode_HBlank = 0,
        kLcdMode_VBlank = 1,
        kLcdMode_OamRead = 2,
        kLcdMode_VRamRead = 3,
        
        kLcdMode_Off = kLcdMode_VBlank,
        kLcdMode_Reset = kLcdMode_OamRead,
    };
    
    enum LcdStatus
    {
        kLcdEnableCoincidenceInterrupt = 0x40,
        kLcdEnableMode2Interrupt = 0x20,
        kLcdEnableMode1Interrupt = 0x10,
        kLcdEnableMode0Interrupt = 0x08,
        kLcdCoincidenceFlag = 0x04,
        kLcdModeMask = 0x03,
    };

    void CheckStatusTrigger();
    void DumpTileImage( int tileIndex, UInt8 palette );
    TileCacheImage* LoadReplacementImage(
        const char* name,
        TileImageLayer layer,
        const UInt8* palette);
    void LoadReplacementTile(
        TileImageLayer layer,
        const char* name,
        TileCacheImage* image,
        const RectF& rect);

    const Options& options;
    MemoryState* memory;
    
    union
    {
        UInt8 reg[64];
        struct
        {
            UInt8 lcdControl;
            UInt8 lcdStatus;
            UInt8 bgScrollY;
            UInt8 bgScrollX;
            UInt8 scanLineY;
            UInt8 compareLineY;
            UInt8 dmaTrigger;
            UInt8 mapPalette;
            UInt8 objPalette0;
            UInt8 objPalette1;
            UInt8 winPosY;
            UInt8 winPosX;
        };
    };
    
    enum LcdFlag
    {
        kLcdFlag_LcdOn = 0x80,
        kLcdFlag_WinMapBase = 0x40,
        kLcdFlag_WinOn = 0x20,
        kLcdFlag_MapTileBase = 0x10,
        kLcdFlag_BgMapBase = 0x08,
        kLcdFlag_ObjSize = 0x04,
        kLcdFlag_ObjOn = 0x02,
        kLcdFlag_MapOn = 0x01,        
    };
    bool TestLcdFlag( LcdFlag flag );
    
    LcdMode GetLcdMode();
    void SetLcdMode( LcdMode mode );

    int modeClocks;
    
    enum
    {
        kNativeSpriteCount = 40,
        kVisibleLineCount = 160,
    };
    
    struct ObjData
    {
        int x;
        int y;
        UInt8 tile;
        bool palette;
        bool xFlip;
        bool yFlip;
        bool priority;
    };
    
    ObjData GetObjInfo( int index );
    TileCacheNode* GetTileCacheNode( TileImageLayer layer, int tileIndex );
    
private:    
    TileCacheNode* tileCaches[kTileImageLayerCount];
    
    IRenderer* _renderer;
};

class IRenderer
{
public:
    virtual void RenderLine(
        GPUState* gpu,
        int line ) = 0;
        
    virtual void RenderBlankFrame() = 0;
    
    virtual void Swap() = 0;

    virtual void Present(GBRenderData& outData) = 0;
    
    enum
    {
        kNativeSpriteCount = 40,
        kVisibleLineCount = 160,
    };
    

};

class HighResRenderer :
    public IRenderer
{
public:
    HighResRenderer();

protected:
    void BindMapProgram();
    void BindObjProgram();

private:    
    unsigned int _mapProgram;
    unsigned int _objProgram;
};

class DefaultRenderer :
    public HighResRenderer
{
public:
    DefaultRenderer();
    
    virtual void RenderLine(
        GPUState* gpu,
        int line );
    
    virtual void RenderBlankFrame();
    
    virtual void Swap();
        
    virtual void Present(GBRenderData& outData);

private:
    enum { kMaxVisibleTilesPerLine = 21 };
    struct TileMapState
    {
        TileCacheSubImage images[kTileImageLayerCount][kMaxVisibleTilesPerLine];
        Color palette;
        int tilePixelY;
        int screenPixelMinX;
        int bgMapIndex;
        bool visible;
    };

    struct SpriteState
    {
        TileCacheSubImage image;
        Color palette;
        
        int tilePixelMinX;
        int tilePixelMaxX;
        int tilePixelMinY;
        int tilePixelMaxY;
        
        int screenPixelMinX;
        int screenPixelMaxX;
        
        bool xFlip;
        bool yFlip;
        bool priority;
        bool visible;
    };

    struct FrameState
    {
        bool disabled;
        TileMapState bgMapStates[kVisibleLineCount];
        TileMapState winMapStates[kVisibleLineCount];
        SpriteState spriteStates[kNativeSpriteCount][kVisibleLineCount];
    };
    
    void DrawTileMap( TileMapState* tileMap, TileImageLayer layer );
    void DrawSprites( FrameState& frameState, bool priority );

    void drawRectangle(
        float sMinX, float sMinY,
        float sMaxX, float sMaxY,
        float tMinX, float tMinY,
        float tMaxX, float tMaxY,
        Color palette);

    void drawVertex(
        float sX, float sY,
        float tX, float tY,
        Color palette);

    FrameState frameStates[2];
    int displayFrameStateIndex;
    int updateFrameStateIndex;

    std::vector<GBVertex> _vertices;
};

class SimpleRenderer :
    public HighResRenderer
{
public:
    SimpleRenderer();
    
    virtual void RenderLine(
        GPUState* gpu,
        int line );
    
    virtual void RenderBlankFrame();
    
    virtual void Swap();
        
    virtual void Present(GBRenderData& outData);

private:
    struct TileMapState
    {
        TileCacheSubImage images[kTileImageLayerCount][32*32];
        Color palette;
        int screenPixelX;
        int screenPixelY;
        bool visible;
    };

    struct SpriteState
    {
        TileCacheSubImage images[2];
        Color palette;
        
        int screenPixelX;
        int screenPixelY;
        
        bool xFlip;
        bool yFlip;
        bool priority;
        bool visible;
    };

    struct FrameState
    {
        bool disabled;
        TileMapState bgMapState;
        TileMapState winMapState;
        SpriteState spriteStates[kNativeSpriteCount];
    };
    
    void DrawTileMap( TileMapState& tileMap, TileImageLayer layer );
    void DrawSprites( FrameState& frameState, bool priority );
    
    FrameState frameStates[2];
    int displayFrameStateIndex;
    int updateFrameStateIndex;
};

class AccurateRenderer :
    public IRenderer
{
public:
    AccurateRenderer();
    
    virtual void RenderLine(
        GPUState* gpu,
        int line );
    
    virtual void RenderBlankFrame();
    
    virtual void Swap();
        
    virtual void Present(GBRenderData& outData);

    struct SpriteInfo
    {
        int index;
        GPUState::ObjData obj;
    };

private:
    enum { kFrameBufferSize = 256, };
    Color _frameBuffer[kFrameBufferSize][kFrameBufferSize];
    uint32_t _frameBufferTex;
};

class MultiRenderer :
    public HighResRenderer
{
public:
    MultiRenderer();
    
    void AddRenderer( IRenderer* renderer );
    void NextRenderer();
    
    virtual void RenderLine(
        GPUState* gpu,
        int line );
    
    virtual void RenderBlankFrame();
    
    virtual void Swap();
        
    virtual void Present(GBRenderData& outData);

private:
    typedef std::vector<IRenderer*> RendererList;
    RendererList _renderers;
    
    int _selectedIndex;
};


#endif
