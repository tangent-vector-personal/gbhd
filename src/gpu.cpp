// Copyright 2011 Theresa Foley. All rights reserved.
//
// gpu.cpp
#include "gpu.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include "png.h"

#include "opengl.h"

#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define LCDC = (reg[0])
#define STAT = (reg[1])
#define YSCROLL = (reg[2])
#define XSCROLL = (reg[3])

extern bool gDumpTilesOnce;

GPUState::GPUState( const Options& options, MemoryState* memory )
    : options(options)
    , memory(memory)
{
    for( int ii = 0; ii < kTileImageLayerCount; ++ii )
        tileCaches[ii] = new TileCacheNode();

    Reset();
}

void GPUState::Reset()
{
    memset( vram, 0, sizeof(vram) );
    memset( oam, 0, sizeof(oam) );
    memset( reg, 0, sizeof(reg) );

    lcdControl = 0x91;
    lcdStatus = 0x00; // really?

    mapPalette = 0xFC;
    objPalette0 = 0xFF;
    objPalette1 = 0xFF;
    
//    SetLcdMode( kLcdMode_Off );
    
    modeClocks = 0;    
    flip = false;
}

UInt8 GPUState::ReadUInt8( UInt16 addr )
{
    UInt16 gfxAddr = addr - 0xff40;
    return reg[gfxAddr];
}

void GPUState::WriteUInt8( UInt16 addr, UInt8 value )
{
    UInt16 gfxAddr = addr - 0xff40;
    switch( gfxAddr )
    {
    case 0:
        {
            bool oldLcdOn = TestLcdFlag( kLcdFlag_LcdOn );
            lcdControl = value;
            bool newLcdOn = TestLcdFlag( kLcdFlag_LcdOn );       
            if( newLcdOn != oldLcdOn )
            {
                SetLcdMode( kLcdMode_Reset );
                scanLineY = 0;
                modeClocks = 0;
                CheckStatusTrigger();
            }
//            fprintf(stderr, "LCDC 0x%02X [line %d]\n", value, scanLineY);
        }
        break;
    
    case 1:
        lcdStatus |= (value & ~0x07);
//        fprintf(stderr, "LCD Status 0x%02X\n", lcdStatus);
        break;
    
    case 4:
        scanLineY = 0;
//        fprintf(stderr, "LY 0x%02X\n", 0);
        break;
        
    case 5:
        compareLineY = value;
//        fprintf(stderr, "LYC 0x%02X\n", value);
        break;
    
    case 6:
        for( int ii = 0; ii < 160; ++ii )
        {
            UInt8 v = memory->ReadUInt8((UInt16(value) << 8) + ii);
            oam[ii] = v;
        }
        break;
    
    default:
        /*
        if( reg[gfxAddr] != value )
        {
            static int count = 0;
            fprintf(stderr, "%d: GPU[0x%04X] = 0x%02X (line:%d)\n", count++, addr, value, scanLineY);
        }
        */
        reg[gfxAddr] = value;
        break;
    }    
}

GPUState::ObjData GPUState::GetObjInfo( int index )
{
    int oamBase = index * 4;
    
    ObjData objInfo;
    
    objInfo.y = int( oam[oamBase + 0] ) - 16;
    objInfo.x = int( oam[oamBase + 1] ) - 8;
    objInfo.tile = oam[oamBase + 2];
    
    UInt8 flags = oam[oamBase + 3];
    
    objInfo.palette = (flags & 0x10) != 0;
    objInfo.xFlip = (flags & 0x20) != 0;
    objInfo.yFlip = (flags & 0x40) != 0;
    objInfo.priority = (flags & 0x80) != 0;
    
    return objInfo;
}

void GPUState::CheckLine( UInt8 m )
{
    if( !TestLcdFlag( kLcdFlag_LcdOn ) )
    {
        SetLcdMode( kLcdMode_Off );
        scanLineY = 0;
        modeClocks = 0;
        CheckStatusTrigger();
        return;
    }

    static const int kModeLimits[] = {
        204,    // Mode 0 : H-blank
        456,    // Mode 1 : V-blank
        80,     // Mode 2 : OAM-read
        172,    // Mode 3 : VRAM-read
    };

    LcdMode lineMode = GetLcdMode();

    modeClocks += m;
    while( modeClocks >= kModeLimits[lineMode] )
    {
        modeClocks -= kModeLimits[lineMode];
        
        switch( lineMode )
        {
        case kLcdMode_HBlank:
            // End of H-blank for last scanline; render screen
            if( scanLineY == 143 )
            {
                lineMode = kLcdMode_VBlank;
                // Write the data
                flip = true;
                _renderer->Swap();
                memory->RaiseInterruptLine(kInterruptFlag_VBlank);
            }
            else
            {
                lineMode = kLcdMode_OamRead;
            }
            scanLineY++;
            break;
            
        case kLcdMode_VBlank:
            scanLineY++;
            if( scanLineY > 153 )
            {
                scanLineY = 0;
                lineMode = kLcdMode_OamRead;
            }
            break;
            
        case kLcdMode_OamRead:
            lineMode = kLcdMode_VRamRead;
            break;
            
        case kLcdMode_VRamRead:
            lineMode = kLcdMode_HBlank;
            RenderLine();
        }
        SetLcdMode( lineMode );
        CheckStatusTrigger();
    }
}

void GPUState::CheckStatusTrigger()
{
    if( !TestLcdFlag( kLcdFlag_LcdOn ) )
    {
        lcdStatus &= ~kLcdCoincidenceFlag;
        SetLcdMode( kLcdMode_Off );
    
        memory->LowerInterruptLine(kInterruptFlag_VBlank);
        memory->LowerInterruptLine(kInterruptFlag_LcdStatus);
        
//        _renderer->RenderBlankFrame();
//        _renderer->Swap();
        
        return;
    }
    
    LcdMode lineMode = GetLcdMode();
    if( lineMode != kLcdMode_VBlank )
        memory->LowerInterruptLine(kInterruptFlag_VBlank);

    bool trigger = false;

    if( scanLineY == compareLineY )
    {
        lcdStatus |= kLcdCoincidenceFlag;
        if( lcdStatus & kLcdEnableCoincidenceInterrupt )
        {
            trigger = true;
        }
    }
    else
    {
        lcdStatus &= ~kLcdCoincidenceFlag;
    }
    
    static const int kModeFlags[] = {
        kLcdEnableMode0Interrupt,
        kLcdEnableMode1Interrupt,
        kLcdEnableMode2Interrupt,
        0x00,
    };
    
    if( lcdStatus & kModeFlags[lineMode] )
    {
        trigger = true;
    }
            
    if( !TestLcdFlag( kLcdFlag_LcdOn) )
    {
        assert(!"Unreachable");
        trigger = false;
    }
        
    if( trigger )
    {
        memory->RaiseInterruptLine(kInterruptFlag_LcdStatus);
    }
    else
    {
        memory->LowerInterruptLine(kInterruptFlag_LcdStatus);
    }
}

static Color MakeColor(UInt8 val)
{
    Color result;
    result.r = result.g = result.b = val;
    result.a = 255;
    return result;
}

static UInt8 GetPaletteGreyValue( UInt8 palette, int index )
{
    static const UInt8 kGreyValues[] = {
        255,
        192,
        96,
        0
    };
    
    UInt8 greyIndex = (palette >> (index*2)) & 0x03;
    return kGreyValues[ greyIndex ];
}

static Color GetPaletteColor( UInt8 palette, int index )
{
    return MakeColor( GetPaletteGreyValue(palette, index) );
}

static Color GetPaletteColor( UInt8 palette )
{
    Color result;
    result.r = GetPaletteGreyValue(palette, 1);
    result.g = GetPaletteGreyValue(palette, 2);
    result.b = GetPaletteGreyValue(palette, 3);
    result.a = GetPaletteGreyValue(palette, 0);
    return result;
}

