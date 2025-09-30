#include "Raytracing.h"
#include "Scene.h"
#include "Helper.h"
#include <fstream>
#include <vector>

Raytracing::Raytracing() :
    m_device(nullptr),
    m_width(0),
    m_height(0),
    m_shaderTableEntrySize(0),
    m_CBVSRVUAVdescHeapSize(0)
{
}

Raytracing::~Raytracing()
{
}

void Raytracing::Initialize(ID3D12Device5* device, uint32_t width, uint32_t height, uint32_t swapChainBufferCount)
{
    m_device = device;
    m_width = width;
    m_height = height;
    m_swapChainBufferCount = swapChainBufferCount;
    
    // Create raytracing pipeline
    CreateRaytracingPipeline();

    // Create descriptor heap
    CreateDescriptorHeap();
    
    // Create raytracing output resource
    CreateRaytracingOutputResource();
    
    // Create shader table
    CreateShaderTable();
}


void Raytracing::CreateRaytracingPipeline()
{
    // Compile shaders
    ComPtr<IDxcBlob> rayGenShader = CompileShader(L"shaders/Raytracing.hlsl", L"RayGenShader", L"lib_6_3");
    ComPtr<IDxcBlob> closestHitShader = CompileShader(L"shaders/Raytracing.hlsl", L"ClosestHitShader", L"lib_6_3");
    ComPtr<IDxcBlob> missShader = CompileShader(L"shaders/Raytracing.hlsl", L"MissShader", L"lib_6_3");
    
    // Create root signature
    {
        // Define descriptor ranges
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;
        srvRange.BaseShaderRegister = 0;
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE uavRange = {};
        uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        uavRange.NumDescriptors = 1;
        uavRange.BaseShaderRegister = 0;
        uavRange.RegisterSpace = 0;
        uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        
        // Define root parameters
        D3D12_ROOT_PARAMETER rootParameters[2] = {};
        
        // SRV for acceleration structure (as descriptor table)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRange;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // UAV for output (as descriptor table)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[1].DescriptorTable.pDescriptorRanges = &uavRange;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = 2;
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
        
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr))
        {
            if (error)
            {
                OutputDebugStringA("Root signature serialization failed:\n");
                OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
            }
            ThrowIfFailed(hr);
        }
        
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&m_rtGlobalRootSignature)));
        m_rtGlobalRootSignature->SetName(L"Raytracing Global Root Signature");
    }
    
    // Create raytracing pipeline state object
    {
        std::vector<D3D12_STATE_SUBOBJECT> subobjects;
        
        // DXIL library - Since all shaders are in the same file, we only need one library
        D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
        dxilLibDesc.DXILLibrary.pShaderBytecode = rayGenShader->GetBufferPointer();
        dxilLibDesc.DXILLibrary.BytecodeLength = rayGenShader->GetBufferSize();
        
        // Define exports for all shaders in the library
        D3D12_EXPORT_DESC exports[] = {
            { L"RayGenShader", nullptr, D3D12_EXPORT_FLAG_NONE },
            { L"ClosestHitShader", nullptr, D3D12_EXPORT_FLAG_NONE },
            { L"MissShader", nullptr, D3D12_EXPORT_FLAG_NONE }
        };
        dxilLibDesc.NumExports = _countof(exports);
        dxilLibDesc.pExports = exports;
        
        D3D12_STATE_SUBOBJECT dxilLib = {};
        dxilLib.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        dxilLib.pDesc = &dxilLibDesc;
        subobjects.push_back(dxilLib);
        
        // Hit group
        D3D12_HIT_GROUP_DESC hitGroup = {};
        hitGroup.HitGroupExport = L"HitGroup";
        hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        hitGroup.ClosestHitShaderImport = L"ClosestHitShader";
        
        D3D12_STATE_SUBOBJECT hitGroupSubobject = {};
        hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        hitGroupSubobject.pDesc = &hitGroup;
        subobjects.push_back(hitGroupSubobject);
        
        // Shader config
        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
        shaderConfig.MaxPayloadSizeInBytes = sizeof(float) * 3;  // float3 color
        shaderConfig.MaxAttributeSizeInBytes = sizeof(float) * 2; // float2 barycentrics
        
        D3D12_STATE_SUBOBJECT shaderConfigSubobject = {};
        shaderConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        shaderConfigSubobject.pDesc = &shaderConfig;
        subobjects.push_back(shaderConfigSubobject);
        
        // Global root signature
        D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig = {};
        globalRootSig.pGlobalRootSignature = m_rtGlobalRootSignature.Get();
        
        D3D12_STATE_SUBOBJECT globalRootSigSubobject = {};
        globalRootSigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        globalRootSigSubobject.pDesc = &globalRootSig;
        subobjects.push_back(globalRootSigSubobject);
        
        // Pipeline config
        D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
        pipelineConfig.MaxTraceRecursionDepth = 1;
        
        D3D12_STATE_SUBOBJECT pipelineConfigSubobject = {};
        pipelineConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        pipelineConfigSubobject.pDesc = &pipelineConfig;
        subobjects.push_back(pipelineConfigSubobject);
        
        // Create the state object
        D3D12_STATE_OBJECT_DESC raytracingPipeline = {};
        raytracingPipeline.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        raytracingPipeline.NumSubobjects = static_cast<UINT>(subobjects.size());
        raytracingPipeline.pSubobjects = subobjects.data();
        
        ThrowIfFailed(m_device->CreateStateObject(&raytracingPipeline, IID_PPV_ARGS(&m_rtPipelineState)));
        m_rtPipelineState->SetName(L"Raytracing Pipeline State Object");
    }
    
    OutputDebugStringA("Raytracing pipeline created successfully.\n");
}

