#include "Application.h"
#include "Helper.h"
#include "ImGuiManager.h"
#include "Scene.h"
#include "Raytracing.h"
#include <cmath>
#include <algorithm>
#include <imgui.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

// Window class name
static const wchar_t WINDOW_CLASS_NAME[] = L"D3D12MiniPathtracerWindowClass";

// Application class implementation
Application::Application(uint32_t width, uint32_t height, const std::wstring& name) :
    m_hwnd(nullptr),
    m_width(width),
    m_height(height),
    m_title(name),
    m_isRunning(false),
    m_windowCreated(false),
    m_hInstance(nullptr),
    m_currentBackBufferIndex(0),
    m_rtvDescriptorSize(0),
    m_fenceEvent(nullptr),
    m_viewport{},
    m_scissorRect{},
    m_frameCounter(0),
    m_imguiManager(std::make_unique<ImGuiManager>()),
    m_scene(std::make_unique<Scene>()),
    m_raytracing(std::make_unique<Raytracing>()),
    m_isDxrSupported(false)
{
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    
    // Initialize fence values
    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        m_fenceValues[i] = 0;
    }
}

Application::~Application()
{
    // Signal thread to stop
    m_isRunning = false;
    
    // Wait for message thread to finish
    if (m_messageThread.joinable())
    {
        m_messageThread.join();
    }
}

void Application::OnInit()
{
    // Create D3D12 device
    CreateDevice();
    
    // Create command queue
    CreateCommandQueue();
    
    // Create swap chain
    CreateSwapChain();
    
    // Create RTV descriptor heap
    CreateRtvDescriptorHeap();
    
    // Create frame resources
    CreateFrameResources();
    
    // Create synchronization objects
    CreateSynchronizationObjects();
    
    // Initialize ImGui
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    m_swapChain->GetDesc1(&swapChainDesc);
    m_imguiManager->Initialize(m_hwnd, m_device.Get(), m_commandQueue.Get(), SWAP_CHAIN_BUFFER_COUNT, swapChainDesc.Format);
    
    // Check DXR support
    m_isDxrSupported = CheckRaytracingSupport();
    
    if (m_isDxrSupported)
    {
        // Initialize scene
        m_scene->Initialize(m_device.Get());
        
        // Create acceleration structures
        ThrowIfFailed(m_commandAllocators[0]->Reset());
        ThrowIfFailed(m_commandList->Reset(m_commandAllocators[0].Get(), nullptr));
        
        m_scene->BuildAccelerationStructures(m_commandList.Get(), m_commandAllocators[0].Get(), m_commandQueue.Get());
        
        // Close command list after building
        ThrowIfFailed(m_commandList->Close());
        
        // Initialize raytracing
        m_raytracing->Initialize(m_device.Get(), m_width, m_height, SWAP_CHAIN_BUFFER_COUNT);
        
        OutputDebugStringA("DXR initialization completed. Acceleration structures and pipeline are ready.\n");
    }
    else
    {
        OutputDebugStringA("DXR is not supported.\n");
        ExitProcess(1);
    }
}

void Application::OnUpdate()
{
    // Increment frame counter
    m_frameCounter++;
}

