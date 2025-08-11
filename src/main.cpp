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

#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

#include "gb.h"

static const int kDefaultWindowScale = 4;

SDL_Window* gWindow = nullptr;
GameBoyState* gConsoleState = nullptr;

ComPtr<ID3D11Device> _d3dDevice;
ComPtr<ID3D11DeviceContext> _d3dContext;
ComPtr<IDXGIFactory2> _dxgiFactory;
ComPtr<IDXGISwapChain1> _dxgiSwapChain;
ComPtr<ID3D11RenderTargetView> _d3dRenderTarget;
DXGI_FORMAT gSwapChainFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

ComPtr<ID3D11VertexShader> _vertexShader;
ComPtr<ID3D11PixelShader> _fragmentShader;

int createSwapChainResources()
{
    ComPtr<ID3D11Texture2D> backBuffer = nullptr;
    if (FAILED(_dxgiSwapChain->GetBuffer(
        0,
        IID_PPV_ARGS(&backBuffer))))
    {
        std::cout << "D3D11: Failed to get Back Buffer from the SwapChain\n";
        return 1;
    }

    if (FAILED(_d3dDevice->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        &_d3dRenderTarget)))
    {
        std::cout << "D3D11: Failed to create RTV from Back Buffer\n";
        return 1;
    }

    return 0;
}

int destroySwapChainResources()
{
    _d3dRenderTarget.Reset();
    return 0;
}

int handleSwapChainResize()
{
    _d3dContext->Flush();

    destroySwapChainResources();

    int windowClientAreaWidth = 0;
    int windowClientAreaHeight = 0;
    SDL_GetWindowSizeInPixels(
        gWindow,
        &windowClientAreaWidth,
        &windowClientAreaHeight);


    if (FAILED(_dxgiSwapChain->ResizeBuffers(
        0,
        windowClientAreaWidth,
        windowClientAreaHeight,
        gSwapChainFormat,
        0)))
    {
        return 1;
    }

    createSwapChainResources();

    return 0;

}

const char* kShaderSource = R"(
struct Vertex
{
    float2 position : POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};
Vertex vertexMain(
    Vertex vertex,
    out float4 sv_position : SV_Position)
{
    sv_position = float4(vertex.position, 0.5, 1.0);
    return vertex;
}
Texture2D texImage : register(t0);
SamplerState samplerState : register(s0);
float4 tileFragmentMain(
    Vertex vertex)
    : SV_Target
{
    float4 vertexColor = vertex.color;
    float4 textureColor = texImage.Sample(samplerState, vertex.texCoord);
    return dot(vertexColor, textureColor);
}
float4 objFragmentMain(
    Vertex vertex)
    : SV_Target
{
    float4 vertexColor = vertex.color;
    float4 textureColor = texImage.Sample(samplerState, vertex.texCoord);
    float3 color = dot(textureColor.xyz, vertexColor.xyz);
    float alpha = textureColor.w;
    return float4(color, alpha);
}
)";

// Note: the tile and object cases above rely on the fact that
// we are using the R,G,B, and A components of colors to hold
// the data related to palette indices 1, 2, 3, and *0*,
// respectively (note how index 0 has been swapped over to the
// alpha component).
//
// For background-related stuff, all four palette indices matter,
// and we perform a dot product between the weights stored in
// the texel color and the values associated with each
// palette entry. The result is used to establish the color of
// any output that gets written.
// TODO: the alpha should probably always be 1.0 for those...
//
// For object stuff, it needs to use the alpha channel to store
// actual transparency info, but that works because palette
// index 0 is used in those cases to represent a fully
// transparent color (independent of what is written in the
// palette data itself). In the object case we use the
// alpha value from the texel as-is, and just do a dot
// product to sum the contributions for the other three
// components.

int compileShader(
    const char* source,
    const char* entryPointName,
    const char* profileName,
    ComPtr<ID3DBlob>& outBlob)
{
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    ComPtr<ID3DBlob> shaderBlob = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    if (FAILED(D3DCompile(
        source,
        strlen(source),
        "gbhd.hlsl",
        nullptr,
        nullptr,
        entryPointName,
        profileName,
        compileFlags,
        0,
        &shaderBlob,
        &errorBlob)))
    {
        std::cout << "D3D11: Failed to read shader from file\n";
        if (errorBlob != nullptr)
        {
            std::cout << "D3D11: With message: " <<
                static_cast<const char*>(errorBlob->GetBufferPointer()) << "\n";
        }

        return E_FAIL;
    }

    outBlob = std::move(shaderBlob);
    return S_OK;
}

ComPtr<ID3DBlob> compiledVertexShaderCode;

