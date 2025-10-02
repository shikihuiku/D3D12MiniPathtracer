#include "Scene.h"
#include "Helper.h"
#include "RaytracingHelpers.h"
#include <format>

// DXR related constants (if not defined in SDK)
#ifndef D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT
#define D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT 256
#endif

namespace
{
    // Cornell Box geometry data
    class CornellBoxGeometry
    {
    public:
        static std::vector<Vertex> GetVertices()
        {
            std::vector<Vertex> vertices;
            
            // Cornell Box dimensions
            const float boxSize = 5.0f;
            const float halfSize = boxSize / 2.0f;
            
            // Floor (white)
            vertices.push_back({ XMFLOAT3(-halfSize, -halfSize, -halfSize), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            vertices.push_back({ XMFLOAT3( halfSize, -halfSize, -halfSize), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            vertices.push_back({ XMFLOAT3( halfSize, -halfSize,  halfSize), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            vertices.push_back({ XMFLOAT3(-halfSize, -halfSize,  halfSize), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            
            // Ceiling (white)
            vertices.push_back({ XMFLOAT3(-halfSize,  halfSize, -halfSize), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            vertices.push_back({ XMFLOAT3(-halfSize,  halfSize,  halfSize), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            vertices.push_back({ XMFLOAT3( halfSize,  halfSize,  halfSize), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            vertices.push_back({ XMFLOAT3( halfSize,  halfSize, -halfSize), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            
            // Back wall (white)
            vertices.push_back({ XMFLOAT3(-halfSize, -halfSize,  halfSize), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            vertices.push_back({ XMFLOAT3( halfSize, -halfSize,  halfSize), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            vertices.push_back({ XMFLOAT3( halfSize,  halfSize,  halfSize), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            vertices.push_back({ XMFLOAT3(-halfSize,  halfSize,  halfSize), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f) });
            
            // Left wall (red)
            vertices.push_back({ XMFLOAT3(-halfSize, -halfSize, -halfSize), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f) });
            vertices.push_back({ XMFLOAT3(-halfSize, -halfSize,  halfSize), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f) });
            vertices.push_back({ XMFLOAT3(-halfSize,  halfSize,  halfSize), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f) });
            vertices.push_back({ XMFLOAT3(-halfSize,  halfSize, -halfSize), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f) });
            
            // Right wall (green)
            vertices.push_back({ XMFLOAT3( halfSize, -halfSize, -halfSize), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.1f, 0.8f, 0.1f, 1.0f) });
            vertices.push_back({ XMFLOAT3( halfSize,  halfSize, -halfSize), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.1f, 0.8f, 0.1f, 1.0f) });
            vertices.push_back({ XMFLOAT3( halfSize,  halfSize,  halfSize), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.1f, 0.8f, 0.1f, 1.0f) });
            vertices.push_back({ XMFLOAT3( halfSize, -halfSize,  halfSize), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.1f, 0.8f, 0.1f, 1.0f) });
            
            return vertices;
        }
        
        static std::vector<uint32_t> GetIndices()
        {
            std::vector<uint32_t> indices;
            
            // Each wall is made of 2 triangles (6 indices per wall)
            // 5 walls total = 30 indices
            
            // Floor
            indices.insert(indices.end(), { 0, 1, 2,    0, 2, 3 });
            
            // Ceiling
            indices.insert(indices.end(), { 4, 5, 6,    4, 6, 7 });
            
            // Back wall
            indices.insert(indices.end(), { 8, 9, 10,   8, 10, 11 });
            
            // Left wall
            indices.insert(indices.end(), { 12, 13, 14,  12, 14, 15 });
            
            // Right wall
            indices.insert(indices.end(), { 16, 17, 18,  16, 18, 19 });
            
            return indices;
        }
    };
}

Scene::Scene() :
    m_device(nullptr),
    m_vertexCount(0),
    m_indexCount(0),
    m_isBuilt(false)
{
}

Scene::~Scene()
{
    // Explicitly reset resources in proper order
    // Temporary resources first
    m_uploadTemporaryHeapManager.Free(m_vertexBufferOffset);
    m_uploadTemporaryHeapManager.Free(m_indexBufferOffset);
    m_uploadTemporaryHeapManager.Free(m_instanceDescBufferOffset);

    m_defaultTemporaryHeapManager.Free(m_blasScratchBufferOffset);
    m_defaultTemporaryHeapManager.Free(m_tlasScratchBufferOffset);

    m_readbackHeapManager.Free(m_blasPostBuildInfoBufferOffset);
    m_readbackHeapManager.Free(m_tlasPostBuildInfoBufferOffset);

    m_ASHeapManager.Free(m_topLevelASOffset);
    m_ASHeapManager.Free(m_bottomLevelASOffset);
}

void Scene::Initialize(ID3D12Device5* device)
{
    m_device = device;
    m_isBuilt = false;
}

void Scene::BuildAccelerationStructures(ID3D12GraphicsCommandList4* commandList,
                                       ID3D12CommandAllocator* commandAllocator,
                                       ID3D12CommandQueue* commandQueue)
{
    if (m_isBuilt)
    {
        OutputDebugStringA("Scene acceleration structures already built.\n");
        return;
    }
    
    if (!m_device)
    {
        OutputDebugStringA("Error: Scene not initialized with device.\n");
        return;
    }

    // allocate 10MB for the default heap. 1KB per element.
    m_ASHeapManager.Initialize(m_device, 1024 * 10, 256, D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "AS Heap", false);
    m_defaultTemporaryHeapManager.Initialize(m_device, 1024 * 10, 256, D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, "Scene Default temporary Heap");

    // allocate 10MB for the upload heap. 1KB per element.
    m_uploadTemporaryHeapManager.Initialize(m_device, 1024 * 10, 256, D3D12_HEAP_TYPE_GPU_UPLOAD, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, "Scene Upload temporary Heap");

    // allocate 32KB for the readback heap. 256B per element.
    m_readbackHeapManager.Initialize(m_device, 32 * 1024, 256, "SceneReadback Heap");

    // Create geometry
    CreateCornellBoxGeometry();
    
    // Build acceleration structures
    CreateBottomLevelAS(commandList);
    CreateTopLevelAS(commandList);
    
    // Close and execute command list
    ThrowIfFailed(commandList->Close());
    
    ID3D12CommandList* commandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, commandLists);

    // Reset command list for future use
    ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));

    // Wait for GPU to complete building
    WaitForGPU(commandQueue);
    
    // Read back post-build info for debugging
    ReadbackPostBuildInfo();
    
    FreeTemporaryResources();

    m_isBuilt = true;
    OutputDebugStringA("Scene acceleration structures built successfully.\n");
}

void Scene::CreateCornellBoxGeometry()
{
    // Get Cornell Box vertices and indices
    auto vertices = CornellBoxGeometry::GetVertices();
    auto indices = CornellBoxGeometry::GetIndices();
    
    m_vertexCount = static_cast<uint32_t>(vertices.size());
    m_indexCount = static_cast<uint32_t>(indices.size());
    
    const UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    const UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(uint32_t));

    m_vertexBufferOffset = m_uploadTemporaryHeapManager.Allocate(vertexBufferSize);
    m_indexBufferOffset = m_uploadTemporaryHeapManager.Allocate(indexBufferSize);
    
    memcpy(m_uploadTemporaryHeapManager.GetMappedPtr(m_vertexBufferOffset), vertices.data(), vertexBufferSize);
    memcpy(m_uploadTemporaryHeapManager.GetMappedPtr(m_indexBufferOffset), indices.data(), indexBufferSize);
    
    OutputDebugStringA("Cornell Box geometry created successfully.\n");
}

void Scene::CreateBottomLevelAS(ID3D12GraphicsCommandList4* commandList)
{
    // Check that we have created geometry
    if (m_vertexBufferOffset == 0 || m_indexBufferOffset == 0)
    {
        OutputDebugStringA("Error: Create geometry before building acceleration structures.\n");
        return;
    }
    
    // Describe the geometry
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_uploadTemporaryHeapManager.GetGPUVirtualAddress(m_vertexBufferOffset);
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
    geometryDesc.Triangles.VertexCount = m_vertexCount;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;  // Position only
    geometryDesc.Triangles.IndexBuffer = m_uploadTemporaryHeapManager.GetGPUVirtualAddress(m_indexBufferOffset);
    geometryDesc.Triangles.IndexCount = m_indexCount;
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;  // No per-geometry transform
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    
    // Get required sizes for acceleration structure buffers
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &geometryDesc;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    // Print prebuild info
    {
        OutputDebugStringA("BLAS Prebuild Info:\n");
        OutputDebugStringA(std::format("Scratch Data Size: {} bytes\n", prebuildInfo.ScratchDataSizeInBytes).c_str());
        OutputDebugStringA(std::format("Result Data Max Size: {} bytes\n", prebuildInfo.ResultDataMaxSizeInBytes).c_str());
    }
    
    // Allocate scratch buffer
    m_blasScratchBufferOffset = m_defaultTemporaryHeapManager.Allocate(static_cast<uint32_t>(prebuildInfo.ScratchDataSizeInBytes));
    
    // Allocate BLAS buffer
    m_bottomLevelASOffset = m_ASHeapManager.Allocate(static_cast<uint32_t>(prebuildInfo.ResultDataMaxSizeInBytes));
    
    // Create post-build info buffer for BLAS (must be UAV-compatible)
    m_blasPostBuildInfoBufferOffset = m_readbackHeapManager.Allocate(sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC));

    // Transition buffers to UAV state
    m_defaultTemporaryHeapManager.Transition(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_ASHeapManager.Transition(commandList, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

    m_readbackHeapManager.GPUWriteBegin(commandList);
    
    // Build BLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.DestAccelerationStructureData = m_ASHeapManager.GetGPUVirtualAddress(m_bottomLevelASOffset);
    buildDesc.ScratchAccelerationStructureData = m_defaultTemporaryHeapManager.GetGPUVirtualAddress(m_blasScratchBufferOffset);
    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildInfoDesc = {};
    postBuildInfoDesc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE;
    postBuildInfoDesc.DestBuffer = m_readbackHeapManager.GetGPUVirtualAddress(m_blasPostBuildInfoBufferOffset);

    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 1, &postBuildInfoDesc);
    
    // Insert UAV barriers to ensure BLAS and post-build info writes complete
    m_defaultTemporaryHeapManager.UAVBarrier(commandList);
    m_ASHeapManager.UAVBarrier(commandList);
    
    // Copy post-build info to readback buffer
    m_readbackHeapManager.GPUWriteEnd(commandList);

    OutputDebugStringA("Bottom Level Acceleration Structure created successfully.\n");
}

void Scene::CreateTopLevelAS(ID3D12GraphicsCommandList4* commandList)
{
    // Check that we have created BLAS
    if (m_bottomLevelASOffset == 0)
    {
        OutputDebugStringA("Error: Create BLAS before building TLAS.\n");
        return;
    }
    
    // Create instance description buffer
    {
        D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
        instanceDesc.InstanceID = 0;
        instanceDesc.InstanceMask = 0xFF;  // Visible to all rays
        instanceDesc.InstanceContributionToHitGroupIndex = 0;
        instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instanceDesc.AccelerationStructure = m_ASHeapManager.GetGPUVirtualAddress(m_bottomLevelASOffset);
        
        // Set identity transform
        instanceDesc.Transform[0][0] = 1.0f;
        instanceDesc.Transform[1][1] = 1.0f;
        instanceDesc.Transform[2][2] = 1.0f;
        
        // Upload instance description to GPU
        m_instanceDescBufferOffset = m_uploadTemporaryHeapManager.Allocate(sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        memcpy(m_uploadTemporaryHeapManager.GetMappedPtr(m_instanceDescBufferOffset), &instanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    }
    
    // Get required sizes for TLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = 1;  // One instance of Cornell Box
    inputs.InstanceDescs = m_uploadTemporaryHeapManager.GetGPUVirtualAddress(m_instanceDescBufferOffset);
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
    
    // Allocate scratch buffer
    m_tlasScratchBufferOffset = m_defaultTemporaryHeapManager.Allocate(static_cast<uint32_t>(prebuildInfo.ScratchDataSizeInBytes));
    
    // Allocate TLAS buffer
    m_topLevelASOffset = m_ASHeapManager.Allocate(static_cast<uint32_t>(prebuildInfo.ResultDataMaxSizeInBytes));

    // Create post-build info buffer for TLAS (must be UAV-compatible)
    m_tlasPostBuildInfoBufferOffset = m_readbackHeapManager.Allocate(sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC));

    m_defaultTemporaryHeapManager.Transition(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_ASHeapManager.Transition(commandList, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
    m_readbackHeapManager.GPUWriteBegin(commandList);
    
    // Build TLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.DestAccelerationStructureData = m_ASHeapManager.GetGPUVirtualAddress(m_topLevelASOffset);
    buildDesc.ScratchAccelerationStructureData = m_defaultTemporaryHeapManager.GetGPUVirtualAddress(m_tlasScratchBufferOffset);
    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildInfoDesc = {};
    postBuildInfoDesc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE;
    postBuildInfoDesc.DestBuffer = m_readbackHeapManager.GetGPUVirtualAddress(m_tlasPostBuildInfoBufferOffset);
    
    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 1, &postBuildInfoDesc);
    
    // Insert UAV barriers to ensure TLAS and post-build info writes complete
    m_defaultTemporaryHeapManager.UAVBarrier(commandList);
    m_ASHeapManager.UAVBarrier(commandList);
    m_readbackHeapManager.GPUWriteEnd(commandList);

    OutputDebugStringA("Top Level Acceleration Structure created successfully.\n");
}

void Scene::WaitForGPU(ID3D12CommandQueue* commandQueue)
{
    // Create temporary fence for synchronization
    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    
    // Create event
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
    
    // Signal and wait
    const UINT64 fenceValue = 1;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));
    ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
    WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
    
    // Cleanup
    CloseHandle(fenceEvent);
}

void Scene::ReadbackPostBuildInfo()
{
    if (m_blasPostBuildInfoBufferOffset == 0 || m_tlasPostBuildInfoBufferOffset == 0)
    {
        OutputDebugStringA("Warning: Post-build info readback buffers not available.\n");
        return;
    }
    
    // Read BLAS post-build info
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC* pData = 
        static_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC*>(m_readbackHeapManager.GetMappedPtr(m_blasPostBuildInfoBufferOffset));

        if (pData)
        {
            char debugMsg[256];
            sprintf_s(debugMsg, "BLAS Current Size: %llu bytes (%.2f KB)\n", 
                     pData->CurrentSizeInBytes,
                     pData->CurrentSizeInBytes / 1024.0);
            OutputDebugStringA(debugMsg);
        }
        else
        {
            OutputDebugStringA("Warning: Failed to map BLAS post-build info readback buffer.\n");
        }
    }
    
    // Read TLAS post-build info
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC* pData = 
        static_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC*>(m_readbackHeapManager.GetMappedPtr(m_tlasPostBuildInfoBufferOffset));

        if (pData)
        {
            char debugMsg[256];
            sprintf_s(debugMsg, "TLAS Current Size: %llu bytes (%.2f KB)\n", 
                     pData->CurrentSizeInBytes,
                     pData->CurrentSizeInBytes / 1024.0);
            OutputDebugStringA(debugMsg);
        }
        else
        {
            OutputDebugStringA("Warning: Failed to map TLAS post-build info readback buffer.\n");
        }
    }
}

void Scene::FreeTemporaryResources()
{
    m_uploadTemporaryHeapManager.Free(m_vertexBufferOffset);
    m_uploadTemporaryHeapManager.Free(m_indexBufferOffset);
    m_uploadTemporaryHeapManager.Free(m_instanceDescBufferOffset);

    m_defaultTemporaryHeapManager.Free(m_blasScratchBufferOffset);
    m_defaultTemporaryHeapManager.Free(m_tlasScratchBufferOffset);

    m_readbackHeapManager.Free(m_blasPostBuildInfoBufferOffset);
    m_readbackHeapManager.Free(m_tlasPostBuildInfoBufferOffset);
}
