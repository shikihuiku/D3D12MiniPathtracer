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
    m_blasScratchBuffer.Reset();
    m_tlasScratchBuffer.Reset();
    m_instanceDescBuffer.Reset();
    m_blasPostBuildInfoBuffer.Reset();
    m_tlasPostBuildInfoBuffer.Reset();
    m_blasPostBuildInfoReadback.Reset();
    m_tlasPostBuildInfoReadback.Reset();
    
    // Acceleration structures
    m_topLevelAS.Reset();
    m_bottomLevelAS.Reset();
    
    // Geometry buffers
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
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
    
    // Common heap properties for upload buffers
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    // Common resource description for buffers
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    // Create vertex buffer
    resourceDesc.Width = vertexBufferSize;
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)));
    
    m_vertexBuffer->SetName(L"Cornell Box Vertex Buffer");
    
    void* mappedData = nullptr;
    ThrowIfFailed(m_vertexBuffer->Map(0, nullptr, &mappedData));
    memcpy(mappedData, vertices.data(), vertexBufferSize);
    m_vertexBuffer->Unmap(0, nullptr);
    
    // Create index buffer
    resourceDesc.Width = indexBufferSize;
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_indexBuffer)));
    
    m_indexBuffer->SetName(L"Cornell Box Index Buffer");
    
    ThrowIfFailed(m_indexBuffer->Map(0, nullptr, &mappedData));
    memcpy(mappedData, indices.data(), indexBufferSize);
    m_indexBuffer->Unmap(0, nullptr);
    
    OutputDebugStringA("Cornell Box geometry created successfully.\n");
}

void Scene::CreateBottomLevelAS(ID3D12GraphicsCommandList4* commandList)
{
    // Check that we have created geometry
    if (!m_vertexBuffer || !m_indexBuffer)
    {
        OutputDebugStringA("Error: Create geometry before building acceleration structures.\n");
        return;
    }
    
    // Describe the geometry
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
    geometryDesc.Triangles.VertexCount = m_vertexCount;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;  // Position only
    geometryDesc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
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
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = AlignSize(prebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_blasScratchBuffer)));
        
        m_blasScratchBuffer->SetName(L"BLAS Scratch Buffer");
    }
    
    // Allocate BLAS buffer
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = AlignSize(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
            nullptr,
            IID_PPV_ARGS(&m_bottomLevelAS)));
        
        m_bottomLevelAS->SetName(L"Bottom Level Acceleration Structure");
    }
    
    // Create post-build info buffer for BLAS (must be UAV-compatible)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_blasPostBuildInfoBuffer)));
        
        m_blasPostBuildInfoBuffer->SetName(L"BLAS Post-Build Info Buffer");
    }

    // Create post-build info readback buffer
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_blasPostBuildInfoReadback)));
        
        m_blasPostBuildInfoReadback->SetName(L"BLAS Post-Build Info Readback");
    }

    // Transition buffers to UAV state
    {
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Transition.pResource = m_blasScratchBuffer.Get();
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Transition.pResource = m_blasPostBuildInfoBuffer.Get();
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(2, barriers);
    }
    
    // Build BLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.DestAccelerationStructureData = m_bottomLevelAS->GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = m_blasScratchBuffer->GetGPUVirtualAddress();
    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildInfoDesc = {};
    postBuildInfoDesc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE;
    postBuildInfoDesc.DestBuffer = m_blasPostBuildInfoBuffer->GetGPUVirtualAddress();

    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 1, &postBuildInfoDesc);
    
    // Insert UAV barriers to ensure BLAS and post-build info writes complete
    {
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        
        // Barrier for BLAS
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[0].UAV.pResource = m_bottomLevelAS.Get();
        
        // Barrier for post-build info buffer
        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[1].UAV.pResource = m_blasPostBuildInfoBuffer.Get();
        
        commandList->ResourceBarrier(2, barriers);
    }
    
    // Copy post-build info to readback buffer
    {
        // transition the post-build info buffer to copy source
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_blasPostBuildInfoBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        // copy the post-build info buffer to the readback buffer
        commandList->CopyResource(m_blasPostBuildInfoReadback.Get(), m_blasPostBuildInfoBuffer.Get());
    }

    OutputDebugStringA("Bottom Level Acceleration Structure created successfully.\n");
}