void Application::OnRender()
{
    // Check for window resize and update swap chain if needed
    ResizeSwapChain();
    
    // Skip rendering if window is minimized
    if (m_width == 0 || m_height == 0)
    {
        return;
    }

    // Reset command allocator and command list
    ThrowIfFailed(m_commandAllocators[m_currentBackBufferIndex]->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_currentBackBufferIndex].Get(), nullptr));

    // Set necessary state
    m_commandList->SetGraphicsRootSignature(nullptr);
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Check if DXR is enabled and raytracing is initialized
    if (m_isDxrSupported && m_raytracing && m_scene)
    {
        m_raytracing->UpdateDescriptorHeap(m_scene.get(), m_currentBackBufferIndex);

        // Perform raytracing
        m_raytracing->Render(m_commandList.Get(), m_currentBackBufferIndex);
        
        // Copy raytracing output to render target
        m_raytracing->CopyToRenderTarget(m_commandList.Get(), m_renderTargets[m_currentBackBufferIndex].Get());
    }
    else
    {
        // Fallback to clear color if raytracing is not available
        // Indicate that the back buffer will be used as a render target
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[m_currentBackBufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrier);

        // Get the handle for the current render target view
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += m_currentBackBufferIndex * m_rtvDescriptorSize;

        // Set the render target
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Animate clear color based on frame counter
        float t = static_cast<float>(m_frameCounter) * 0.01f;
        const float clearColor[] = {
            0.5f + 0.5f * sinf(t),                    // Red channel
            0.5f + 0.5f * sinf(t + 2.0f * 3.14159f / 3.0f),  // Green channel  
            0.5f + 0.5f * sinf(t + 4.0f * 3.14159f / 3.0f),  // Blue channel
            1.0f                                      // Alpha channel
        };
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    }
    
    // Start ImGui frame
    m_imguiManager->BeginFrame();

    // Create performance window
    ImGui::Begin("Performance Stats");
    
    // Display frame rate
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Frame Rate: %.1f FPS", io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);
    
    // Display frame counter and buffer index
    ImGui::Separator();
    ImGui::Text("Frame Counter: %llu", m_frameCounter);
    ImGui::Text("Current Back Buffer Index: %u", m_currentBackBufferIndex);
    
    // Display swap chain information
    ImGui::Separator();
    ImGui::Text("Swap Chain Buffer Count: %u", SWAP_CHAIN_BUFFER_COUNT);
    ImGui::Text("Window Size: %u x %u", m_width, m_height);
    
    ImGui::End();
    
    // End ImGui frame and render
    m_imguiManager->EndFrame();
    
    // Ensure render target is in correct state for ImGui rendering
    if (m_isDxrSupported && m_raytracing && m_scene)
    {
        // Render target is already in RENDER_TARGET state after CopyToRenderTarget
        // Get the handle for the current render target view
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += m_currentBackBufferIndex * m_rtvDescriptorSize;
        
        // Set the render target for ImGui
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    }
    
    m_imguiManager->Render(m_commandList.Get());
    
    // Indicate that the back buffer will now be used to present
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_renderTargets[m_currentBackBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrier);

    // Close the command list
    ThrowIfFailed(m_commandList->Close());

    // Execute the command list
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame
    ThrowIfFailed(m_swapChain->Present(1, 0));

    // Move to the next frame
    MoveToNextFrame();
}

void Application::OnDestroy()
{
    // Wait for the GPU to be done with all resources
    WaitForGpu();

    // Close the fence event handle
    if (m_fenceEvent != nullptr)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    // Reset raytracing
    m_raytracing.reset();
    
    // Reset scene (which contains acceleration structures)
    m_scene.reset();
    
    // Reset ImGui manager
    m_imguiManager.reset();
    
    // Reset all ComPtr objects
    m_commandList.Reset();
    m_fence.Reset();
    
    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        m_commandAllocators[i].Reset();
        m_renderTargets[i].Reset();
    }
    
    m_rtvHeap.Reset();
    m_swapChain.Reset();
    m_commandQueue.Reset();
    m_device.Reset();
    m_factory.Reset();

#ifdef _DEBUG
    // Report live objects for debugging memory leaks
    ComPtr<IDXGIDebug1> dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
    {
        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_DETAIL));
    }
#endif
}

void Application::ParseCommandLineArgs()
{
    // TODO: Parse command line arguments if needed
}

int Application::Run()
{
    // Store instance handle
    m_hInstance = GetModuleHandle(nullptr);
    
    // Start message pump thread
    m_isRunning = true;
    m_messageThread = std::thread(&Application::MessagePumpThread, this);
    
    // Wait for window creation
    {
        std::unique_lock<std::mutex> lock(m_windowMutex);
        m_windowCreatedCV.wait(lock, [this] { return m_windowCreated.load() || !m_isRunning.load(); });
    }
    
    // Check if window was created successfully
    if (!m_windowCreated)
    {
        return -1;
    }
    
    // Initialize application
    OnInit();
    
    // Main render loop
    while (m_isRunning)
    {
        OnUpdate();
        OnRender();
    }
    
    // Clean up
    OnDestroy();
    
    return 0;
}