void MakeDirectory(const char* name)
{
#ifdef WIN32
    _mkdir(name);
#else
    mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

void GPUState::DumpTileImage( int tileIndex, UInt8 palette )
{
    if( !options.dumpTilesOnce )
        return;

    static const int kTileSizeInBytes = 16;
    
    static std::map<std::string, int> sMap;

    std::filesystem::path mediaDirectoryPath = options.mediaPath;
    auto gameDirectoryPath = mediaDirectoryPath / options.prettyGameName;
    auto dumpDirectoryPath = gameDirectoryPath / "dump";

    std::filesystem::create_directories(dumpDirectoryPath);

    std::ostringstream imageFileNameBuilder;
    for( int ii = 0; ii < kTileSizeInBytes; ++ii )
    {
        UInt8 tileData = vram[ tileIndex*16 + ii ];

        // TODO: just format this the C++ way...

        char buffer[4];
        sprintf_s(buffer, "%02x", tileData);

        imageFileNameBuilder << buffer;
    }
    imageFileNameBuilder << "p";
    for( int ii = 0; ii < 4; ++ii )
    {
        UInt8 colorValue = (palette >> (ii*2)) & 0x3;

        char buffer[4];
        sprintf_s(buffer, "%01x", colorValue);

        imageFileNameBuilder << buffer;
    }
    imageFileNameBuilder << ".png";

    auto imageFileName = imageFileNameBuilder.str();

    if( sMap.find(imageFileName) != sMap.end() )
        return;

    sMap[imageFileName] = 1;

    // Compute a pixel representation

    static const int kTileWidth = 8;
    static const int kTileHeight = 8;
    Color rows[kTileHeight][kTileWidth];
    UInt8* rowPointers[kTileHeight];
    
    for( int yy = 0; yy < kTileHeight; ++yy )
    {
        rowPointers[yy] = reinterpret_cast<UInt8*>(&rows[yy][0]);
        
        for( int xx = 0; xx < kTileWidth; ++xx )
        {
            UInt16 bgTileRowBase = tileIndex*16 + yy*2;
            UInt8 tileRowBits0 = vram[ bgTileRowBase ];
            UInt8 tileRowBits1 = vram[ bgTileRowBase + 1 ];
            
            UInt8 bitToCheck = 0x80 >> xx;
            UInt8 tilePixelColorIndex =
                    ((tileRowBits0 & bitToCheck) ? 0x01 : 0)
                    | ((tileRowBits1 & bitToCheck) ? 0x02 : 0);
            Color tilePixelColor = GetPaletteColor( palette, tilePixelColorIndex );
            if( tilePixelColorIndex == 0 )
                tilePixelColor.a = 0;
            
            rows[yy][xx] = tilePixelColor;
        }
    }

#if 0
    // Now write out an actual PNG file
    FILE* file = fopen(nameBuffer, "wb");
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
        kTileWidth,
        kTileHeight,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);
        
    png_write_info(pngContext, pngInfo);

    // write image data
    png_write_image(pngContext, rowPointers);
    
    png_write_end(pngContext, NULL);

    fclose(file);
#endif
}

static int HexDigit( char c )
{
    if( (c >= '0') && (c <= '9') )
        return c - '0';
    
    if( (c >= 'a') && (c <= 'f') )
        return 10 + (c - 'a');
    
    if( (c >= 'A') && (c <= 'F') )
        return 10 + (c - 'A');
    
    return 0;
}

void GPUState::LoadReplacementTiles()
{
    auto replacementFilePath =
        std::filesystem::path(options.mediaPath)
        / options.prettyGameName
        / "replace"
        / "replace.txt";

    FILE* file = fopen(replacementFilePath.u8string().c_str(), "r");
    if( file == NULL )
        return;
    char lineBuffer[1024];
    
    TileCacheImage* image = NULL;
    RectF rect;
    UInt8 palette[4] = { 0, 1, 2, 3 };
    TileImageLayer layer = kTileImageLayer_Background;
    
    while( fgets(lineBuffer, 1023, file) != NULL )
    {
        if( lineBuffer[strlen(lineBuffer)-1] == '\n' )
            lineBuffer[strlen(lineBuffer)-1] = 0;
        
        if( lineBuffer[0] == 0 ) continue;
        if( lineBuffer[0] == '/' ) continue;
        if( lineBuffer[0] == '#' ) continue;
        
        const char* delim = " \t";
        const char* cmd = strtok(lineBuffer, delim);
        
        if( strcmp(cmd, "image") == 0 )
        {
            const char* fileName = strtok(NULL, delim);
            if( image != NULL )
                image->Release();
            image = LoadReplacementImage(
                fileName,
                layer,
                palette);
            // load image from fileName 
        }
        else if( strcmp(cmd, "rect") == 0 )
        {
            for( int ii = 0; ii < 4; ++ii )
            {
                const char* token = strtok(NULL, delim);
                rect.values[ii] = (float) atof(token);
            }
        }
        else if( strcmp(cmd, "palette") == 0 )
        {
            const char* p = strtok(NULL, delim);
            for( int ii = 0; ii < 4; ++ii )
            {
                palette[ii] = p[ii] - '0';
            }
        }
        else if( strcmp(cmd, "layer") == 0 )
        {
            const char* layerName = strtok(NULL, delim);
            switch(layerName[0])
            {
            case 'b':
                layer = kTileImageLayer_Background;
                break;
            case 'f':
                layer = kTileImageLayer_Foreground;
                break;
            default:
                fprintf(stderr, "No idea which layer %s means!\n", layerName);
                break;
            }
        }
        else if( strcmp(cmd, "tile") == 0 )
        {
            const char* tileName = strtok(NULL, "");
            LoadReplacementTile(layer, tileName, image, rect);
        }
        else
        {
            fprintf(stderr, "Unknown replacement command %s\n", cmd);
        }
    }
    fclose(file);
    
    if( image != NULL )
        image->Release();
}

void GPUState::ClearReplacementTiles()
{
    for( int ii = 0; ii < kTileImageLayerCount; ++ii )
    {
        tileCaches[ii]->ClearReplacementTiles();
    }
}

struct PaletteEntry
{
    float value;
    Color color;
};

bool operator<(
    const PaletteEntry& left,
    const PaletteEntry& right )
{
    return left.value < right.value;
}

Color operator*( float left, const Color& right )
{
    Color c;
    const UInt8* src = reinterpret_cast<const UInt8*>(&right);
    UInt8* dst = reinterpret_cast<UInt8*>(&c);
    for( int ii = 0; ii < 4; ++ii )
    {
        float s = left * src[ii];
        int i = s;
        if( i < 0 ) i = 0;
        if( i > 255 ) i = 255;
        dst[ii] = i;
    }
    return c;
}

Color operator+( const Color& left, const Color& right )
{
    Color c;
    const UInt8* l = reinterpret_cast<const UInt8*>(&left);
    const UInt8* r = reinterpret_cast<const UInt8*>(&right);
    UInt8* dst = reinterpret_cast<UInt8*>(&c);
    for( int ii = 0; ii < 4; ++ii )
    {
        int i = int(l[ii]) + int(r[ii]);
        if( i > 255 ) i = 255;
        dst[ii] = i;
    }
    return c;
}

