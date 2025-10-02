#include "HeapManager.h"
#include "Helper.h"
#include <algorithm>
#include <format>
#include <cassert>


class HeapAllocator
{
public:
    // Initialize the heap allocator
    HeapAllocator(uint32_t numElements, uint32_t elementSize, uint32_t alignment, const char *allocatorName = nullptr, bool outputDebugStringLogs = false)
    {
        m_numElements = numElements;
        m_elementSize = elementSize;
        m_alignment = alignment; 
        m_outputDebugStringLogs = outputDebugStringLogs;

        const uint64_t totalSize = static_cast<uint64_t>(numElements) * elementSize;
        m_alignedSize = static_cast<uint32_t>(AlignSize(totalSize, m_alignment));
    
        // Initialize memory management structures
        auto blockIt = m_blocks.emplace(m_blocks.end());
        blockIt->offset = 0;
        blockIt->size = m_alignedSize;
        blockIt->isFree = true;
        
        // Add to lookup maps
        m_blockByOffset[0] = blockIt;
        m_freeBlocksBySizeMap.emplace(m_alignedSize, blockIt);

        if (allocatorName) {
            m_allocatorName = allocatorName;
        }
        else {
            m_allocatorName = "Unnamed HeapAllocator";
        }
    }

    ~HeapAllocator()
    {
        // output warning if there are any allocations
        if (m_allocations.size() > 0)
        {
            OutputDebugStringLog("Warning - there are still allocations");
        }
        // show some detailed information about the left allocations
        for (const auto& allocation : m_allocations)
        {
            OutputDebugStringLog(std::format("Allocation - offset={}, size={}\n", allocation.first, allocation.second.size).c_str());
        }
    }

    // suballocate memory from the heap
    uint32_t Allocate(uint32_t allocationSize)
    {
        auto FindBestFitBlock = [this](uint32_t size) -> std::list<Block>::iterator
        {
            // Find the smallest free block that can fit the requested size - O(log n)
            auto it = m_freeBlocksBySizeMap.lower_bound(size);
            if (it != m_freeBlocksBySizeMap.end())
            {
                return it->second;
            }
            return m_blocks.end();
        };

        auto SplitBlock = [this](std::list<Block>::iterator blockIt, uint32_t size)
        {
            // Create new free block for the remaining space
            auto newBlockIt = m_blocks.emplace(std::next(blockIt));
            newBlockIt->offset = blockIt->offset + size;
            newBlockIt->size = blockIt->size - size;
            newBlockIt->isFree = true;
           
            // Resize current block
            blockIt->size = size;
            
            // Add new block to lookup map
            m_blockByOffset[newBlockIt->offset] = newBlockIt;
            
            // Add new block to free blocks map
            m_freeBlocksBySizeMap.emplace(newBlockIt->size, newBlockIt);
        };

        const uint32_t alignedSize = static_cast<uint32_t>(AlignSize(allocationSize, m_alignment));

        // Find best fit block
        auto blockIt = FindBestFitBlock(alignedSize);
        if (blockIt == m_blocks.end())
        {
            OutputDebugStringLog(std::format("Allocation failed - no suitable block found for {} bytes\n", alignedSize).c_str());
            return (uint32_t)-1;
        }
        
        uint32_t allocOffset = blockIt->offset;
        
        // Remove from free blocks map before modification
        RemoveFromFreeMap(blockIt);
        
        // Split block if necessary
        if (blockIt->size > alignedSize)
        {
            SplitBlock(blockIt, alignedSize);
        }
        
        // Mark block as allocated
        blockIt->isFree = false;
        
        // Add to allocations map
        AllocationInfo allocInfo;
        allocInfo.offset = allocOffset;
        allocInfo.size = alignedSize;
        allocInfo.blockIt = blockIt;
        m_allocations[allocOffset] = allocInfo;
    
        OutputDebugStringLog(std::format("Allocated {} bytes at internal offset {}\n", 
            alignedSize, allocOffset).c_str());
    
        return allocOffset;
    }
    