void Raytracing::CreateDescriptorHeap()
{
    // Create descriptor heaps for each frame
    m_descHeaps.resize(m_swapChainBufferCount);

    m_CBVSRVUAVdescHeapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for (uint32_t i = 0; i < m_swapChainBufferCount; ++i)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = DescHeapEntries::Count;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.NodeMask = 0;
        
        ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descHeaps[i])));
        
        std::wstring heapName = L"Raytracing Descriptor Heap[" + std::to_wstring(i) + L"]";
        m_descHeaps[i]->SetName(heapName.c_str());
    }
}

void Raytracing::CreateRaytracingOutputResource()
{
    // Create the output resource. The dimensions and format should match the swap-chain
    D3D12_RESOURCE_DESC outputDesc = {};
    outputDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    outputDesc.Alignment = 0;
    outputDesc.Width = m_width;
    outputDesc.Height = m_height;
    outputDesc.DepthOrArraySize = 1;
    outputDesc.MipLevels = 1;
    outputDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    outputDesc.SampleDesc.Count = 1;
    outputDesc.SampleDesc.Quality = 0;
    outputDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    outputDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 0;
    heapProperties.VisibleNodeMask = 0;
    
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &outputDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&m_raytracingOutput)));
    m_raytracingOutput->SetName(L"Raytracing Output");
}

void Raytracing::CreateShaderTable()
{
    
    // Get shader identifiers
    ComPtr<ID3D12StateObjectProperties> stateObjectProps;
    ThrowIfFailed(m_rtPipelineState.As(&stateObjectProps));
    
    void* rayGenShaderIdentifier = stateObjectProps->GetShaderIdentifier(L"RayGenShader");
    void* missShaderIdentifier = stateObjectProps->GetShaderIdentifier(L"MissShader");
    void* hitGroupIdentifier = stateObjectProps->GetShaderIdentifier(L"HitGroup");
    
    if (!rayGenShaderIdentifier || !missShaderIdentifier || !hitGroupIdentifier)
    {
        OutputDebugStringA("Failed to get shader identifiers\n");
        ThrowIfFailed(E_FAIL);
    }

    // Shader table entry size (aligned to D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT)
    const uint32_t shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    const uint32_t shaderTableAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    m_shaderTableEntrySize = AlignSize(shaderIdentifierSize, shaderTableAlignment);
    
    // Calculate shader table size
    const uint32_t shaderTableSize = m_shaderTableEntrySize * 3; // RayGen + Miss + HitGroup
    
    // Create shader table buffer
    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    uploadHeap.CreationNodeMask = 0;
    uploadHeap.VisibleNodeMask = 0;
    
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = shaderTableSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    ThrowIfFailed(m_device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_shaderTable)));
    m_shaderTable->SetName(L"Shader Table");
    
    // Map and write shader identifiers
    {
        uint8_t* pData = nullptr;
        ThrowIfFailed(m_shaderTable->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
        
        // Copy shader identifiers
        memcpy(pData, rayGenShaderIdentifier, shaderIdentifierSize);
        pData += m_shaderTableEntrySize;
        
        memcpy(pData, missShaderIdentifier, shaderIdentifierSize);
        pData += m_shaderTableEntrySize;
        
        memcpy(pData, hitGroupIdentifier, shaderIdentifierSize);
        
        m_shaderTable->Unmap(0, nullptr);
    }
}