void Scene::CreateTopLevelAS(ID3D12GraphicsCommandList4* commandList)
{
    // Check that we have created BLAS
    if (!m_bottomLevelAS)
    {
        OutputDebugStringA("Error: Create BLAS before building TLAS.\n");
        return;
    }
    
    // Create instance description buffer
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.InstanceID = 0;
    instanceDesc.InstanceMask = 0xFF;  // Visible to all rays
    instanceDesc.InstanceContributionToHitGroupIndex = 0;
    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDesc.AccelerationStructure = m_bottomLevelAS->GetGPUVirtualAddress();
    
    // Set identity transform
    instanceDesc.Transform[0][0] = 1.0f;
    instanceDesc.Transform[1][1] = 1.0f;
    instanceDesc.Transform[2][2] = 1.0f;
    
    // Upload instance description to GPU
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_instanceDescBuffer)));
        
        m_instanceDescBuffer->SetName(L"Instance Description Buffer");
        
        // Map and copy instance description
        void* mappedData = nullptr;
        ThrowIfFailed(m_instanceDescBuffer->Map(0, nullptr, &mappedData));
        memcpy(mappedData, &instanceDesc, sizeof(instanceDesc));
        m_instanceDescBuffer->Unmap(0, nullptr);
    }
    
    // Get required sizes for TLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = 1;  // One instance of Cornell Box
    inputs.InstanceDescs = m_instanceDescBuffer->GetGPUVirtualAddress();
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
    
    // Allocate scratch buffer
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = AlignSize(prebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_tlasScratchBuffer)));
        
        m_tlasScratchBuffer->SetName(L"TLAS Scratch Buffer");
    }
    
    // Allocate TLAS buffer
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = AlignSize(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
            nullptr,
            IID_PPV_ARGS(&m_topLevelAS)));
        
        m_topLevelAS->SetName(L"Top Level Acceleration Structure");
    }
    
    // Create post-build info buffer for TLAS (must be UAV-compatible)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_tlasPostBuildInfoBuffer)));
        
        m_tlasPostBuildInfoBuffer->SetName(L"TLAS Post-Build Info Buffer");
    }

    // Create post-build info readback buffer
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_tlasPostBuildInfoReadback)));
        
        m_tlasPostBuildInfoReadback->SetName(L"TLAS Post-Build Info Readback");
    }

    // Transition post-build info buffer to UAV state
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_tlasPostBuildInfoBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    } 

    // Transition scratch buffer to UAV state
    {
        D3D12_RESOURCE_BARRIER scratchBarrier = {};
        scratchBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        scratchBarrier.Transition.pResource = m_tlasScratchBuffer.Get();
        scratchBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        scratchBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        scratchBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &scratchBarrier);
    }
    
    // Build TLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.DestAccelerationStructureData = m_topLevelAS->GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = m_tlasScratchBuffer->GetGPUVirtualAddress();
    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildInfoDesc = {};
    postBuildInfoDesc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE;
    postBuildInfoDesc.DestBuffer = m_tlasPostBuildInfoBuffer->GetGPUVirtualAddress();
    
    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 1, &postBuildInfoDesc);
    
    // Insert UAV barriers to ensure TLAS and post-build info writes complete
    {
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        
        // Barrier for TLAS
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[0].UAV.pResource = m_topLevelAS.Get();
        
        // Barrier for post-build info buffer
        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[1].UAV.pResource = m_tlasPostBuildInfoBuffer.Get();
        
        commandList->ResourceBarrier(2, barriers);
    }
    
    // Copy post-build info to readback buffer
    {
        // transition the post-build info buffer to copy source
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_tlasPostBuildInfoBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        // copy the post-build info buffer to the readback buffer
        commandList->CopyResource(m_tlasPostBuildInfoReadback.Get(), m_tlasPostBuildInfoBuffer.Get());
    }

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
    if (!m_blasPostBuildInfoReadback || !m_tlasPostBuildInfoReadback)
    {
        OutputDebugStringA("Warning: Post-build info readback buffers not available.\n");
        return;
    }
    
    const UINT64 bufferSize = sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC);
    
    // Read BLAS post-build info
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC* pData = nullptr;
        D3D12_RANGE readRange = { 0, bufferSize };
        
        HRESULT hr = m_blasPostBuildInfoReadback->Map(0, &readRange, reinterpret_cast<void**>(&pData));
        if (SUCCEEDED(hr) && pData)
        {
            char debugMsg[256];
            sprintf_s(debugMsg, "BLAS Current Size: %llu bytes (%.2f KB)\n", 
                     pData->CurrentSizeInBytes,
                     pData->CurrentSizeInBytes / 1024.0);
            OutputDebugStringA(debugMsg);
            
            D3D12_RANGE writeRange = { 0, 0 }; // No write
            m_blasPostBuildInfoReadback->Unmap(0, &writeRange);
        }
        else
        {
            OutputDebugStringA("Warning: Failed to map BLAS post-build info readback buffer.\n");
        }
    }
    
    // Read TLAS post-build info
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC* pData = nullptr;
        D3D12_RANGE readRange = { 0, bufferSize };
        
        HRESULT hr = m_tlasPostBuildInfoReadback->Map(0, &readRange, reinterpret_cast<void**>(&pData));
        if (SUCCEEDED(hr) && pData)
        {
            char debugMsg[256];
            sprintf_s(debugMsg, "TLAS Current Size: %llu bytes (%.2f KB)\n", 
                     pData->CurrentSizeInBytes,
                     pData->CurrentSizeInBytes / 1024.0);
            OutputDebugStringA(debugMsg);
            
            D3D12_RANGE writeRange = { 0, 0 }; // No write
            m_tlasPostBuildInfoReadback->Unmap(0, &writeRange);
        }
        else
        {
            OutputDebugStringA("Warning: Failed to map TLAS post-build info readback buffer.\n");
        }
    }
}