void Application::MessagePumpThread()
{
    // Create window in this thread
    CreateApplicationWindow();
    
    // Run message pump
    MSG msg = {};
    while (m_isRunning && GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Destroy window
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void Application::CreateApplicationWindow()
{
    // Register window class
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = m_hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    
    if (!RegisterClassExW(&windowClass))
    {
        MessageBoxW(nullptr, L"Window registration failed!", L"Error", MB_OK | MB_ICONERROR);
        m_isRunning = false;
        m_windowCreatedCV.notify_all();
        return;
    }
    
    // Calculate window rectangle
    RECT windowRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    
    // Create window
    m_hwnd = CreateWindowW(
        WINDOW_CLASS_NAME,
        m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        m_hInstance,
        this
    );
    
    if (!m_hwnd)
    {
        MessageBoxW(nullptr, L"Window creation failed!", L"Error", MB_OK | MB_ICONERROR);
        m_isRunning = false;
        m_windowCreatedCV.notify_all();
        return;
    }
    
    // Show window
    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);
    
    // Signal window created
    {
        std::lock_guard<std::mutex> lock(m_windowMutex);
        m_windowCreated = true;
    }
    m_windowCreatedCV.notify_all();
}

// Window procedure
LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Application* pApplication = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    // Handle ImGui input first
    if (pApplication && pApplication->m_imguiManager)
    {
        LRESULT result = pApplication->m_imguiManager->WndProcHandler(hWnd, message, wParam, lParam);
        if (result != 0)
        {
            return result;
        }
    }

    switch (message)
    {
    case WM_CREATE:
        {
            // Save the Application pointer passed in to CreateWindow
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
            
            // Save window handle
            pApplication = reinterpret_cast<Application*>(pCreateStruct->lpCreateParams);
            if (pApplication)
            {
                pApplication->m_hwnd = hWnd;
            }
        }
        return 0;

    case WM_PAINT:
        // No longer handle painting here as rendering is done in main thread
        ValidateRect(hWnd, nullptr);
        return 0;

    case WM_DESTROY:
        if (pApplication)
        {
            pApplication->m_isRunning = false;
        }
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            if (pApplication)
            {
                pApplication->m_isRunning = false;
            }
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;
    }

    // Handle any messages the switch statement didn't
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// D3D12 initialization helper methods

// Get hardware adapter for creating device
static void GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter = false)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    if(adapter.Get() == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }
    
    *ppAdapter = adapter.Detach();
}

void Application::CreateDevice()
{
#ifdef _DEBUG
    // Enable debug layer
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        
        // Enable additional debug layers
        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController.As(&debugController1)))
        {
            debugController1->SetEnableGPUBasedValidation(TRUE);
            debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
        }
    }
#endif

    // Create DXGI factory
    uint32_t dxgiFactoryFlags = 0;
    
#ifdef _DEBUG
    // Enable additional debug layers for DXGI
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

    // Try to create hardware device
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(m_factory.Get(), &hardwareAdapter, true);

    if (!hardwareAdapter)
    {
        MessageBoxW(nullptr, L"No Direct3D 12 compatible hardware adapter found", L"Error", MB_OK | MB_ICONERROR);
        m_isRunning = false;
        return;
    }

    // Create the D3D12 device
    ThrowIfFailed(D3D12CreateDevice(
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)
    ));

    // Set debug name for device
    m_device->SetName(L"Main D3D12 Device");
}

void Application::CreateCommandQueue()
{
    // Describe and create the command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    
    m_commandQueue->SetName(L"Main Command Queue");
}

void Application::CreateSwapChain()
{
    // Describe and create the swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(m_factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    // Disable ALT+ENTER fullscreen toggle
    ThrowIfFailed(m_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

    // Get swap chain interface
    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    m_swapChain->SetPrivateData(WKPDID_D3DDebugObjectName, 10, "SwapChain");
}

void Application::CreateRtvDescriptorHeap()
{
    // Describe and create a render target view (RTV) descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    // Get descriptor size
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_rtvHeap->SetName(L"RTV Descriptor Heap");
}

void Application::CreateFrameResources()
{
    // Create render target views for each frame
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        // Get buffer from swap chain
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));

        // Create RTV
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;

        // Create command allocator if not already created
        if (!m_commandAllocators[i])
        {
            ThrowIfFailed(m_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&m_commandAllocators[i])));
            
            wchar_t name[32];
            swprintf_s(name, L"Command Allocator %u", i);
            m_commandAllocators[i]->SetName(name);
        }

        // Set debug names
        wchar_t name[32];
        swprintf_s(name, L"Render Target %u", i);
        m_renderTargets[i]->SetName(name);
    }
}

void Application::CreateSynchronizationObjects()
{
    // Create fence
    ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_currentBackBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValues[m_currentBackBufferIndex]++;

    // Create an event handle to use for frame synchronization
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Create command list (in closed state initially)
    ThrowIfFailed(m_device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocators[m_currentBackBufferIndex].Get(),
        nullptr,
        IID_PPV_ARGS(&m_commandList)));
    
    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(m_commandList->Close());

    // Set debug names
    m_fence->SetName(L"Frame Fence");
    m_commandList->SetName(L"Command List");
}