    // free memory from the heap
    void Free(uint32_t offset)
    {
        if (offset == (uint32_t)-1)
        {
            return;
        }
        
        // Find allocation - O(log n)
        auto allocIt = m_allocations.find(offset);
        if (allocIt == m_allocations.end())
        {
            OutputDebugStringLog(std::format("Free failed - no allocation found at offset {}\n", offset).c_str(), true);
            assert(false);
            return;
        }
        
        // Get block iterator from allocation info - O(1)
        auto blockIt = allocIt->second.blockIt;
        
        // Mark block as free
        blockIt->isFree = true;
        
        // Remove from allocations
        m_allocations.erase(allocIt);

        OutputDebugStringLog(std::format("Freed {} bytes at offset {}\n", blockIt->size, offset).c_str());

        // Coalesce only with adjacent blocks - O(1)
        CoalesceAdjacentFreeBlocks(blockIt);        
    }

    HeapAllocationStats GetStats() const
    {
        HeapAllocationStats stats = {};
        stats.totalSize = m_numElements * m_elementSize;
        stats.numAllocations = m_allocations.size();
        
        for (const auto& block : m_blocks)
        {
            if (!block.isFree)
            {
                stats.usedSize += block.size;
            }
            else
            {
                stats.numFreeBlocks++;
                stats.largestFreeBlock = std::max(stats.largestFreeBlock, static_cast<size_t>(block.size));
            }
        }
        
        // Calculate fragmentation ratio
        if (stats.totalSize > stats.usedSize && stats.largestFreeBlock > 0)
        {
            size_t freeSpace = stats.totalSize - stats.usedSize;
            stats.fragmentationRatio = 1.0f - (static_cast<float>(stats.largestFreeBlock) / static_cast<float>(freeSpace));
        }
       
        return stats;
    }

    // Element size
    uint32_t ElementSize() const { return m_elementSize; }

    // Aligned size
    uint32_t AlignedSize() const { return m_alignedSize; }

    // Allocator name
    std::string AllocatorName() const { return m_allocatorName; }

private:
    // Enhanced memory block structure with bidirectional links
    struct Block {
        uint32_t offset;
        uint32_t size;
        bool isFree;
    };

    // Enhanced allocation info structure
    struct AllocationInfo {
        uint32_t offset;
        uint32_t size;
        std::list<Block>::iterator blockIt;  // Direct iterator to the block
    };
   
    // Number of elements
    uint32_t m_numElements = {};
    
    // Element size
    uint32_t m_elementSize = {};

    // Alignment
    uint32_t m_alignment = {};

    // Aligned size
    uint32_t m_alignedSize = {};

    // Output debug string logs
    bool m_outputDebugStringLogs = false;

    // Allocator name
    std::string m_allocatorName;

    // Optimized memory management structures
    std::list<Block> m_blocks;                                          // All blocks (always sorted by offset)
    std::map<uint32_t, std::list<Block>::iterator> m_blockByOffset;    // O(1) block lookup by offset
    std::multimap<uint32_t, std::list<Block>::iterator> m_freeBlocksBySizeMap;  // Free blocks sorted by size
    std::map<uint32_t, AllocationInfo> m_allocations;                  // Active allocations by offset
   
    void OutputDebugStringLog(const char *message, bool forceOutput = false)
    {
        if (m_outputDebugStringLogs || forceOutput)
        {
            OutputDebugStringA(std::format("{}: {}\n", m_allocatorName, message).c_str());
        }
    }