TileCacheImage* GPUState::LoadReplacementImage(
    const char* name,
    TileImageLayer layer,
    const UInt8* palette )
{
    char nameBuffer[1024];
    sprintf(nameBuffer, "%s/%s/replace/%s",
        options.mediaPath.c_str(),
        options.prettyGameName.c_str(), name);
    
    FILE* file = fopen(nameBuffer, "rb");
    
#if 1
    throw 99;
#else
    UInt8 header[8];
    fread(header, 1, 8, file);
    if( png_sig_cmp(header, 0, 8) )
        throw 1;

    png_structp pngContext = png_create_read_struct(
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
    png_set_sig_bytes(pngContext, 8);
    
    png_read_info(pngContext, pngInfo);

    int width = png_get_image_width(pngContext, pngInfo);
    int height = png_get_image_height(pngContext, pngInfo);
    int colorType = png_get_color_type(pngContext, pngInfo);
    int bitDepth = png_get_bit_depth(pngContext, pngInfo);
    
    png_set_interlace_handling(pngContext);
    png_read_update_info(pngContext, pngInfo);
    
    
    int bytesPerPixel = 0;
    const int* rgbaRemap = NULL;
    
    static const int kRemapRGB[] = { 0, 1, 2, -2 };
    static const int kRemapRGBA[] = { 0, 1, 2, 3 };
    
    switch( colorType )
    {
    case PNG_COLOR_TYPE_RGBA:
        bytesPerPixel = 4;
        rgbaRemap = kRemapRGBA;
        break;
    case PNG_COLOR_TYPE_RGB:
        bytesPerPixel = 3;
        rgbaRemap = kRemapRGB;
        break;
    default:
        fprintf(stderr, "Can't handle PNG color type %d\n", colorType);
        return NULL;
    }
        
    if( bitDepth != 8 )
        throw 1;

    std::vector<UInt8> rawData;
    rawData.resize( width * height * bytesPerPixel );

    std::vector<UInt8*> rowPointers;
    rowPointers.reserve(height);
    for( int ii = 0; ii < height; ++ii )
    {
        rowPointers.push_back( reinterpret_cast<UInt8*>(&rawData[ ii*width*bytesPerPixel ]) );
    }
    
    png_read_image(pngContext, &rowPointers[0]);
    fclose(file);
    
    std::vector<Color> data;
    data.resize( width * height );
    
    // set up for palette-ification
    //
    std::vector<PaletteEntry> palEntries;
    for( int ii = 0; ii < 4; ++ii )
    {
        // Don't include layer 0 for foreground stuff
        if( ii == 0 && layer == kTileImageLayer_Foreground )
            continue;
            
        static const float kGreyValues[] = {
            1.0f,
            192 / 255.0f,
            96 / 255.0f,
            0.0f,
        };
        static const Color kColors[] = {
            { 0, 0, 0, 255 },
            { 255, 0, 0, 0 },
            { 0, 255, 0, 0 },
            { 0, 0, 255, 0 },
        };

        float greyValue = kGreyValues[ palette[ii] ];
        Color color = kColors[ palette[ii] ];
        
        
        PaletteEntry entry;
        entry.value = greyValue;
        entry.color = color;
        
        palEntries.push_back(entry);
    }
    std::sort(palEntries.begin(), palEntries.end());
    
    int palEntryCount = palEntries.size();
    if( palEntries[0].value != 0.0f )
    {
//        fprintf(stderr, "No palette entry with value 0.0 for image %s!\n", name);
        PaletteEntry entry;
        entry.value = 0.0f;
        Color color = { 0, 0, 0, 0 };
        entry.color = color;
        
        palEntries.push_back(entry);
    }
    if( palEntries[palEntryCount-1].value != 1.0f )
    {
//        fprintf(stderr, "No palette entry with value 1.0 for image %s!\n", name);
        
        // Start adding up other entries, to see if we
        // can get something that adds up to ou
        // Start adding up the entries we *do* have, to see if
        // we can get something that adds up to 1.0f.
        // We will set the weight for the first entry (the highest
        // to 1.0, so we tally it separately).
        float valAccum = 0.0f;
        Color colorAccum = { 0, 0, 0, 0 };
        for( int ii = palEntryCount-1; ii >= 0; --ii )
        {
            float entryVal = palEntries[ii].value;
            Color entryColor = palEntries[ii].color;
            if( valAccum + entryVal >= 1.0f )
            {
                float t = (1.0f - valAccum) / entryVal;
                colorAccum = colorAccum + t*entryColor;
                break;
            }
            else
            {
                valAccum += entryVal;
                colorAccum = colorAccum + entryColor;
            }
        }
        PaletteEntry entry;
        entry.value = 1.0f;
        entry.color = colorAccum;
        
        palEntries.push_back(entry);
    }
    std::sort(palEntries.begin(), palEntries.end());
    palEntryCount = palEntries.size();
    
    for( int yy = 0; yy < height; ++yy )
    for( int xx = 0; xx < width; ++xx )
    {
        const UInt8* src = &rawData[ (yy*width + xx)*bytesPerPixel ];
        Color tmpColor;
        UInt8* tmp = reinterpret_cast<UInt8*>(&tmpColor);
        Color& dstColor = data[ yy*width + xx ];
        for( int cc = 0; cc < 4; ++cc )
        {
            int remap = rgbaRemap[cc];
            if( remap >= 0 )
                tmp[cc] = src[remap];
            else
                tmp[cc] = UInt8(remap+1);
        }
        
        // compute luminance/alpha from input pixel
        float luminance =
              tmpColor.r * 0.2126f/255.0f
            + tmpColor.g * 0.7152f/255.0f
            + tmpColor.b * 0.0722f/255.0f;
        
        // cast this as a linear combination
        // of the palette colors.

        int hi = 0;
        for( ; hi < palEntryCount; ++hi )
        {
            if( luminance < palEntries[hi].value )
                break;
        }
        assert( hi >= 0 );
        assert( hi <= palEntryCount );
        if( hi == palEntryCount )
        {
            dstColor = palEntries[hi-1].color;
        }
        else if ( hi == 0 )
        {
            dstColor = palEntries[hi].color;
        }
        else
        {
            int lo = hi-1;
            const PaletteEntry& loEntry = palEntries[lo];
            const PaletteEntry& hiEntry = palEntries[hi];
            float t = (luminance -  loEntry.value) / (hiEntry.value - loEntry.value);
            dstColor = (1.0f - t) * loEntry.color + t * hiEntry.color;
        }

        // For foreground images, copy alpha over straight
        if( layer == kTileImageLayer_Foreground )
        {
            dstColor.a = tmpColor.a;
        }
    }
    
    
    TileCacheImage* image = new TileCacheImage();    
    
    image->SetImageData(width, height, &data[0]);

    return image;
#endif
}


void GPUState::LoadReplacementTile(
    TileImageLayer layer,
    const char* name,
    TileCacheImage* image,
    const RectF& rect )
{
    image->Acquire();

    TileCacheNode* node = tileCaches[layer];
    const char* n = name;
    while( *n != 0 )
    {
        int a = HexDigit( *n++ );
        node = node->GetChildUInt4(a);
    }
    TileCacheSubImage oldSubImage = node->GetSubImage( layer );
    if( oldSubImage.image != NULL )
    {
        oldSubImage.image->Release();
    }
    
    TileCacheSubImage subImage(image, rect);
    node->SetSubImage( layer, subImage);
}


void GPUState::RenderLine()
{
    _renderer->RenderLine( this, scanLineY );
}

bool GPUState::TestLcdFlag(LcdFlag flag)
{
    return (reg[0] & flag) != 0;
}

GPUState::LcdMode GPUState::GetLcdMode()
{
    return LcdMode( lcdStatus & kLcdModeMask );
}

void GPUState::SetLcdMode( LcdMode mode )
{
    lcdStatus &= ~kLcdModeMask;
    lcdStatus |= mode;
}

//

TileCacheImage::TileCacheImage()
    : _referenceCount(1)
{
    textureID = 0;
}

TileCacheImage::~TileCacheImage()
{
#if 0
    if( textureID != 0 )
    {
        glDeleteTextures(1, &textureID);
    }
#endif
}

void TileCacheImage::Acquire()
{
    _referenceCount++;
}

void TileCacheImage::Release()
{
    _referenceCount--;
    if( _referenceCount == 0 )
        delete this;
}


void TileCacheImage::SetImageData(int width, int height, const Color* data)
{
#if 0
    if( textureID == 0 )
        glGenTextures(1, &textureID);
            
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
}

void TileCacheImage::ClearReplacementImage()
{
    if( textureID != 0 )
    {
        SetImageData(8, 8, &pixels[0][0]);
    }
}

uint32_t TileCacheImage::GetTextureID()
{
    if( textureID == 0 )
    {
        SetImageData(8, 8, &pixels[0][0]);
    }
    return textureID;
}

//

RectF::RectF()
{}

RectF::RectF( float left, float top, float right, float bottom )
    : left(left)
    , top(top)
    , right(right)
    , bottom(bottom)
{}

TileCacheSubImage::TileCacheSubImage()
    : image(NULL)
{}


TileCacheSubImage::TileCacheSubImage(
    TileCacheImage* image,
    const RectF& rect )
    : image(image)
    , rect(rect)
{
}

//


TileCacheNode::TileCacheNode()
{
    memset(children, 0, sizeof(children));
}

TileCacheNode* TileCacheNode::GetChildUInt8( UInt8 value )
{
    TileCacheNode* n = this;
    n = n->GetChildUInt4( (value >> 4) & 0xf );
    n = n->GetChildUInt4( value & 0xf );
    return n;
}

TileCacheNode* TileCacheNode::GetChildUInt4( UInt8 value )
{
    if( children[value] == NULL )
    {
        children[value] = new TileCacheNode();
    }
    return children[value];
}

void TileCacheNode::ClearReplacementTiles()
{
    for( int ii = 0; ii < kTileImageLayerCount; ++ii )
    {
        if( images[ii].image != NULL )
        {
            images[ii].image->Release();
            images[ii].image = NULL;
        }
    }

    for( int ii = 0; ii < 16; ++ii )
    {
        if( children[ii] )
            children[ii]->ClearReplacementTiles();
    }
}


TileCacheNode* GPUState::GetTileCacheNode( TileImageLayer layer, int tileIndex )
{
    TileCacheNode* n = tileCaches[layer];
    
    static const int kBytesPerTile = 16;
    for( int ii = 0; ii < kBytesPerTile; ++ii )
    {
        UInt8 tileData = vram[ tileIndex*16 + ii ];
        n = n->GetChildUInt8( tileData );
    }
    if( n->GetSubImage(layer).image == NULL )
    {
        TileCacheImage* image = new TileCacheImage();
        
        for( int yy = 0; yy < 8; ++yy )
        {
            UInt8 bits0 = vram[ tileIndex*16 + yy*2 ];
            UInt8 bits1 = vram[ tileIndex*16 + yy*2 + 1];
            
            UInt8 bitToCheck = 0x80;
            
            for( int xx = 0; xx < 8; ++xx )
            {
                UInt8 colorIndex =
                    ((bits0 & bitToCheck) ? 0x01 : 0)
                    | ((bits1 & bitToCheck) ? 0x02 : 0);
                
                static const Color kLayerColors[kTileImageLayerCount][4] = {
                    // background
                    { { 0, 0, 0, 255 }, { 255, 0, 0, 0}, {0, 255, 0, 0}, {0, 0, 255, 0} },
                    // foreground
                    { { 0, 0, 0, 0 }, { 255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255} },
                };
                Color color = kLayerColors[layer][colorIndex];
                image->pixels[yy][xx] = color;
            
                bitToCheck >>= 1;
            }
        }        
        
        TileCacheSubImage subImage(image, RectF(0, 0, 1, 1));
        n->SetSubImage(layer, subImage);
    }
    
    return n;
}

static const char* kVertexShaderSource =
"varying vec2 texCoord;\n"
"varying vec4 color;\n"
"void main()\n"
"{\n"
"   texCoord = gl_MultiTexCoord0.xy;\n"
"   color = gl_Color;\n"
"   gl_Position = ftransform();\n"
"}\n"
"\n";

static const char* kMapFragmentShaderSource =
"uniform sampler2D texMap;\n"
"varying vec2 texCoord;\n"
"varying vec4 color;\n"
"void main()\n"
"{\n"
"   vec4 texColor = texture2D(texMap, texCoord);\n"
"   gl_FragColor.xyz = vec3(dot(texColor, color));\n"
"   gl_FragColor.w = 1.0;\n"
"}\n"
"\n";

static const char* kObjFragmentShaderSource =
"uniform sampler2D texMap;\n"
"varying vec2 texCoord;\n"
"varying vec4 color;\n"
"void main()\n"
"{\n"
"   vec4 texColor = texture2D(texMap, texCoord);\n"
"   gl_FragColor.xyz = vec3(dot(texColor.xyz, color.xyz));\n"
"   gl_FragColor.w = texColor.w;\n"
"}\n"
"\n";

#if 0
static GLuint CreateShader( GLenum type, const char* source )
{
    GLuint shader = glCreateShader(type);
    GLint sourceLength = strlen(source);
    glShaderSource(shader, 1, &source, &sourceLength);    
    glCompileShader(shader);
    static const int kBufferSize = 1024;
    char buffer[kBufferSize];
    GLint length = 0;
    glGetShaderInfoLog(shader, kBufferSize, &length, buffer);
    buffer[length] = 0;
    
    if( length > 0 )
    {
        fprintf(stderr, "GLSL: %s\n", buffer);
    }
    return shader;
}

static GLuint CreateProgram(
    const char* vertexSource,
    const char* fragmentSource )
{
    GLuint program = glCreateProgram();
    GLuint vertexShader = CreateShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = CreateShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    static const int kBufferSize = 1024;
    char buffer[kBufferSize];
    GLint length = 0;
    glGetProgramInfoLog(program, kBufferSize, &length, buffer);
    buffer[length] = 0;
    if( length > 0 )
    {
        fprintf(stderr, "GLSL: %s\n", buffer);
    }
    
    glUseProgram(program);
    GLint texMapLoc = glGetUniformLocation(program, "texMap");
    glUniform1i(texMapLoc, 0);
    glUseProgram(0);
    return program;
}
#endif

HighResRenderer::HighResRenderer()
{
#if 0
    _mapProgram = CreateProgram(kVertexShaderSource, kMapFragmentShaderSource);
    _objProgram = CreateProgram(kVertexShaderSource, kObjFragmentShaderSource);
#endif
}

void HighResRenderer::BindMapProgram()
{
#if 0
    glUseProgram(_mapProgram);
    GLint texMapLoc = glGetUniformLocation(_mapProgram, "texMap");
    glUniform1i(texMapLoc, 0);
#endif
}

void HighResRenderer::BindObjProgram()
{
#if 0
    glUseProgram(_objProgram);
    GLint texMapLoc = glGetUniformLocation(_objProgram, "texMap");
    glUniform1i(texMapLoc, 0);
#endif
}

// DefaultRenderer

DefaultRenderer::DefaultRenderer()
{
    memset( frameStates, 0, sizeof(frameStates) );
    displayFrameStateIndex = 0;
    updateFrameStateIndex = 1;    
}

void DefaultRenderer::RenderLine(
    GPUState* gpu,
    int line )
{
    static const int kTileWidth = 8;
    static const int kTileHeight = 8;

    int nativePixelY = line;
    
    FrameState& frameState = frameStates[updateFrameStateIndex];
    frameState.disabled = false;

    for( int ii = 0; ii < kNativeSpriteCount; ++ii )
    {
        GPUState::ObjData obj = gpu->GetObjInfo(ii); // objData[ii];

        int objNativeWidthInPixels = 8;
        int objNativeHeightInPixels = gpu->TestLcdFlag(GPUState::kLcdFlag_ObjSize) ? 16 : 8;
        
        int objNativePixelY = nativePixelY - obj.y;
        
        SpriteState& state = frameState.spriteStates[ii][nativePixelY];

        memset( &state, 0, sizeof(state) );

        if( !gpu->TestLcdFlag(GPUState::kLcdFlag_LcdOn) ) continue;
        if( objNativePixelY < 0 ) continue;
        if( objNativePixelY >= objNativeHeightInPixels ) continue;

        int tileIndex = obj.tile;
        if( gpu->TestLcdFlag(GPUState::kLcdFlag_ObjSize) )
            tileIndex &= ~0x01; // Round down to even for large sprites
        if( objNativePixelY >= 8 )
            tileIndex++;
        
        int tilePixelMinX = 0;
        int tilePixelMaxX = objNativeWidthInPixels;
        
        int tilePixelMinY = objNativePixelY % kTileHeight;
        int tilePixelMaxY = tilePixelMinY + 1;
        
        if( obj.xFlip )
        {
            tilePixelMinX = kTileWidth - tilePixelMinX;
            tilePixelMaxX = kTileWidth - tilePixelMaxX;
        }
        if( obj.yFlip )
        {
            tilePixelMinY = kTileHeight - tilePixelMinY;
            tilePixelMaxY = kTileHeight - tilePixelMaxY;
            if( gpu->TestLcdFlag( GPUState::kLcdFlag_ObjSize ) )
                tileIndex ^= 0x01;
        }
        
        TileImageLayer layer = kTileImageLayer_Foreground;

        UInt8 objPal = obj.palette ? gpu->objPalette1 : gpu->objPalette0;
        TileCacheNode* tileNode = gpu->GetTileCacheNode(layer, tileIndex);
        gpu->DumpTileImage(tileIndex, objPal );
        
        state.image = tileNode->GetSubImage(layer);
        state.palette = GetPaletteColor(objPal);
        
        
        state.tilePixelMinX = tilePixelMinX;
        state.tilePixelMaxX = tilePixelMaxX;
        state.tilePixelMinY = tilePixelMinY;
        state.tilePixelMaxY = tilePixelMaxY;
        
        state.screenPixelMinX = obj.x;
        state.screenPixelMaxX = obj.x + objNativeWidthInPixels;
        
        state.xFlip = obj.xFlip;
        state.yFlip = obj.yFlip;
        state.priority = obj.priority;
        state.visible = true;
    }
    
    {
    TileMapState& bgMapState = frameState.bgMapStates[nativePixelY];
    memset(&bgMapState, 0, sizeof(bgMapState));

    TileMapState& winMapState = frameState.winMapStates[nativePixelY];
    memset(&winMapState, 0, sizeof(winMapState));
    
    bool isLcdOn = gpu->TestLcdFlag( GPUState::kLcdFlag_LcdOn );
    bool isMapOn = gpu->TestLcdFlag( GPUState::kLcdFlag_MapOn );
    bool isWinOn = gpu->TestLcdFlag( GPUState::kLcdFlag_WinOn );
    
    int bgMapIndex = gpu->TestLcdFlag( GPUState::kLcdFlag_BgMapBase ) ? 1 : 0;
    int winMapIndex = gpu->TestLcdFlag( GPUState::kLcdFlag_WinMapBase ) ? 1 : 0;
    
    UInt16 bgMapBase = gpu->TestLcdFlag( GPUState::kLcdFlag_BgMapBase ) ? 0x1C00 : 0x1800;
    UInt16 winMapBase = gpu->TestLcdFlag( GPUState::kLcdFlag_WinMapBase ) ? 0x1C00 : 0x1800;
    
    bool mapTileBase = gpu->TestLcdFlag( GPUState::kLcdFlag_MapTileBase );
    
    if( isLcdOn && isMapOn )
    {
        int bgPixelY = (nativePixelY + gpu->bgScrollY) % 256;
        int bgTileY = bgPixelY / 8;

        int bgFirstTileX = int(gpu->bgScrollX) / 8;
        int bgFirstPixelX = -( int(gpu->bgScrollX) % 8 );
        
        int bgTilePixelY = bgPixelY % 8;
        
        bgMapState.tilePixelY = bgTilePixelY;
        bgMapState.screenPixelMinX = bgFirstPixelX;
        bgMapState.bgMapIndex = (bgMapBase == 0x1c00) ? 1 : 0;
        bgMapState.visible = true;
        bgMapState.palette = GetPaletteColor(gpu->mapPalette);
        
        for( int ii = 0; ii < kMaxVisibleTilesPerLine; ++ii )
        {
            int bgTileX = (bgFirstTileX + ii) % 32;
            
            int tileIndex = UInt32(gpu->vram[ bgMapBase + bgTileY*32 + bgTileX ]);
            if( !mapTileBase && (tileIndex < 128) )
                tileIndex += 256;
                
            for( int ll = 0; ll < kTileImageLayerCount; ++ll )
            {
                TileImageLayer layer = TileImageLayer(ll);
                TileCacheNode* tileNode = gpu->GetTileCacheNode(layer, tileIndex);
                gpu->DumpTileImage(tileIndex, gpu->mapPalette);
                bgMapState.images[layer][ii] = tileNode->GetSubImage(layer);
            }
        }
        
        if( isWinOn )
        {
            // Compute window-related stuff
            int winFirstPixelY = int(gpu->reg[0xA]);
            if( nativePixelY >= winFirstPixelY )
            {                
                int winPixelY = nativePixelY - winFirstPixelY;
                int winTileY = winPixelY / 8;

                int winFirstPixelX = int(gpu->reg[0xB]) - 7;
                int winFirstTileX = 0;
                
                int winTilePixelY = winPixelY % 8;
                
                winMapState.tilePixelY = winTilePixelY;
                winMapState.screenPixelMinX = winFirstPixelX;
                winMapState.bgMapIndex = (gpu->reg[0] & 0x40) ? 1 : 0;
                winMapState.visible = true;
                winMapState.palette = GetPaletteColor(gpu->mapPalette);
                
                int winMapBase = (gpu->reg[0] & 0x40) ? 0x1C00 : 0x1800;
                
                for( int ii = 0; ii < kMaxVisibleTilesPerLine; ++ii )
                {
                    int winTileX = (winFirstTileX + ii) % 32;
                    
                    int tileIndex = UInt32(gpu->vram[ winMapBase + winTileY*32 + winTileX ]);
                    if( !mapTileBase && (tileIndex < 128) )
                        tileIndex += 256;
                        
                    for( int ll = 0; ll < kTileImageLayerCount; ++ll )
                    {
                        TileImageLayer layer = TileImageLayer(ll);
                        TileCacheNode* tileNode = gpu->GetTileCacheNode(layer, tileIndex);
                        gpu->DumpTileImage(tileIndex, gpu->mapPalette);
                        winMapState.images[layer][ii] = tileNode->GetSubImage(layer);
                    }
                }
            }
        }
    }
        
    }
}

        
void DefaultRenderer::RenderBlankFrame()
{
    frameStates[updateFrameStateIndex].disabled = true;
}

void DefaultRenderer::Swap()
{
    std::swap( updateFrameStateIndex, displayFrameStateIndex );
}
    
void DefaultRenderer::Present()
{
    FrameState& frameState = frameStates[displayFrameStateIndex];
    if (frameState.disabled)
    {
        return;
    }

    // The sequencing here is intended to ensure that when
    // replacement graphics have been provided that include
    // things like an alpha channel, we can correctly composite
    // the resulting image.
    //
    // Depending on its state, a sprite may render either in
    // front of or behind the tile maps (background and window),
    // but even in the case where a sprite is rendered behind
    // the background, it still shows as effectively in front
    // of any background pixels that used color #0 from the
    // background palette.
    //
    // Each tile used for tile maps (background or window) is
    // represented by two different images when doing replacement.
    // One of those images represents what to show in places
    // where background color #0 would show through on a real
    // Game Boy, and the other represents the content to show
    // that belongs in front of anything that logically goes
    // behind the background.
    //
    // That sounds really complicated, but it really amounts
    // to saying we have the following layers, from back-most
    // to front-most:
    //
    // * the background layer images for the background tile map
    // * the background layer images for the window time map
    // * the sprites with priority set to be behind the tile maps
    // * the foreground layer images for the background tile map
    // * the foreground layer images for the window tile map
    //
    // There are some additional details to all of this, because of
    // course there is.

    // Draw background layer images for background tile map.
    //
    DrawTileMap(frameState.bgMapStates, kTileImageLayer_Background);

    // Draw background layer images for the window tile map.
    //
    // This step will also set up state (stencil, alpha, whatever) so
    // that any subsequent drawing of the background tile map doesn't
    // show through for pixels that are covered by the window.
    //
    DrawTileMap(frameState.winMapStates, kTileImageLayer_Background);

    // Draw the sprites that want to appear behind the tile maps.
    //
    DrawSprites(frameState, true);

    // Draw the foreground layer images for the background tile map.
    //
    DrawTileMap(frameState.bgMapStates, kTileImageLayer_Foreground);

    // Draw the foreground layer images for the window tile map.
    //
    DrawTileMap(frameState.winMapStates, kTileImageLayer_Foreground);

    // Draw the sprites that want to appear in front of the tile maps.
    //
    DrawSprites(frameState, false);

#if 0
    glDisable(GL_DEPTH_TEST);

    glColor4f(1,1,1,1);
    
    FrameState& frameState = frameStates[displayFrameStateIndex];
    if( frameState.disabled )
    {
        return;
    }

    glEnable(GL_TEXTURE_2D);

    // First, draw each tile map (background and window).
    // We draw all of the pixels of the tile maps in this
    // pass, but we will re-draw some of them in another
    // pass (those that should appear in front of sprites).
    BindMapProgram();
    
    DrawTileMap(frameState.bgMapStates, kTileImageLayer_Background);
    
    // The window should always draw in front of the background,
    // so we use this pass to set up the stencil buffer on
    // and window pixels, and we will then test against this
    // stencil mask when we re-draw background tiles to
    // avoid having them "show through" the window.
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp( GL_NONE, GL_NONE, GL_INCR );
    DrawTileMap(frameState.winMapStates, kTileImageLayer_Background);
    glDisable(GL_STENCIL_TEST);

    // turn on blending for the remaining layers
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //
    BindObjProgram();
    
    // Draw sprites with priority bit set
    // (these should draw behind tile-map pixels with
    // alpha set);
    DrawSprites( frameState, true );
        
    // Now draw the tile-map pixels with alpha set
    // Note: we do not switch shaders here, since these
    // will be drawn as foreground elements.
    
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp( GL_NONE, GL_NONE, GL_NONE );
    DrawTileMap(frameState.bgMapStates, kTileImageLayer_Foreground);
    glDisable(GL_STENCIL_TEST);

    DrawTileMap(frameState.winMapStates, kTileImageLayer_Foreground);
    
    // Finally, draw sprites without the priority bit
    DrawSprites( frameState, false );
    
    glUseProgram(0);
    
    glDisable(GL_BLEND);
#endif
}

static float lerp( float a, float b, float t )
{
    return a + t*(b-a);
}

void DefaultRenderer::DrawTileMap( TileMapState* tileMap, TileImageLayer layer )
{
    TileCacheImage* lastImage = NULL;

    //
    // TODO(tfoley): Note sure why these loops are nested in this
    // order. It would seem to make more sense to but the loop
    // over lines as the outer loop, so that the check for disabled
    // lines could be done once per line.
    //

    for( int jj = 0; jj < kMaxVisibleTilesPerLine; ++jj )
    {    
        for( int ii = 0; ii < kNativeScreenHeight; ++ii )
        {
            const TileMapState& state = tileMap[ii];
            if( !state.visible )
                continue;
                
            TileCacheSubImage subImage = state.images[layer][jj];
            TileCacheImage* image = subImage.image;
#if 0
            if( image != lastImage )
            {
                if( lastImage != NULL )
                {
                    glEnd();
                }
            
                glBindTexture(GL_TEXTURE_2D, image->GetTextureID());
                
                lastImage = image;
                
                glBegin(GL_QUADS);
            }
#endif

            // In terms of actual GB hardware, we are
            // about to render to an area that is 8
            // pixels wide, and 1 pixel tall.
            //
            // The actual number of pixels covered by
            // that rectangle will depend on the
            // resolution of the output framebuffer
            // we are rendering into.
            //
            float sMinX = state.screenPixelMinX + jj*8;
            float sMaxX = state.screenPixelMinX + (jj+1)*8;
            float sMinY = ii;
            float sMaxY = ii + 1;

            //
            // The source data for that 8x1 rectangle
            // will come from the tile sub-image (which
            // could represent original data or
            // replacement data).
            //
            // Similarly to the case above, this is
            // conceptually a single row from a tile
            // image of 8x8 pixels, but in our case
            // the tile image is a sub-rectangle of
            // a source image that could have almost
            // any dimensions.
            //

            float tMinX = 0;
            float tMaxX = 1;
            float tMinY = state.tilePixelY / 8.0f;
            float tMaxY = (state.tilePixelY+1) / 8.0f;

            RectF rect = subImage.rect;
            tMinX = lerp( rect.left, rect.right, tMinX );
            tMaxX = lerp( rect.left, rect.right, tMaxX );
            tMinY = lerp( rect.top, rect.bottom, tMinY );
            tMaxY = lerp( rect.top, rect.bottom, tMaxY );

            auto palette = state.palette;

            // TODO: these properties together define
            // the vertices we want to add to the output.

#if 0
            glColor4ubv((GLubyte*) &state.palette);

            glTexCoord2f(tMinX, tMinY);
            glVertex2f(sMinX, sMinY);

            glTexCoord2f(tMinX, tMaxY);
            glVertex2f(sMinX, sMaxY);

            glTexCoord2f(tMaxX, tMaxY);
            glVertex2f(sMaxX, sMaxY);

            glTexCoord2f(tMaxX, tMinY);
            glVertex2f(sMaxX, sMinY);
#endif
        }
    }

#if 0
    if( lastImage != NULL )
    {
        glEnd();
    }
#endif
}

void DefaultRenderer::DrawSprites( FrameState& frameState, bool priority )
{
//    glDisable(GL_TEXTURE_2D);
//    glColor4f(1, 0, 0, 0.5);

//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // TODO(tfoley): Why are these loops organized like this?
    // I guess it is to hopefully group together draws that
    // use the same texture image as one another.

    for( int ii = 0; ii < 40; ++ii )
    {
        TileCacheImage* lastImage = NULL;
    
        for( int jj = 0; jj < kNativeScreenHeight; ++jj )
        {
            const SpriteState& state = frameState.spriteStates[ii][jj];
            if( !state.visible || (state.priority != priority))
            {
#if 0
                if( lastImage != NULL )
                {
                    glEnd();
                    lastImage = NULL;
                }
#endif
                continue;
            }
            
            const SpriteState& nextState = frameState.spriteStates[ii][jj+1];
            TileCacheSubImage subImage = state.image;
            TileCacheImage* image = subImage.image;
            
#if 0
            if( image != lastImage )
            {
                if( lastImage != NULL )
                {
#if 0
                    glEnd();
#endif
                }

#if 0
                glBindTexture(GL_TEXTURE_2D, image->GetTextureID());
#endif

                lastImage = image;
                
#if 0
                glBegin(GL_QUADS);
#endif
            }
#endif
            
            float sMinX = state.screenPixelMinX;
            float sMaxX = state.screenPixelMaxX;
            float sMinY = jj;
            float sMaxY = jj + 1;
            
            float tMinX = state.tilePixelMinX / 8.0f;
            float tMaxX = state.tilePixelMaxX / 8.0f;
            float tMinY = state.tilePixelMinY / 8.0f;
            float tMaxY = state.tilePixelMaxY / 8.0f;
            
            /*
            if( state.xFlip )
            {
                tMinX = 1.0f - tMinX;
                tMaxX = 1.0f - tMaxX;
            }
            if( state.yFlip )
            {
                tMinY = 1.0f - tMinY;
                tMaxY = 1.0f - tMaxY;
            }
            */
            
            RectF rect = subImage.rect;
            tMinX = lerp( rect.left, rect.right, tMinX );
            tMaxX = lerp( rect.left, rect.right, tMaxX );
            tMinY = lerp( rect.top, rect.bottom, tMinY );
            tMaxY = lerp( rect.top, rect.bottom, tMaxY );

#if 0
            glColor4ubv((GLubyte*) &state.palette);

            glTexCoord2f(tMinX, tMinY);
            glVertex2f(sMinX, sMinY);

            glTexCoord2f(tMinX, tMaxY);
            glVertex2f(sMinX, sMaxY);

            glTexCoord2f(tMaxX, tMaxY);
            glVertex2f(sMaxX, sMaxY);

            glTexCoord2f(tMaxX, tMinY);
            glVertex2f(sMaxX, sMinY);            
#endif
        }
#if 0
        if( lastImage != NULL )
        {
            glEnd();
        }
#endif
    }
//    glDisable(GL_BLEND);
    
//    glEnable( GL_TEXTURE_2D );
//    glColor4f(1,1,1,1);
}

// SimpleRenderer

SimpleRenderer::SimpleRenderer()
{
    memset( frameStates, 0, sizeof(frameStates) );
    displayFrameStateIndex = 0;
    updateFrameStateIndex = 1;
}

void SimpleRenderer::RenderLine(
    GPUState* gpu,
    int line )
{
    static const int kTileWidth = 8;
    static const int kTileHeight = 8;

    if( line != 143 )
        return;
    
    FrameState& frameState = frameStates[updateFrameStateIndex];
    frameState.disabled = false;

    for( int ii = 0; ii < kNativeSpriteCount; ++ii )
    {
        GPUState::ObjData obj = gpu->GetObjInfo(ii); // objData[ii];

        SpriteState& state = frameState.spriteStates[ii];

        memset( &state, 0, sizeof(state) );

        if( !gpu->TestLcdFlag(GPUState::kLcdFlag_LcdOn) ) continue;

        int tileIndex = obj.tile;
        if( gpu->TestLcdFlag(GPUState::kLcdFlag_ObjSize) )
        {
            tileIndex &= ~0x01; // Round down to even for large sprites
            
            // Then start with the second tile for vertical-flip
            if( obj.yFlip )
                tileIndex |= 0x01;
        }
        
        state.screenPixelX = obj.x;
        state.screenPixelY = obj.y;
        state.xFlip = obj.xFlip;
        state.yFlip = obj.yFlip;
        state.priority = obj.priority;
        state.visible = true;
        
        UInt8 objPal = obj.palette ? gpu->objPalette1 : gpu->objPalette0;
        
        state.palette = GetPaletteColor(objPal);

        int tileCount = gpu->TestLcdFlag(GPUState::kLcdFlag_ObjSize) ? 2 : 1;
        for( int jj = 0; jj < tileCount; ++jj )
        {
            TileImageLayer layer = kTileImageLayer_Foreground;
            TileCacheNode* tileNode = gpu->GetTileCacheNode(layer, tileIndex);
            gpu->DumpTileImage(tileIndex, objPal );
            state.images[jj] = tileNode->GetSubImage(layer);
            
            // switch to "other" tile for 8x16 sprite
            tileIndex ^= 0x01;
        }

    }
    
    {
    TileMapState& bgMapState = frameState.bgMapState;
    memset(&bgMapState, 0, sizeof(bgMapState));

    TileMapState& winMapState = frameState.winMapState;
    memset(&winMapState, 0, sizeof(winMapState));
    
    bool isLcdOn = gpu->TestLcdFlag( GPUState::kLcdFlag_LcdOn );
    bool isMapOn = gpu->TestLcdFlag( GPUState::kLcdFlag_MapOn );
    bool isWinOn = gpu->TestLcdFlag( GPUState::kLcdFlag_WinOn );
    
    int bgMapIndex = gpu->TestLcdFlag( GPUState::kLcdFlag_BgMapBase ) ? 1 : 0;
    int winMapIndex = gpu->TestLcdFlag( GPUState::kLcdFlag_WinMapBase ) ? 1 : 0;
    
    UInt16 bgMapBase = gpu->TestLcdFlag( GPUState::kLcdFlag_BgMapBase ) ? 0x1C00 : 0x1800;
    UInt16 winMapBase = gpu->TestLcdFlag( GPUState::kLcdFlag_WinMapBase ) ? 0x1C00 : 0x1800;
    
    bool mapTileBase = gpu->TestLcdFlag( GPUState::kLcdFlag_MapTileBase );
    
    if( isLcdOn && isMapOn )
    {
        int screenPixelX = -( int(gpu->bgScrollX) % 8);
        int screenPixelY = -( int(gpu->bgScrollY) % 8);

        bgMapState.screenPixelX = screenPixelX;
        bgMapState.screenPixelY = screenPixelY;
        bgMapState.visible = true;
        bgMapState.palette = GetPaletteColor(gpu->mapPalette);

        
        int firstTileX = int(gpu->bgScrollX) / 8;
        int firstTileY = int(gpu->bgScrollY) / 8;
        
        for( int xx = 0; xx < 32; ++xx )
        for( int yy = 0; yy < 32; ++yy )
        {
            int tileX = (firstTileX + xx) % 32;
            int tileY = (firstTileY + yy) % 32;
        
            int tileIndex = UInt32(gpu->vram[ bgMapBase + tileY*32 + tileX ]);
            if( !mapTileBase && (tileIndex < 128) )
                tileIndex += 256;
                
            for( int ll = 0; ll < kTileImageLayerCount; ++ll )
            {
                TileImageLayer layer = TileImageLayer(ll);
                TileCacheNode* tileNode = gpu->GetTileCacheNode(layer, tileIndex);
                gpu->DumpTileImage(tileIndex, gpu->mapPalette);
               
                bgMapState.images[layer][yy*32 + xx] = tileNode->GetSubImage(layer);
            }                
        }
        
        if( isWinOn )
        {
            // Compute window-related stuff
            
            int screenPixelX = int(gpu->reg[0xB]) - 7;
            int screenPixelY = int(gpu->reg[0xA]);
            winMapState.screenPixelX = screenPixelX;
            winMapState.screenPixelY = screenPixelY;
            winMapState.visible = true;
            winMapState.palette = GetPaletteColor(gpu->mapPalette);

            
            int winFirstTileX = 0;
            int winFirstTileY = 0;
            int winMapBase = (gpu->reg[0] & 0x40) ? 0x1C00 : 0x1800;

            for( int xx = 0; xx < 32; ++xx )
            for( int yy = 0; yy < 32; ++yy )
            {
                int tileX = (winFirstTileX + xx) % 32;
                int tileY = (winFirstTileY + yy) % 32;
            
                int tileIndex = UInt32(gpu->vram[ winMapBase + tileY*32 + tileX ]);
                if( !mapTileBase && (tileIndex < 128) )
                    tileIndex += 256;
                    
                for( int ll = 0; ll < kTileImageLayerCount; ++ll )
                {
                    TileImageLayer layer = TileImageLayer(ll);
                    TileCacheNode* tileNode = gpu->GetTileCacheNode(layer, tileIndex);
                    gpu->DumpTileImage(tileIndex, gpu->mapPalette);
                   
                    winMapState.images[layer][yy*32 + xx] = tileNode->GetSubImage(layer);
                }                
            }
        }
    }
        
    }
}

        
void SimpleRenderer::RenderBlankFrame()
{
    frameStates[updateFrameStateIndex].disabled = true;
}

void SimpleRenderer::Swap()
{
    std::swap( updateFrameStateIndex, displayFrameStateIndex );
}
    
void SimpleRenderer::Present()
{    
#if 0
    glColor4f(1,1,1,1);
    
    FrameState& frameState = frameStates[displayFrameStateIndex];
    if( frameState.disabled )
    {
        return;
    }

    glEnable(GL_TEXTURE_2D);



    // First, draw each tile map (background and window).
    // We draw all of the pixels of the tile maps in this
    // pass, but we will re-draw some of them in another
    // pass (those that should appear in front of sprites).
    BindMapProgram();
    
    DrawTileMap(frameState.bgMapState, kTileImageLayer_Background);
    
    // The window should always draw in front of the background,
    // so we use this pass to set up the stencil buffer on
    // and window pixels, and we will then test against this
    // stencil mask when we re-draw background tiles to
    // avoid having them "show through" the window.
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp( GL_NONE, GL_NONE, GL_INCR );
    DrawTileMap(frameState.winMapState, kTileImageLayer_Background);
    glDisable(GL_STENCIL_TEST);
    
    // Draw sprites with priority bit set
    // (these should draw behind tile-map pixels with
    // alpha set);
    BindObjProgram();
    
    DrawSprites( frameState, true );
    
    // Now draw the tile-map pixels with alpha set
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0.5f );

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp( GL_NONE, GL_NONE, GL_NONE );
    DrawTileMap(frameState.bgMapState, kTileImageLayer_Foreground);
    glDisable(GL_STENCIL_TEST);

    DrawTileMap(frameState.winMapState, kTileImageLayer_Foreground);
    glDisable( GL_ALPHA_TEST );
    
    // Finally, draw sprites without the priority bit
    DrawSprites( frameState, false );
    
    glUseProgram(0);
#endif
}