void Application::WaitForGpu()
{
    // Schedule a Signal command in the queue
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_currentBackBufferIndex]));

    // Wait until the fence has been processed
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_currentBackBufferIndex], m_fenceEvent));
    
    // Wait with timeout and warning
    const DWORD timeoutMs = 200;
    DWORD waitResult = WaitForSingleObjectEx(m_fenceEvent, timeoutMs, FALSE);
    
    if (waitResult == WAIT_TIMEOUT)
    {
        // First timeout - output warning and continue waiting
        OutputDebugStringW(L"[WARNING] WaitForGpu: GPU is taking longer than 200ms to complete. Possible performance issue or GPU hang.\n");
        
        // Wait indefinitely for completion (but we've already logged the issue)
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }
    else if (waitResult != WAIT_OBJECT_0)
    {
        // Unexpected error
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"[ERROR] WaitForGpu: WaitForSingleObjectEx failed with result: 0x%08X\n", waitResult);
        OutputDebugStringW(errorMsg);
    }

    // Increment the fence value for the current frame
    m_fenceValues[m_currentBackBufferIndex]++;
}

// This is called after calling Present() so the backbuffer index is already updated in D3D side.
void Application::MoveToNextFrame()
{
    // Schedule a Signal command in the queue
    const uint64_t currentFenceValue = m_fenceValues[m_currentBackBufferIndex];
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

    // Update the frame index
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready
    if (m_fence->GetCompletedValue() < m_fenceValues[m_currentBackBufferIndex])
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_currentBackBufferIndex], m_fenceEvent));
        
        // Wait with timeout and warning
        const DWORD timeoutMs = 200;
        DWORD waitResult = WaitForSingleObjectEx(m_fenceEvent, timeoutMs, FALSE);
        
        if (waitResult == WAIT_TIMEOUT)
        {
            // First timeout - output warning and continue waiting
            wchar_t warningMsg[256];
            swprintf_s(warningMsg, L"[WARNING] MoveToNextFrame: Waiting for fence value %llu is taking longer than 200ms. Possible frame pacing issue.\n", m_fenceValues[m_currentBackBufferIndex]);
            OutputDebugStringW(warningMsg);
            
            // Wait indefinitely for completion (but we've already logged the issue)
            WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
        }
        else if (waitResult != WAIT_OBJECT_0)
        {
            // Unexpected error
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"[ERROR] MoveToNextFrame: WaitForSingleObjectEx failed with result: 0x%08X\n", waitResult);
            OutputDebugStringW(errorMsg);
        }
    }

    // Set the fence value for the next frame
    m_fenceValues[m_currentBackBufferIndex] = currentFenceValue + 1;
}

void Application::ResizeSwapChain()
{
    // Get current window client area size
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    uint32_t width = clientRect.right - clientRect.left;
    uint32_t height = clientRect.bottom - clientRect.top;
    
    // Validate size
    if (width == 0 || height == 0)
    {
        return;
    }
    
    // Check if size actually changed
    if (width == m_width && height == m_height)
    {
        return;
    }
    
    // Wait for GPU to finish all frames
    WaitForGpu();
    
    // Release the resources holding references to the swap chain
    CleanupRenderTargets();
    
    // Resize the swap chain
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    m_swapChain->GetDesc1(&desc);
    ThrowIfFailed(m_swapChain->ResizeBuffers(
        SWAP_CHAIN_BUFFER_COUNT,
        width,
        height,
        desc.Format,
        desc.Flags));
    
    // Update the current back buffer index
    {
        uint64_t latestFenceValue = m_fenceValues[m_currentBackBufferIndex];
        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        
        // Reset all fence values to 0 and restore the latest fence value for current buffer
        std::fill(std::begin(m_fenceValues), std::end(m_fenceValues), 0);
        m_fenceValues[m_currentBackBufferIndex] = latestFenceValue;
    }
    
    // Update width and height
    m_width = width;
    m_height = height;
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    
    // Update viewport
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
    
    // Update scissor rect
    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(width);
    m_scissorRect.bottom = static_cast<LONG>(height);
    
    // Recreate render target views
    CreateFrameResources();
    
    // Resize raytracing output if DXR is enabled
    if (m_isDxrSupported && m_raytracing)
    {
        m_raytracing->Resize(width, height);
    }
}

void Application::CleanupRenderTargets()
{
    // Release render targets
    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        m_renderTargets[i].Reset();
    }
}

bool Application::CheckRaytracingSupport()
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
    
    if (FAILED(hr) || options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
    {
        OutputDebugStringA("Raytracing is not supported on this device.\n");
        return false;
    }
    
    OutputDebugStringA("Raytracing is supported!\n");
    return true;
}