    void CoalesceAdjacentFreeBlocks(std::list<Block>::iterator freedBlockIt)
    {
        // Try to merge with previous block
        if (freedBlockIt != m_blocks.begin())
        {
            auto prevIt = std::prev(freedBlockIt);
            if (prevIt->isFree && prevIt->offset + prevIt->size == freedBlockIt->offset)
            {
                // Remove prev block from free map
                RemoveFromFreeMap(prevIt);
                
                // Merge into previous block
                prevIt->size += freedBlockIt->size;

                // Remove current block from lookup map
                m_blockByOffset.erase(freedBlockIt->offset);
                
                // Erase current block
                m_blocks.erase(freedBlockIt);
                
                // Continue with the merged block
                freedBlockIt = prevIt;
            }
        }
        
        // Try to merge with next block
        auto nextIt = std::next(freedBlockIt);
        if (nextIt != m_blocks.end())
        {
            if (nextIt->isFree && freedBlockIt->offset + freedBlockIt->size == nextIt->offset)
            {
                // Remove next block from free map
                RemoveFromFreeMap(nextIt);

                // Merge into current block
                freedBlockIt->size += nextIt->size;

                // Remove next block from lookup map
                m_blockByOffset.erase(nextIt->offset);
                
                // Erase next block
                m_blocks.erase(nextIt);
            }
        }
        
        // Add the (possibly merged) block to free map
        m_freeBlocksBySizeMap.emplace(freedBlockIt->size, freedBlockIt);
    }
    
    void RemoveFromFreeMap(std::list<Block>::iterator blockIt)
    {
        // Optimized removal using hint from block size
        auto range = m_freeBlocksBySizeMap.equal_range(blockIt->size);
        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second == blockIt)
            {
                m_freeBlocksBySizeMap.erase(it);
                break;
            }
        }
    }
    
};






HeapManager::HeapManager() :
    m_device(nullptr),
    m_isGPUUploadHeapIsSupported(false),
    m_isManualWriteTrackingResourceSupported(false),
    m_type(D3D12_HEAP_TYPE_DEFAULT),
    m_heapFlags(D3D12_HEAP_FLAG_NONE),
    m_resourceFlags(D3D12_RESOURCE_FLAG_NONE),
    m_resourceState(D3D12_RESOURCE_STATE_COMMON),
    m_mappedPtr(nullptr),
    m_gpuVirtualAddress(0)
{
}

HeapManager::~HeapManager()
{
    if (m_mappedPtr && m_resource)
    {
        m_resource->Unmap(0, nullptr);
    }
    m_resource.Reset();
    m_mappedPtr = nullptr;
    m_gpuVirtualAddress = 0;
}