ComPtr<IDxcBlob> Raytracing::CompileShader(const std::wstring& filename, const std::wstring& entryPoint, const std::wstring& target)
{
    static ComPtr<IDxcLibrary> library;
    static ComPtr<IDxcCompiler> compiler;
    static ComPtr<IDxcIncludeHandler> includeHandler;
    
    // Initialize DXC on first use
    if (!library)
    {
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
        ThrowIfFailed(library->CreateIncludeHandler(&includeHandler));
    }
    
    // Read shader file
    std::ifstream shaderFile(filename, std::ios::binary);
    if (!shaderFile.is_open())
    {
        std::wstring errorMsg = L"Failed to open shader file: " + filename;
        OutputDebugStringW(errorMsg.c_str());
        throw std::runtime_error("Shader file not found");
    }
    
    shaderFile.seekg(0, std::ios::end);
    size_t fileSize = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);
    
    std::vector<char> shaderSource(fileSize);
    shaderFile.read(shaderSource.data(), fileSize);
    shaderFile.close();
    
    // Create blob from source
    ComPtr<IDxcBlobEncoding> sourceBlob;
    ThrowIfFailed(library->CreateBlobWithEncodingFromPinned(
        shaderSource.data(), static_cast<UINT32>(fileSize), CP_UTF8, &sourceBlob));
    
    // Compile arguments
    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"-E");
    arguments.push_back(entryPoint.c_str());
    arguments.push_back(L"-T");
    arguments.push_back(target.c_str());
    arguments.push_back(L"-HV");
    arguments.push_back(L"2021");
    arguments.push_back(L"-I");
    arguments.push_back(L"shaders/");
    
#ifdef _DEBUG
    arguments.push_back(L"-Zi");
    arguments.push_back(L"-Od");
#else
    arguments.push_back(L"-O3");
#endif
    
    // Compile shader
    ComPtr<IDxcOperationResult> result;
    HRESULT hr = compiler->Compile(
        sourceBlob.Get(),
        filename.c_str(),
        entryPoint.c_str(),
        target.c_str(),
        arguments.data(),
        static_cast<UINT32>(arguments.size()),
        nullptr,
        0,
        includeHandler.Get(),
        &result);
    
    // Check for compilation errors or warnings
    {
        ComPtr<IDxcBlobEncoding> errors;
        if (SUCCEEDED(result->GetErrorBuffer(&errors)) && errors) {
            auto pText = static_cast<const char*>(errors->GetBufferPointer());
            if (pText != nullptr && errors->GetBufferSize() > 0) {
                OutputDebugStringA("Shader compilation messages:\n");
                OutputDebugStringA(pText);
            }
        }
    }
    
    // Check if compilation failed
    if (FAILED(hr))
    {
        OutputDebugStringA("Shader compilation failed\n");
        ThrowIfFailed(hr);
    }

    // Get compiled shader
    ComPtr<IDxcBlob> compiledShader;
    hr = result->GetResult(&compiledShader);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to get compiled shader\n");
        ThrowIfFailed(hr);
    }
    
    std::wstring successMsg = L"Successfully compiled shader: " + filename + L" [" + entryPoint + L"]\n";
    OutputDebugStringW(successMsg.c_str());
    
    return compiledShader;
}

