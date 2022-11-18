// AudioVisual.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "audiovisual.h"
#include "logging.h"
#include "oscillator_ui.h"
#include "plotting.h"

// Data (imgui)
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations (imgui)
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations (this lib)
static int paCallback(const void*                     /*inputBuffer*/,
                      void*                           outputBuffer,
                      unsigned long                   framesPerBuffer,
                      const PaStreamCallbackTimeInfo* /*timeInfo*/,
                      PaStreamCallbackFlags           /*statusFlags*/,
                      void*                           /*userData*/);

PaStream* InitializePAStream()
{
    PaHostApiIndex const numAPIs = Pa_GetHostApiCount();
    if (numAPIs < 0)
        return nullptr;

    PaHostApiInfo const* hostApiInfo;
    PaDeviceIndex devIndex = 0;
    for (PaHostApiIndex i = 0; i < numAPIs; i++)
    {
        hostApiInfo = Pa_GetHostApiInfo(i);
        //if (hostApiInfo && hostApiInfo->type == paASIO)
        if (hostApiInfo && hostApiInfo->type == paWASAPI)
        {
            devIndex = hostApiInfo->defaultOutputDevice;
            break;
        }
    }

    // WASAPI-specific stream parameters
    auto wasapiStreamInfo = std::make_unique<PaWasapiStreamInfo>();
    {
        wasapiStreamInfo->size = sizeof(PaWasapiStreamInfo);
        wasapiStreamInfo->hostApiType = PaHostApiTypeId::paWASAPI;
        wasapiStreamInfo->version = 1;
        //wasapiStreamInfo->flags = paWinWasapiExclusive; // requires software limiting. watch your ears :)
        wasapiStreamInfo->flags = paWinWasapiThreadPriority;
        wasapiStreamInfo->threadPriority = PaWasapiThreadPriority::eThreadPriorityProAudio;
        wasapiStreamInfo->streamCategory = PaWasapiStreamCategory::eAudioCategoryGameMedia;
        wasapiStreamInfo->streamOption = PaWasapiStreamOption::eStreamOptionRaw;
    }

    // ASIO-specific stream parameters. Use if using ASIO.
    //auto asioStreamInfo = std::make_unique<PaAsioStreamInfo>();
    //struct IntTwo { int data[2]{ 0, 1 }; };
    //auto channelSelectors = std::make_unique<IntTwo>();
    //{
    //    asioStreamInfo->size = sizeof(PaAsioStreamInfo);
    //    asioStreamInfo->hostApiType = paASIO;
    //    asioStreamInfo->version = 1;
    //    asioStreamInfo->flags = paAsioUseChannelSelectors;
    //    asioStreamInfo->channelSelectors = (int*)&channelSelectors->data;
    //}

    PaStreamParameters outputParameters;
    outputParameters.channelCount = 2;
    outputParameters.device = devIndex;
    outputParameters.hostApiSpecificStreamInfo = wasapiStreamInfo.get();
    //outputParameters.hostApiSpecificStreamInfo = asioStreamInfo.get();
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;

    PaError err;
    PaStream* stream;
    err = Pa_OpenStream(&stream,
        NULL,
        &outputParameters,
        SAMPLE_RATE,
        paFramesPerBufferUnspecified,
        paClipOff,
        paCallback,
        nullptr);
    assert(err == PaErrorCode::paNoError);
    err = Pa_StartStream(stream);
    assert(err == PaErrorCode::paNoError);
    
    return stream;
}

// This function runs on the realtime thread provided by portaudio.
// It should not make any system calls (incl. allocation). It should primarily
// process requests to change its settings, enqueue responses to those requests,
// and write the next section of samples to the audio device.
static int paCallback(const void*                     /*inputBuffer*/,
                      void*                           outputBuffer,
                      unsigned long                   framesPerBuffer,
                      const PaStreamCallbackTimeInfo* /*timeInfo*/,
                      PaStreamCallbackFlags           /*statusFlags*/,
                      void*                           /*userData*/)
{
    ProcessModifyGeneratorRequests();

    Generator<>& generator = GeneratorAccess::getInstance();
    float*       out       = static_cast<float*>(outputBuffer);

    generator.writeSamples(std::span<float>(out, framesPerBuffer * 2ul));
    
#if LOG_SESSION_TO_FILE
    Logging::CopyBufferAndDefer(out, framesPerBuffer);
#endif

    return paContinue;
}

int APIENTRY wWinMain(_In_ HINSTANCE    /*hInstance*/,
                     _In_opt_ HINSTANCE /*hPrevInstance*/,
                     _In_ LPWSTR        /*lpCmdLine*/,
                     _In_ int           /*nCmdShow*/)
{
    if (Pa_Initialize() != paNoError)
        return -1;

    WaveTables::Initialize();

    PaStream* stream = InitializePAStream();
    if (stream == nullptr)
        return -1;

    {
        ImGui_ImplWin32_EnableDpiAwareness();
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("AudioVisual Test"), NULL };
        ::RegisterClassEx(&wc);
        HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("AudioVisual Test"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

        // Initialize Direct3D
        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            ::UnregisterClass(wc.lpszClassName, wc.hInstance);
            return 1;
        }

        // Show the window
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // Main loop
        bool done = false;
        while (!done)
        {
            MSG msg;
            while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            // Handle communication from realtime thread
            (void)ThreadCommunication::processDeferredActions();
            GetUIOscillatorView().HandleRealTimeResponse();

            // Start the Dear ImGui frame
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Generator Settings");
            GetUIOscillatorView().Show();
            ImGui::End();

            ImGui::Begin("Debug Info");
            ShowDebugInfo(stream);
            ImGui::End();

#if LOG_SESSION_TO_FILE
            ImGui::Begin("Oscillator Plot");
            auto [logBufferLeft, logBufferRight] = Logging::GetLogBuffers();
            Plotting::DrawOscilatorPlot(logBufferLeft.get(), logBufferRight.get());
            ImGui::End();
#endif

            // Rendering
            ImGui::Render();
            const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            g_pSwapChain->Present(1, 0); // vsync on
        }

        // Cleanup
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        ImPlot::DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

#if LOG_SESSION_TO_FILE
    Logging::WriteSessionToFile();
#endif

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