void HeapManager::Initialize(ID3D12Device5* device, uint32_t numElements, uint32_t elementSize, D3D12_HEAP_TYPE type, D3D12_HEAP_FLAGS flags, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_RESOURCE_STATES initialResourceState,
     const char *allocatorName, bool outputDebugStringLogs)
{
    // alignment size = element size.
    m_heapAllocator = std::make_unique<HeapAllocator>(numElements, elementSize, elementSize, allocatorName, outputDebugStringLogs);

    m_device = device;
    m_type = type;
    m_heapFlags = flags;
    m_resourceFlags = resourceFlags;
    assert(type == D3D12_HEAP_TYPE_GPU_UPLOAD || type == D3D12_HEAP_TYPE_UPLOAD || type == D3D12_HEAP_TYPE_READBACK || type == D3D12_HEAP_TYPE_DEFAULT);
    
    // Check feature support
    D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};    
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16))))
    {
        m_isGPUUploadHeapIsSupported = options16.GPUUploadHeapSupported;
        
        // Check manual write tracking support
        D3D12_FEATURE_DATA_D3D12_OPTIONS17 options17 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS17, &options17, sizeof(options17))))
        {
            m_isManualWriteTrackingResourceSupported = options17.ManualWriteTrackingResourceSupported;
        }
    }
    
    // Check If GPU upload heap is supported
    if (type == D3D12_HEAP_TYPE_GPU_UPLOAD && !m_isGPUUploadHeapIsSupported)
    {
        OutputDebugStringA("GPU Upload Heap not supported\n");
        assert(false);
    }
    
    // Create heap properties
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = type;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    // Create resource description
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = m_heapAllocator->AlignedSize();
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = m_resourceFlags;
    
    // Add manual write tracking flag if supported
    if ((type == D3D12_HEAP_TYPE_GPU_UPLOAD || type == D3D12_HEAP_TYPE_UPLOAD) && m_isManualWriteTrackingResourceSupported)
    {
        m_heapFlags = D3D12_HEAP_FLAG_TOOLS_USE_MANUAL_WRITE_TRACKING;
    }
    
    // Create the resource
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProps,
        m_heapFlags,
        &resourceDesc,
        initialResourceState,
        nullptr,
        IID_PPV_ARGS(&m_resource)));
    m_resourceState = initialResourceState;
    
    // Set name based on heap type
    if (type == D3D12_HEAP_TYPE_GPU_UPLOAD)
    {
        m_resource->SetName(L"Buffer managed by HeapManager of type GPU Upload");
    }
    else if (type == D3D12_HEAP_TYPE_UPLOAD)
    {
        m_resource->SetName(L"Buffer managed by HeapManager of type Upload");
    }
    else if (type == D3D12_HEAP_TYPE_READBACK)
    {
        m_resource->SetName(L"Buffer managed by HeapManager of type Readback");
    }
    else if (type == D3D12_HEAP_TYPE_DEFAULT)
    {
        m_resource->SetName(L"Buffer managed by HeapManager of type Default");
    }
    
    OutputDebugStringA(std::format("{}: initialized: Type={}, Size={}KB, ElementSize={}B, NumElements={}\n", 
        m_heapAllocator->AllocatorName(), static_cast<int>(type), m_heapAllocator->AlignedSize() / 1024, elementSize, numElements).c_str());

    if (type == D3D12_HEAP_TYPE_UPLOAD || type == D3D12_HEAP_TYPE_GPU_UPLOAD || type == D3D12_HEAP_TYPE_READBACK)
    {
        m_resource->Map(0, nullptr, &m_mappedPtr);
        if (! m_mappedPtr) {
            OutputDebugStringA(std::format("{}: Failed to map the resource\n", m_heapAllocator->AllocatorName()).c_str());
            assert(false);
        }
    }
    else {
        m_mappedPtr = nullptr;
    }

    m_gpuVirtualAddress = m_resource->GetGPUVirtualAddress();
    if (m_gpuVirtualAddress == 0) {
        OutputDebugStringA(std::format("{}: Failed to get the GPU virtual address of the resource\n", m_heapAllocator->AllocatorName()).c_str());
        assert(false);
    }

    if (m_heapFlags & D3D12_HEAP_FLAG_TOOLS_USE_MANUAL_WRITE_TRACKING) {
        m_resource->QueryInterface(IID_PPV_ARGS(&m_manualWriteTrackingResource));
    }
}

uint32_t HeapManager::Allocate(uint32_t allocationSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_heapAllocator->Allocate(allocationSize) + RESERVED_OFFSET;
}

void HeapManager::Free(uint32_t offset)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_heapAllocator->Free(offset - RESERVED_OFFSET);
}

void HeapManager::TrackWrite(D3D12_RANGE* pWrittenRange)
{
    if (!m_isManualWriteTrackingResourceSupported || !pWrittenRange)
    {
        return;
    }
    
    // If manual write tracking is supported, we can track the written range
    // This helps with GPU debugging tools like PIX
    if (m_manualWriteTrackingResource && m_mappedPtr) {
        m_manualWriteTrackingResource->TrackWrite(0, pWrittenRange);
    
        // In a real implementation, you would accumulate written ranges
        // and potentially call WriteToSubresource or similar
        OutputDebugStringA(std::format("TrackWrite: Range [{}, {}]\n", 
            pWrittenRange->Begin, pWrittenRange->End).c_str());
    }
}

HeapAllocationStats HeapManager::GetStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    return m_heapAllocator->GetStats();
}

void HeapManager::Transition(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES state)
{
    if (m_resourceState == state)
    {
        return;
    }
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_resource.Get();
    barrier.Transition.StateBefore = m_resourceState;
    barrier.Transition.StateAfter = state;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    m_resourceState = state;
}

void HeapManager::UAVBarrier(ID3D12GraphicsCommandList4* commandList)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_resource.Get();
    commandList->ResourceBarrier(1, &barrier);
}