void SimpleRenderer::DrawTileMap( TileMapState& tileMap, TileImageLayer layer )
{
    if( !tileMap.visible )
        return;

#if 0
    for( int jj = 0; jj < 32; ++jj )
    {
        TileCacheImage* lastImage = NULL;
    
        for( int ii = 0; ii < 32; ++ii )
        {
            TileCacheSubImage subImage = tileMap.images[layer][jj*32 + ii];
            TileCacheImage* image = subImage.image;
            if( image != lastImage )
            {
                if( lastImage != NULL )
                {
                    glEnd();
                }
            
                glBindTexture(GL_TEXTURE_2D, image->GetTextureID());
                
                lastImage = image;
                
                glBegin(GL_QUADS);
            }
            
            float sMinX = tileMap.screenPixelX + ii*8;
            float sMaxX = tileMap.screenPixelX + (ii+1)*8;
            float sMinY = tileMap.screenPixelY + jj*8;
            float sMaxY = tileMap.screenPixelY + (jj+1)*8;
            
            float tMinX = 0;
            float tMaxX = 1;
            float tMinY = 0;
            float tMaxY = 1;

            RectF rect = subImage.rect;
            tMinX = lerp( rect.left, rect.right, tMinX );
            tMaxX = lerp( rect.left, rect.right, tMaxX );
            tMinY = lerp( rect.top, rect.bottom, tMinY );
            tMaxY = lerp( rect.top, rect.bottom, tMaxY );

            glColor4ubv((GLubyte*) &tileMap.palette);

            glTexCoord2f(tMinX, tMinY);
            glVertex2f(sMinX, sMinY);

            glTexCoord2f(tMinX, tMaxY);
            glVertex2f(sMinX, sMaxY);

            glTexCoord2f(tMaxX, tMaxY);
            glVertex2f(sMaxX, sMaxY);

            glTexCoord2f(tMaxX, tMinY);
            glVertex2f(sMaxX, sMinY);
        }
        
        if( lastImage != NULL )
        {
            glEnd();
        }
    }
#endif
}