int initVertexShader()
{
    ComPtr<ID3DBlob> compiledCode;
    if (FAILED(compileShader(kShaderSource, "vertexMain", "vs_4_0", compiledCode)))
        return E_FAIL;

    if (FAILED(_d3dDevice->CreateVertexShader(
        compiledCode->GetBufferPointer(),
        compiledCode->GetBufferSize(),
        nullptr,
        _vertexShader.GetAddressOf())))
    {
        return E_FAIL;
    }

    compiledVertexShaderCode = compiledCode;

    return S_OK;
}

int initFragmentShader()
{
    ComPtr<ID3DBlob> compiledCode;
    if (FAILED(compileShader(kShaderSource, "tileFragmentMain", "ps_4_0", compiledCode)))
        return E_FAIL;

    if (FAILED(_d3dDevice->CreatePixelShader(
        compiledCode->GetBufferPointer(),
        compiledCode->GetBufferSize(),
        nullptr,
        _fragmentShader.GetAddressOf())))
    {
        return E_FAIL;
    }
    return S_OK;
}

int initShaders()
{
    if (initVertexShader())
        return 1;
    if (initFragmentShader())
        return 1;
    return 0;
}

ComPtr<ID3D11InputLayout> gD3DInputLayout;

int initInputLayout()
{
    D3D11_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(GBVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(GBVertex, texCoord), D3D11_INPUT_PER_VERTEX_DATA, 0},
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(GBVertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    if (FAILED(_d3dDevice->CreateInputLayout(
        inputElements,
        _countof(inputElements),
        compiledVertexShaderCode->GetBufferPointer(),
        compiledVertexShaderCode->GetBufferSize(),
        &gD3DInputLayout)))
    {
        std::cout << "D3D11: Failed to create default vertex input layout\n";
        return E_FAIL;
    }
    return S_OK;
}

ComPtr<ID3D11Buffer> gSingleTriVertexBuffer;

int initSingleTriVertexBuffer()
{
    GBVertex vertices[] =
    {
        { { 0, 0}, {0,0}, { 1, 0, 0, 1}},
        { { 0, 1}, {0,1}, { 0, 1, 0, 1}},
        { { 1, 0}, {1,0}, { 0, 0, 1, 1}},
    };

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    if (FAILED(_d3dDevice->CreateBuffer(
        &bufferDesc,
        &initData,
        &gSingleTriVertexBuffer)))
    {
        std::cout << "D3D11: Failed to create triangle vertex buffer\n";
        return E_FAIL;
    }
    return S_OK;
}

ComPtr<ID3D11Buffer> gFullVertexBuffer;
int gFullVertexCount = 0;

int ensureFullVertexBuffer(int minVertexCount)
{
    if (minVertexCount <= gFullVertexCount)
        return S_OK;

    gFullVertexCount = minVertexCount;

    gFullVertexBuffer.Reset();

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = gFullVertexCount * sizeof(GBVertex);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(_d3dDevice->CreateBuffer(
        &bufferDesc,
        nullptr,
        &gFullVertexBuffer)))
    {
        std::cout << "D3D11: Failed to create triangle vertex buffer\n";
        return E_FAIL;
    }
    return S_OK;
}

ComPtr<ID3D11SamplerState> gSamplerState;

int initSamplerState()
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    if (FAILED(_d3dDevice->CreateSamplerState(&samplerDesc, gSamplerState.GetAddressOf())))
    {
        throw 99;
    }
    return S_OK;
}


int initD3D11()
{
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory))))
        return 1;

    UINT deviceFlags = 0;
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

    constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
    if (FAILED(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        deviceFlags,
        &deviceFeatureLevel,
        1,
        D3D11_SDK_VERSION,
        &_d3dDevice,
        nullptr,
        &_d3dContext)))
    {
        return 1;
    }

    int windowClientAreaWidth = 0;
    int windowClientAreaHeight = 0;
    SDL_GetWindowSizeInPixels(
        gWindow,
        &windowClientAreaWidth,
        &windowClientAreaHeight);

    DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
    swapChainDescriptor.Width = windowClientAreaWidth;
    swapChainDescriptor.Height = windowClientAreaHeight;
    swapChainDescriptor.Format = gSwapChainFormat;
    swapChainDescriptor.SampleDesc.Count = 1;
    swapChainDescriptor.SampleDesc.Quality = 0;
    swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescriptor.BufferCount = 2;
    swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDescriptor.Scaling = DXGI_SCALING_STRETCH;
    swapChainDescriptor.Flags = {};

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor = {};
    swapChainFullscreenDescriptor.Windowed = true;

    auto windowHandle = (HWND)SDL_GetPointerProperty(
        SDL_GetWindowProperties(gWindow),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER,
        nullptr);


    if (FAILED(_dxgiFactory->CreateSwapChainForHwnd(
        _d3dDevice.Get(),
        windowHandle,
        &swapChainDescriptor,
        &swapChainFullscreenDescriptor,
        nullptr,
        &_dxgiSwapChain)))
    {
        std::cout << "DXGI: Failed to create swapchain\n";
        return false;
    }

    createSwapChainResources();

    initShaders();
    initInputLayout();

    initSingleTriVertexBuffer();

    initSamplerState();

    return 0;
}







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