ReadbackHeapManager::ReadbackHeapManager() :
    m_device(nullptr),
    m_mappedPtr(nullptr),
    m_gpuVirtualAddress(0)
{
}

ReadbackHeapManager::~ReadbackHeapManager()
{
    if (m_mappedPtr && m_readbackResource)
    {
        m_readbackResource->Unmap(0, nullptr);
    }
    m_readbackResource.Reset();
    m_resource.Reset();
    m_mappedPtr = nullptr;
    m_gpuVirtualAddress = 0;
}

void ReadbackHeapManager::Initialize(ID3D12Device5* device, uint32_t numElements, uint32_t elementSize, const char *allocatorName, bool outputDebugStringLogs)
{
    m_heapAllocator = std::make_unique<HeapAllocator>(numElements, elementSize, elementSize, allocatorName, outputDebugStringLogs);

    m_device = device;

    {        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = m_heapAllocator->AlignedSize();
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        {
            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_READBACK;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        
            // Create the readback resource for CPU read
            m_readbackResourceState = D3D12_RESOURCE_STATE_COMMON;
            ThrowIfFailed(m_device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                m_readbackResourceState,
                nullptr,
                IID_PPV_ARGS(&m_readbackResource)));
        }

        {
            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            // Create the default resource for GPU write
            m_resourceState = D3D12_RESOURCE_STATE_COMMON;
            ThrowIfFailed(m_device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                m_resourceState,
                nullptr,
                IID_PPV_ARGS(&m_resource)));
        }
    }
  
    
    OutputDebugStringA(std::format("{}: initialized: Size={}KB, ElementSize={}B, NumElements={}\n", 
        m_heapAllocator->AllocatorName(), 
        m_heapAllocator->AlignedSize() / 1024, elementSize, numElements).c_str());

    m_readbackResource->Map(0, nullptr, &m_mappedPtr);
    if (! m_mappedPtr) {
        OutputDebugStringA(std::format("{}: Failed to map the resource\n", m_heapAllocator->AllocatorName()).c_str());
        assert(false);
    }

    m_gpuVirtualAddress = m_resource->GetGPUVirtualAddress();
    if (m_gpuVirtualAddress == 0) {
        OutputDebugStringA(std::format("{}: Failed to get the GPU virtual address of the resource\n", m_heapAllocator->AllocatorName()).c_str());
        assert(false);
    }
}

uint32_t ReadbackHeapManager::Allocate(uint32_t allocationSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_heapAllocator->Allocate(allocationSize) + RESERVED_OFFSET;
}

void ReadbackHeapManager::Free(uint32_t offset)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_heapAllocator->Free(offset - RESERVED_OFFSET);
}

HeapAllocationStats ReadbackHeapManager::GetStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    return m_heapAllocator->GetStats();
}

void ReadbackHeapManager::GPUWriteBegin(ID3D12GraphicsCommandList4* commandList)
{
    // Transition default resource to UAV state
    D3D12_RESOURCE_BARRIER barriers[1] = {};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = m_resource.Get();
    barriers[0].Transition.StateBefore = m_resourceState;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(1, barriers);

    m_resourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

void ReadbackHeapManager::GPUWriteEnd(ID3D12GraphicsCommandList4* commandList)
{
    // transition the default resource to copy source
    // transition the readback resource to copy dest
    {
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Transition.pResource = m_resource.Get();
        barriers[0].Transition.StateBefore = m_resourceState;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Transition.pResource = m_readbackResource.Get();
        barriers[1].Transition.StateBefore = m_readbackResourceState;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
        commandList->ResourceBarrier(2, barriers);
        m_resourceState = D3D12_RESOURCE_STATE_COPY_SOURCE;
        m_readbackResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    // copy the default resource to the readback resource
    commandList->CopyResource(m_readbackResource.Get(), m_resource.Get());

    {
        D3D12_RESOURCE_BARRIER barriers[1] = {};
    
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Transition.pResource = m_readbackResource.Get();
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
        commandList->ResourceBarrier(1, barriers);
        m_readbackResourceState = D3D12_RESOURCE_STATE_COMMON;
    }
}