void SimpleRenderer::DrawSprites( FrameState& frameState, bool priority )
{
#if 0
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for( int ii = 0; ii < 40; ++ii )
    {
        TileCacheImage* lastImage = NULL;
        
        const SpriteState& state = frameState.spriteStates[ii];
        if( !state.visible || (state.priority != priority))
        {
            if( lastImage != NULL )
            {
                glEnd();
                lastImage = NULL;
            }
            continue;
        }
        
        for( int jj = 0; jj < 2; ++jj )
        {
            TileCacheSubImage subImage = state.images[jj];
            TileCacheImage* image = subImage.image;
            if( image == NULL ) continue;
            
            if( image != lastImage )
            {
                if( lastImage != NULL )
                {
                    glEnd();
                }
            
                glBindTexture(GL_TEXTURE_2D, image->GetTextureID());
                
                lastImage = image;
                
                glBegin(GL_QUADS);
            }
            
            float sMinX = state.screenPixelX;
            float sMaxX = state.screenPixelX + 8;
            float sMinY = state.screenPixelY + jj*8;
            float sMaxY = state.screenPixelY + (jj+1)*8;
            
            float tMinX = 0;
            float tMaxX = 1;
            float tMinY = 0;
            float tMaxY = 1;
            
            if( state.xFlip )
            {
                tMinX = 1.0f - tMinX;
                tMaxX = 1.0f - tMaxX;
            }
            if( state.yFlip )
            {
                tMinY = 1.0f - tMinY;
                tMaxY = 1.0f - tMaxY;
            }
            
            RectF rect = subImage.rect;
            tMinX = lerp( rect.left, rect.right, tMinX );
            tMaxX = lerp( rect.left, rect.right, tMaxX );
            tMinY = lerp( rect.top, rect.bottom, tMinY );
            tMaxY = lerp( rect.top, rect.bottom, tMaxY );

            glColor4ubv((GLubyte*) &state.palette);

            glTexCoord2f(tMinX, tMinY);
            glVertex2f(sMinX, sMinY);

            glTexCoord2f(tMinX, tMaxY);
            glVertex2f(sMinX, sMaxY);

            glTexCoord2f(tMaxX, tMaxY);
            glVertex2f(sMaxX, sMaxY);

            glTexCoord2f(tMaxX, tMinY);
            glVertex2f(sMaxX, sMinY);            
        }
        if( lastImage != NULL )
        {
            glEnd();
        }
    }
    glDisable(GL_BLEND);
#endif
}

