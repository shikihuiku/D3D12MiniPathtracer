#pragma once
#define NOMINMAX

#include <d3d12.h>
#include <directxmath.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <map>
#include <list>
#include <mutex>

using Microsoft::WRL::ComPtr;

// Get allocation statistics
struct HeapAllocationStats {
    size_t totalSize;
    size_t usedSize;
    size_t largestFreeBlock;
    size_t numAllocations;
    size_t numFreeBlocks;
    float fragmentationRatio;  // 0.0 = no fragmentation, 1.0 = heavily fragmented
};


class HeapAllocator;

class HeapManager
{
public:
    HeapManager();
    ~HeapManager();

    // Initialize the heap manager with device
    void Initialize(ID3D12Device5* device, uint32_t numElements, uint32_t elementSize,  D3D12_HEAP_TYPE type, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_RESOURCE_STATES initialResourceState,
         const char *allocatorName = nullptr, bool outputDebugStringLogs = false);

       // Get the heap managed by this class
    ComPtr<ID3D12Resource> Get() const { return m_resource; }

    // suballocate memory from the heap
    // This function returns an offset of the allocated memory region in the heap.
    uint32_t Allocate(uint32_t allocationSize);

    // Get the GPU virtual address of the allocated memory
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t offset) const
    {
        if (offset == 0 || offset < RESERVED_OFFSET || !m_gpuVirtualAddress) 
            return 0;
        return m_gpuVirtualAddress + static_cast<uint64_t>(offset - RESERVED_OFFSET); 
    }

    // Get the mapped pointer of the allocated memory
    void *GetMappedPtr(uint32_t offset) const
        {
        if (offset == 0 || offset < RESERVED_OFFSET || !m_mappedPtr) 
            return nullptr;
        return static_cast<uint8_t*>(m_mappedPtr) + (offset - RESERVED_OFFSET); 
    }

    void Transition(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES state);

    void UAVBarrier(ID3D12GraphicsCommandList4* commandList);

    // free memory from the heap
    // This function takes the offset of the allocated element in the heap.
    void Free(uint32_t offset);

    // This is used to determine the range of the heap to give a hint for PIX or NSight.
    void TrackWrite(D3D12_RANGE* pWrittenRange);

    // Get allocation statistics
    HeapAllocationStats GetStats() const;

private:
    // Reserved offset
    const uint32_t RESERVED_OFFSET = 1;

    // Heap allcoator
    std::unique_ptr<HeapAllocator> m_heapAllocator;

    // Thread safety
    mutable std::mutex m_mutex;

    // Device reference (not owned)
    ID3D12Device5* m_device;
    
    // Whether the GPU upload heap is supported
    bool m_isGPUUploadHeapIsSupported;

    // Whether the manual write tracking resource is supported
    bool m_isManualWriteTrackingResourceSupported;

    //  A heap managed by this class.
    ComPtr<ID3D12Resource>                      m_resource;
    ComPtr<ID3D12ManualWriteTrackingResource>   m_manualWriteTrackingResource;
    
    // Heap type
    D3D12_HEAP_TYPE m_type;
    
    // Heap flags
    D3D12_HEAP_FLAGS m_heapFlags;

    // Resource flags
    D3D12_RESOURCE_FLAGS m_resourceFlags;

    // Resource state
    D3D12_RESOURCE_STATES m_resourceState;

    void *m_mappedPtr;
    D3D12_GPU_VIRTUAL_ADDRESS m_gpuVirtualAddress; 
};


class ReadbackHeapManager
{
public:
    ReadbackHeapManager();
    ~ReadbackHeapManager();

    // Initialize the heap manager with device
    void Initialize(ID3D12Device5* device, uint32_t numElements, uint32_t elementSize, const char *allocatorName = nullptr, bool outputDebugStringLogs = false);

    // Get the heap managed by this class
    ComPtr<ID3D12Resource> GetDefaultResource() const { return m_resource; }
    ComPtr<ID3D12Resource> GetReadbackResource() const { return m_readbackResource; }

    // suballocate memory from the heap
    // This function returns an offset of the allocated memory region in the heap.
    uint32_t Allocate(uint32_t numElements);

    // Get the GPU virtual address of the allocated memory of default resource
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t offset) const
    {
        if (offset == 0 || offset < RESERVED_OFFSET || !m_gpuVirtualAddress) 
            return 0;
        return m_gpuVirtualAddress + static_cast<uint64_t>(offset - RESERVED_OFFSET); 
    }

    // Get the mapped pointer of the allocated memory of readback resource
    void *GetMappedPtr(uint32_t offset) const
    {
        if (offset == 0 || offset < RESERVED_OFFSET || !m_mappedPtr) 
            return nullptr;
        return static_cast<uint8_t*>(m_mappedPtr) + (offset - RESERVED_OFFSET); 
    }

    // free memory from the heap
    // This function takes the offset of the allocated element in the heap.
    void Free(uint32_t offset);

    HeapAllocationStats GetStats() const;

    void GPUWriteBegin(ID3D12GraphicsCommandList4* commandList);
    void GPUWriteEnd(ID3D12GraphicsCommandList4* commandList);

private:
    // Reserved offset
    const uint32_t RESERVED_OFFSET = 1;

    // Heap allcoator
    std::unique_ptr<HeapAllocator> m_heapAllocator;

    // Thread safety
    mutable std::mutex m_mutex;

    // Device reference (not owned)
    ID3D12Device5* m_device;

    //  A heap managed by this class.
    ComPtr<ID3D12Resource>                      m_resource;
    ComPtr<ID3D12Resource>                      m_readbackResource;
    D3D12_RESOURCE_STATES m_resourceState;
    D3D12_RESOURCE_STATES m_readbackResourceState;

    // mapped pointer for readback resource
    void *m_mappedPtr;

    // GPU virtual address for default resource
    D3D12_GPU_VIRTUAL_ADDRESS m_gpuVirtualAddress; 
};