void Raytracing::UpdateDescriptorHeap(Scene* scene, uint32_t frameIndex)
{
    if (m_descHeaps.empty() || !m_raytracingOutput || frameIndex >= m_swapChainBufferCount)
        return;
        
    // Update only the specified frame's descriptor heap
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descHeaps[frameIndex]->GetCPUDescriptorHandleForHeapStart();
    
    // Create SRV for TLAS
    if (scene && scene->GetTLAS())
    {
        D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor = cpuHandle;
        srvDescriptor.ptr += m_CBVSRVUAVdescHeapSize * DescHeapEntries::SRV_TLAS;
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.RaytracingAccelerationStructure.Location = scene->GetTLAS()->GetGPUVirtualAddress();
        m_device->CreateShaderResourceView(nullptr, &srvDesc, srvDescriptor);
    }
    
    // Create UAV for output
    {
        D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptor = cpuHandle;
        uavDescriptor.ptr += m_CBVSRVUAVdescHeapSize * DescHeapEntries::UAV_Output;
        
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        m_device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &uavDesc, uavDescriptor);
    }
}

void Raytracing::Render(ID3D12GraphicsCommandList4* commandList, uint32_t frameIndex)
{
    if (!m_rtPipelineState || m_descHeaps.empty() || frameIndex >= m_swapChainBufferCount)
        return;
    
    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { m_descHeaps[frameIndex].Get() };
    commandList->SetDescriptorHeaps(1, heaps);
    
    // Set pipeline state
    commandList->SetPipelineState1(m_rtPipelineState.Get());
    
    // Set global root signature
    commandList->SetComputeRootSignature(m_rtGlobalRootSignature.Get());
    
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descHeaps[frameIndex]->GetGPUDescriptorHandleForHeapStart();
        gpuHandle.ptr += m_CBVSRVUAVdescHeapSize * DescHeapEntries::SRV_TLAS;
        commandList->SetComputeRootDescriptorTable(0, gpuHandle);
    }
    {
        // Bind descriptor table for UAV (output)
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descHeaps[frameIndex]->GetGPUDescriptorHandleForHeapStart();
        gpuHandle.ptr += m_CBVSRVUAVdescHeapSize * DescHeapEntries::UAV_Output;
        commandList->SetComputeRootDescriptorTable(1, gpuHandle);
    }

    // Dispatch rays
    if (m_shaderTable)
    {
        // DispatchRays
        D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
        
        // Ray generation shader table
        dispatchDesc.RayGenerationShaderRecord.StartAddress = m_shaderTable->GetGPUVirtualAddress();
        dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_shaderTableEntrySize;
        
        // Miss shader table
        dispatchDesc.MissShaderTable.StartAddress = m_shaderTable->GetGPUVirtualAddress() + m_shaderTableEntrySize;
        dispatchDesc.MissShaderTable.SizeInBytes = m_shaderTableEntrySize;
        dispatchDesc.MissShaderTable.StrideInBytes = m_shaderTableEntrySize;
        
        // Hit group table
        dispatchDesc.HitGroupTable.StartAddress = m_shaderTable->GetGPUVirtualAddress() + m_shaderTableEntrySize * 2;
        dispatchDesc.HitGroupTable.SizeInBytes = m_shaderTableEntrySize;
        dispatchDesc.HitGroupTable.StrideInBytes = m_shaderTableEntrySize;
        
        // Dispatch dimensions
        dispatchDesc.Width = m_width;
        dispatchDesc.Height = m_height;
        dispatchDesc.Depth = 1;
        
        commandList->DispatchRays(&dispatchDesc);
    }
}

void Raytracing::CopyToRenderTarget(ID3D12GraphicsCommandList4* commandList, ID3D12Resource* renderTarget)
{
    if (!m_raytracingOutput || !renderTarget)
        return;
    
    // Transition resources
    D3D12_RESOURCE_BARRIER barriers[2] = {};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = m_raytracingOutput.Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = renderTarget;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    commandList->ResourceBarrier(2, barriers);
    
    // Copy resource
    commandList->CopyResource(renderTarget, m_raytracingOutput.Get());
    
    // Transition back
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    
    commandList->ResourceBarrier(2, barriers);
}

void Raytracing::Resize(uint32_t width, uint32_t height)
{
    if (m_width == width && m_height == height)
        return;
    
    m_width = width;
    m_height = height;
    
    // Recreate output resource
    CreateRaytracingOutputResource();
}