// AccurateRenderer

static void CheckGL()
{
#if 0
    int err = glGetError();
    assert( err == 0 );
#endif
}

AccurateRenderer::AccurateRenderer()
{
#if 0
    glGenTextures(1, &_frameBufferTex);
    glBindTexture(GL_TEXTURE_2D, _frameBufferTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    CheckGL();
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        kFrameBufferSize, kFrameBufferSize,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        &_frameBuffer[0][0]);
    CheckGL();
#endif
}

bool operator<(
    const AccurateRenderer::SpriteInfo& left,
    const AccurateRenderer::SpriteInfo& right )
{
    if( left.obj.x != right.obj.x )
        return left.obj.x < right.obj.x;
    return left.index < right.index;
}

void AccurateRenderer::RenderLine(
    GPUState* gpu,
    int line )
{
    int nativePixelY = line;
    // Figure out which sprites are visible on this line
    
    SpriteInfo visibleSprites[kNativeSpriteCount];
    int visibleSpriteCount = 0;
    
    bool objSizeFlag = gpu->TestLcdFlag(GPUState::kLcdFlag_ObjSize);
    
    for( int ii = 0; ii < kNativeSpriteCount; ++ii )
    {
        const GPUState::ObjData& obj = gpu->GetObjInfo(ii);
        int objNativeHeightInPixels = objSizeFlag ? 16 : 8;
        
        int objNativePixelY = nativePixelY - obj.y;

        if( !gpu->TestLcdFlag(GPUState::kLcdFlag_LcdOn) ) continue;
        if( objNativePixelY < 0 ) continue;
        if( objNativePixelY >= objNativeHeightInPixels ) continue;
        
        
        visibleSprites[visibleSpriteCount].index = ii;
        visibleSprites[visibleSpriteCount].obj = obj;
        visibleSpriteCount++;
    }
    
    // Now sort visible sprites into priority order
    std::sort(visibleSprites, visibleSprites + visibleSpriteCount);
    // and then clamp to at most 10 sprites for this line
    if( visibleSpriteCount > 10 )
        visibleSpriteCount = 10;
    
    bool isLcdOn = gpu->TestLcdFlag( GPUState::kLcdFlag_LcdOn );
    bool isMapOn = gpu->TestLcdFlag( GPUState::kLcdFlag_MapOn );
    bool isWinOn = gpu->TestLcdFlag( GPUState::kLcdFlag_WinOn );
    
    bool isBgOn = isLcdOn && isMapOn;
    isWinOn = isWinOn && isBgOn;
    
    UInt16 bgMapBase = gpu->TestLcdFlag( GPUState::kLcdFlag_BgMapBase ) ? 0x1C00 : 0x1800;
    UInt16 winMapBase = gpu->TestLcdFlag( GPUState::kLcdFlag_WinMapBase ) ? 0x1C00 : 0x1800;
    
    bool mapTileBase = gpu->TestLcdFlag( GPUState::kLcdFlag_MapTileBase );

    int bgPixelY = (nativePixelY + gpu->bgScrollY) % 256;

    int bgFirstTileX = int(gpu->bgScrollX) / 8;
    int bgFirstTilePixelX = int(gpu->bgScrollX) % 8;
    
    int bgTileX = bgFirstTileX;
    int bgTileY = bgPixelY / 8;
    int bgTilePixelX = bgFirstTilePixelX;
    int bgTilePixelY = bgPixelY % 8;
    
    
    int winFirstPixelY = int(gpu->reg[0xA]);
    int winPixelY = nativePixelY - winFirstPixelY;
    int winTileY = winPixelY / 8;
    int winTilePixelY = winPixelY % 8;

    int winFirstPixelX = int(gpu->reg[0xB]) - 7;
    
    if( winPixelY < 0 || winFirstPixelX == -7 )
    {
        isWinOn = false;
    }
    
    for( int xx = 0; xx < kNativeScreenWidth; ++xx )
    {
        int nativePixelX = xx;
        
        Color objColor = { 255, 255, 255, 0 };
        bool objPriority = false;
        
        for( int ss = 0; ss < visibleSpriteCount; ++ss )
        {
            const GPUState::ObjData& obj = visibleSprites[ss].obj;

            int objNativeWidthInPixels = 8;
            int objNativeHeightInPixels = objSizeFlag ? 16 : 8;
            
            int objNativePixelX = nativePixelX - obj.x;
            if( objNativePixelX < 0 ) continue;
            if( objNativePixelX >= objNativeWidthInPixels ) continue;
            
            int objNativePixelY = nativePixelY - obj.y;
            
            if( obj.xFlip )
                objNativePixelX = (objNativeWidthInPixels-1) - objNativePixelX;
            if( obj.yFlip )
                objNativePixelY = (objNativeHeightInPixels-1) - objNativePixelY;

            int tileIndex = obj.tile;
            if( objSizeFlag )
            {
                tileIndex &= ~0x01;
                if( objNativePixelY > 8 )
                {
                    tileIndex++;
                    objNativePixelY -= 8;
                }
            }
            
            UInt8 bits0 = gpu->vram[ tileIndex*16 + objNativePixelY*2 ];
            UInt8 bits1 = gpu->vram[ tileIndex*16 + objNativePixelY*2 + 1];
            
            UInt8 bitToCheck = 0x80 >> objNativePixelX;

            UInt8 colorIndex =
                ((bits0 & bitToCheck) ? 0x01 : 0)
                | ((bits1 & bitToCheck) ? 0x02 : 0);

            if( colorIndex == 0 ) continue;
            
            UInt8 objPal = obj.palette ? gpu->objPalette1 : gpu->objPalette0;
            
            objColor = GetPaletteColor(objPal, colorIndex);
            objPriority = obj.priority;
            break;
        }
        

        Color mapColor = { 255, 255, 255, 0 };
        int mapColorIndex = 0;

        if( isBgOn )
        {
            int tileIndex = UInt32(gpu->vram[ bgMapBase + bgTileY*32 + bgTileX ]);
            if( !mapTileBase && (tileIndex < 128) )
                tileIndex += 256;
            
            UInt8 bits0 = gpu->vram[ tileIndex*16 + bgTilePixelY*2 ];
            UInt8 bits1 = gpu->vram[ tileIndex*16 + bgTilePixelY*2 + 1];
            
            UInt8 bitToCheck = 0x80 >> bgTilePixelX;

            UInt8 colorIndex =
                ((bits0 & bitToCheck) ? 0x01 : 0)
                | ((bits1 & bitToCheck) ? 0x02 : 0);

            mapColor = GetPaletteColor(gpu->mapPalette, colorIndex);
            mapColorIndex = colorIndex;
        }

        bgTilePixelX = (bgTilePixelX + 1) % 8;
        if( bgTilePixelX == 0 )            
        {
            bgTilePixelX = 0;
            bgTileX = (bgTileX + 1) % 32;
        }

        int winPixelX = nativePixelX - winFirstPixelX;
        if( isWinOn && winPixelX >= 0 )
        {
            int winTileX = (winPixelX / 8) % 32;
            int winTilePixelX = winPixelX % 8;
            int tileIndex = UInt32(gpu->vram[ winMapBase + winTileY*32 + winTileX ]);
            if( !mapTileBase && (tileIndex < 128) )
                tileIndex += 256;
            
            UInt8 bits0 = gpu->vram[ tileIndex*16 + winTilePixelY*2 ];
            UInt8 bits1 = gpu->vram[ tileIndex*16 + winTilePixelY*2 + 1];
            
            UInt8 bitToCheck = 0x80 >> winTilePixelX;

            UInt8 colorIndex =
                ((bits0 & bitToCheck) ? 0x01 : 0)
                | ((bits1 & bitToCheck) ? 0x02 : 0);

            mapColor = GetPaletteColor(gpu->mapPalette, colorIndex);
            mapColorIndex = colorIndex;
        }

        Color pixelColor = mapColor;
        if( (mapColorIndex == 0 || !objPriority) && objColor.a != 0)
        {
            pixelColor = objColor;
        }

        _frameBuffer[nativePixelY][nativePixelX] = pixelColor;
    }
}