ID3D11ShaderResourceView* ensureTexture(GBTexture* gbTexture)
{
    if (gbTexture->backEndState > 0)
        return (ID3D11ShaderResourceView*) gbTexture->backEndViewPtr;

    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

    D3D11_TEXTURE2D_DESC resourceDesc = {};
    resourceDesc.Width = gbTexture->width;
    resourceDesc.Height = gbTexture->height;
    resourceDesc.MipLevels = 1;
    resourceDesc.ArraySize = 1;
    resourceDesc.Format = format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Usage = D3D11_USAGE_IMMUTABLE;
    resourceDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = gbTexture->data;
    initData.SysMemPitch = gbTexture->width * 4;

    ID3D11Texture2D* resource = nullptr;
    if (FAILED(_d3dDevice->CreateTexture2D(
        &resourceDesc,
        &initData,
        &resource)))
    {
        throw 99;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Format = format;
    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = 1;
    viewDesc.Texture2D.MostDetailedMip = 0;

    ID3D11ShaderResourceView* view = nullptr;
    if (FAILED(_d3dDevice->CreateShaderResourceView(
        resource,
        &viewDesc,
        &view)))
    {
        throw 99;
    }

    gbTexture->backEndResourcePtr = resource;
    gbTexture->backEndViewPtr = view;
    gbTexture->backEndState = 1;
    return view;
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

    auto renderData = GameBoyState_Render(
        gConsoleState);
//        windowClientAreaWidth,
//        windowClientAreaHeight);

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(windowClientAreaWidth);
    viewport.Height = static_cast<float>(windowClientAreaHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    constexpr float clearColor[] = { 0.2f, 0.5f, 0.7f, 0.0f };

    _d3dContext->ClearRenderTargetView(
        _d3dRenderTarget.Get(),
        clearColor);
    _d3dContext->RSSetViewports(
        1,
        &viewport);
    _d3dContext->OMSetRenderTargets(
        1,
        _d3dRenderTarget.GetAddressOf(),
        nullptr);



    _d3dContext->IASetInputLayout(gD3DInputLayout.Get());
    _d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    _d3dContext->VSSetShader(
        _vertexShader.Get(),
        nullptr,
        0);
    _d3dContext->PSSetShader(
        _fragmentShader.Get(),
        nullptr,
        0);


    UINT vertexStride = sizeof(GBVertex);
    UINT vertexOffset = 0;

    _d3dContext->IASetVertexBuffers(
        0,
        1,
        gSingleTriVertexBuffer.GetAddressOf(),
        &vertexStride,
        &vertexOffset);
    _d3dContext->Draw(3, 0);

    if (renderData.vertexCount > 0)
    {
        ensureFullVertexBuffer(renderData.vertexCount);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (FAILED(_d3dContext->Map(gFullVertexBuffer.Get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mapped)))
        {
            throw 99;
        }

        memcpy(mapped.pData, renderData.vertices, renderData.vertexCount * sizeof(GBVertex));

        _d3dContext->Unmap(gFullVertexBuffer.Get(), 0);

        _d3dContext->IASetVertexBuffers(
            0,
            1,
            gFullVertexBuffer.GetAddressOf(),
            &vertexStride,
            &vertexOffset);

        _d3dContext->PSSetSamplers(
            0, 1,
            gSamplerState.GetAddressOf());

        for (int i = 0; i < renderData.spanCount; ++i)
        {
            auto& span = renderData.spans[i];
            ID3D11ShaderResourceView* texture = ensureTexture(span.texture);

            _d3dContext->PSSetShaderResources(
                0, 1,
                &texture);

            _d3dContext->Draw(span.vertexCount, span.startVertex);
        }
    }

    _dxgiSwapChain->Present(1, 0);

}

void runMainLoop()
{
    for (;;)
    {
        handlePlatformEvents();

        simulateAndRenderFrame();
    }
}


int initGameBoy()
{
    gConsoleState = GameBoyState_Create();
    if (!gConsoleState)
    {
        SDL_Log("failed to create Game Boy state");
        return 1;
    }

    GameBoyState_SetMediaPath(gConsoleState, "./media/");
    GameBoyState_SetGamePath(gConsoleState, "./external/game-boy-test-roms/bully/bully.gb");

    GameBoyState_Reset(gConsoleState);
    GameBoyState_Start(gConsoleState);
    return 0;
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
    gWindow = SDL_CreateWindow(
        "gbhd",
        kDefaultWindowScale * 160,
        kDefaultWindowScale * 144,
        windowFlags);
    if (!gWindow)
    {
        SDL_Log("failed to create window: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (auto err = initD3D11())
    {
        return err;
    }

    if (auto err = initGameBoy())
    {
        return err;
    }



    runMainLoop();

    return EXIT_SUCCESS;
}