void AccurateRenderer::RenderBlankFrame()
{
    memset(_frameBuffer, 0xFF, sizeof(_frameBuffer));
}

void AccurateRenderer::Swap()
{
#if 0
    glBindTexture(GL_TEXTURE_2D, _frameBufferTex);
    CheckGL();
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0, 0,
        kFrameBufferSize, kFrameBufferSize,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        &_frameBuffer[0][0]);
    CheckGL();
#endif
}
    
void AccurateRenderer::Present()
{
#if 0
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _frameBufferTex);
    glColor4f(1, 1, 1, 1);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(0, 1);
    glVertex2f(0, kFrameBufferSize);
    glTexCoord2f(1, 1);
    glVertex2f(kFrameBufferSize, kFrameBufferSize);
    glTexCoord2f(1, 0);
    glVertex2f(kFrameBufferSize, 0);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
#endif
}


// MultiRenderer

MultiRenderer::MultiRenderer()
    : _selectedIndex(0)
{}

void MultiRenderer::AddRenderer( IRenderer* renderer )
{
    _renderers.push_back( renderer );
}

void MultiRenderer::NextRenderer()
{
    _selectedIndex++;
}

void MultiRenderer::RenderLine(
    GPUState* gpu,
    int line )
{
    for( RendererList::const_iterator
            ii = _renderers.begin(),
            ie = _renderers.end();
        ii != ie;
        ++ii )
    {
        IRenderer* renderer = *ii;
        renderer->RenderLine(gpu, line);
    }
}

void MultiRenderer::RenderBlankFrame()
{
    for( RendererList::const_iterator
            ii = _renderers.begin(),
            ie = _renderers.end();
        ii != ie;
        ++ii )
    {
        IRenderer* renderer = *ii;
        renderer->RenderBlankFrame();
    }
}

void MultiRenderer::Swap()
{
    for( RendererList::const_iterator
            ii = _renderers.begin(),
            ie = _renderers.end();
        ii != ie;
        ++ii )
    {
        IRenderer* renderer = *ii;
        renderer->Swap();
    }
}
    
void MultiRenderer::Present()
{
    _selectedIndex = (_selectedIndex % _renderers.size());
    _renderers[_selectedIndex]->Present();
}   
