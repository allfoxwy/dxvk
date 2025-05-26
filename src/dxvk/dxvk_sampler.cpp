#include "dxvk_sampler.h"
#include "dxvk_device.h"

namespace dxvk {
    
  DxvkSampler::DxvkSampler(
          DxvkSamplerPool*        pool,
    const DxvkSamplerKey&         key,
          uint16_t                index)
  : m_pool(pool), m_key(key), m_index(index) {
    auto vk = m_pool->m_device->vkd();

    VkSamplerCustomBorderColorCreateInfoEXT borderColorInfo = { VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT };
    borderColorInfo.customBorderColor   = key.borderColor;

    VkSamplerReductionModeCreateInfo reductionInfo = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO };
    reductionInfo.reductionMode         = VkSamplerReductionMode(key.u.p.reduction);

    VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VkFilter(key.u.p.magFilter);
    samplerInfo.minFilter = VkFilter(key.u.p.minFilter);
    samplerInfo.mipmapMode = VkSamplerMipmapMode(key.u.p.mipMode);
    samplerInfo.addressModeU = VkSamplerAddressMode(key.u.p.addressU);
    samplerInfo.addressModeV = VkSamplerAddressMode(key.u.p.addressV);
    samplerInfo.addressModeW = VkSamplerAddressMode(key.u.p.addressW);
    samplerInfo.mipLodBias = bit::decodeFixed<int32_t, 6, 8>(key.u.p.lodBias);
    samplerInfo.anisotropyEnable = key.u.p.anisotropy > 0u;
    samplerInfo.maxAnisotropy = float(key.u.p.anisotropy);
    samplerInfo.compareEnable = key.u.p.compareEnable != 0u;
    samplerInfo.compareOp = VkCompareOp(key.u.p.compareOp);
    samplerInfo.minLod = bit::decodeFixed<uint32_t, 4, 8>(key.u.p.minLod);
    samplerInfo.maxLod = bit::decodeFixed<uint32_t, 4, 8>(key.u.p.maxLod);
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = key.u.p.pixelCoord;

    if (key.u.p.legacyCube && m_pool->m_device->features().extNonSeamlessCubeMap.nonSeamlessCubeMap)
      samplerInfo.flags |= VK_SAMPLER_CREATE_NON_SEAMLESS_CUBE_MAP_BIT_EXT;

    if (!m_pool->m_device->features().core.features.samplerAnisotropy)
      samplerInfo.anisotropyEnable = VK_FALSE;

    if (key.u.p.hasBorder)
      samplerInfo.borderColor = determineBorderColorType();

    if (samplerInfo.borderColor == VK_BORDER_COLOR_FLOAT_CUSTOM_EXT
     || samplerInfo.borderColor == VK_BORDER_COLOR_INT_CUSTOM_EXT)
      borderColorInfo.pNext = std::exchange(samplerInfo.pNext, &borderColorInfo);

    if (reductionInfo.reductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE)
      reductionInfo.pNext = std::exchange(samplerInfo.pNext, &reductionInfo);

    if (vk->vkCreateSampler(vk->device(),
        &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
      throw DxvkError("DxvkSampler::DxvkSampler: Failed to create sampler");

    m_pool->m_descriptorPool.writeDescriptor(m_index, m_sampler);
  }


  DxvkSampler::~DxvkSampler() {
    auto vk = m_pool->m_device->vkd();

    vk->vkDestroySampler(vk->device(), m_sampler, nullptr);
  }


  void DxvkSampler::release() {
    m_pool->releaseSampler(this);
  }


  VkBorderColor DxvkSampler::determineBorderColorType() const {
    static const std::array<std::pair<VkClearColorValue, VkBorderColor>, 4> s_borderColors = {{
      { { { 0.0f, 0.0f, 0.0f, 0.0f } }, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK },
      { { { 0.0f, 0.0f, 0.0f, 1.0f } }, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK },
      { { { 1.0f, 1.0f, 1.0f, 1.0f } }, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE },
    }};

    // Iterate over border colors and try to find an exact match
    uint32_t componentCount = m_key.u.p.compareEnable ? 1u : 4u;

    for (const auto& e : s_borderColors) {
      bool allEqual = true;

      for (uint32_t i = 0; i < componentCount; i++)
        allEqual &= m_key.borderColor.float32[i] == e.first.float32[i];

      if (allEqual)
        return e.second;
    }

    // If custom border colors are supported, use that
    if (m_pool->m_device->features().extCustomBorderColor.customBorderColorWithoutFormat)
      return VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;

    // Otherwise, use the sum of absolute differences to find the
    // closest fallback value. Some D3D9 games may rely on this.
    Logger::warn("DXVK: Custom border colors not supported");
 
    VkBorderColor result = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

    float minSad = -1.0f;

    for (const auto& e : s_borderColors) {
      float sad = 0.0f;

      for (uint32_t i = 0; i < componentCount; i++)
        sad += std::abs(m_key.borderColor.float32[i] - e.first.float32[i]);

      if (sad < minSad || minSad < 0.0f) {
        minSad = sad;
        result = e.second;
      }
    }

    return result;
  }




  DxvkSamplerDescriptorPool::DxvkSamplerDescriptorPool(
          DxvkDevice*               device,
          uint32_t                  size)
  : m_device(device) {
    auto vk = m_device->vkd();

    VkDescriptorPoolSize poolSize = { };
    poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
    poolSize.descriptorCount = size;

    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets = 1u;
    poolInfo.poolSizeCount = 1u;
    poolInfo.pPoolSizes = &poolSize;

    VkResult vr = vk->vkCreateDescriptorPool(vk->device(), &poolInfo, nullptr, &m_pool);

    if (vr)
      throw DxvkError(str::format("Failed to create sampler pool: ", vr));

    VkDescriptorSetLayoutBinding binding = { };
    binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    binding.descriptorCount = size;
    binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorBindingFlags bindingFlags =
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo layoutFlags = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
    layoutFlags.bindingCount = 1u;
    layoutFlags.pBindingFlags = &bindingFlags;

    VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, &layoutFlags };
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.bindingCount = 1u;
    layoutInfo.pBindings = &binding;

    if ((vr = vk->vkCreateDescriptorSetLayout(vk->device(), &layoutInfo, nullptr, &m_setLayout)))
      throw DxvkError(str::format("Failed to create sampler descriptor set layout: ", vr));

    VkDescriptorSetAllocateInfo setInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    setInfo.descriptorPool = m_pool;
    setInfo.descriptorSetCount = 1u;
    setInfo.pSetLayouts = &m_setLayout;

    if ((vr = vk->vkAllocateDescriptorSets(vk->device(), &setInfo, &m_set)))
      throw DxvkError(str::format("Failed to allocate sampler descriptor set: ", vr));
  }


  DxvkSamplerDescriptorPool::~DxvkSamplerDescriptorPool() {
    auto vk = m_device->vkd();

    vk->vkDestroyDescriptorPool(vk->device(), m_pool, nullptr);
    vk->vkDestroyDescriptorSetLayout(vk->device(), m_setLayout, nullptr);
  }


  void DxvkSamplerDescriptorPool::writeDescriptor(
          uint16_t              index,
          VkSampler             sampler) {
    auto vk = m_device->vkd();

    VkDescriptorImageInfo info = { };
    info.sampler = sampler;

    VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = m_set;
    write.dstArrayElement = index;
    write.descriptorCount = 1u;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.pImageInfo = &info;

    vk->vkUpdateDescriptorSets(vk->device(), 1u, &write, 0u, nullptr);
  }




  DxvkSamplerPool::DxvkSamplerPool(DxvkDevice* device)
  : m_device(device), m_descriptorPool(device, MaxSamplerCount) {
    // Populate free list in reverse order. Sampler index 0 is
    // reserved for the default sampler, so skip that.
    for (uint16_t i = MaxSamplerCount; i; i--)
      m_freeList.push_back(i);

    // Default sampler, implicitly used for null descriptors or when creating
    // additional samplers fails for any reason. Keep a persistent reference
    // so that this sampler does not accidentally get recycled.
    DxvkSamplerKey defaultKey;
    defaultKey.setFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
    defaultKey.setLodRange(-256.0f, 256.0f, 0.0f);
    defaultKey.setAddressModes(
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    defaultKey.setReduction(VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE);

    m_default = &m_samplers.emplace(std::piecewise_construct,
      std::forward_as_tuple(defaultKey),
      std::forward_as_tuple(this, defaultKey, 0u)).first->second;
  }


  DxvkSamplerPool::~DxvkSamplerPool() {
    m_default = nullptr;
    m_samplers.clear();
  }


  Rc<DxvkSampler> DxvkSamplerPool::createSampler(const DxvkSamplerKey& key) {
    std::unique_lock lock(m_mutex);
    auto entry = m_samplers.find(key);

    if (entry != m_samplers.end()) {
      DxvkSampler* sampler = &entry->second;

      // Remove the sampler from the LRU list if it's in there. Due
      // to the way releasing samplers is implemented upon reaching
      // a ref count of 0, it is possible that we reach this before
      // the releasing thread inserted the list into the LRU list.
      if (!sampler->m_refCount.fetch_add(1u, std::memory_order_acquire)) {
        if (sampler->m_lruPrev)
          sampler->m_lruPrev->m_lruNext = sampler->m_lruNext;
        else if (m_lruHead == sampler)
          m_lruHead = sampler->m_lruNext;

        if (sampler->m_lruNext)
          sampler->m_lruNext->m_lruPrev = sampler->m_lruPrev;
        else if (m_lruTail == sampler)
          m_lruTail = sampler->m_lruPrev;

        sampler->m_lruPrev = nullptr;
        sampler->m_lruNext = nullptr;

        m_samplersLive.store(m_samplersLive.load() + 1u);
      }

      // We already took a reference, forward the pointer as-is
      return Rc<DxvkSampler>::unsafeCreate(sampler);
    }

    // If we're spamming sampler allocations, we might need
    // to clean up unused ones here to stay within the limit
    uint16_t samplerIndex = allocateSamplerIndex();

    if (samplerIndex) {
      DxvkSampler* sampler = &m_samplers.emplace(std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(this, key, samplerIndex)).first->second;

      m_samplersTotal.store(m_samplers.size());
      m_samplersLive.store(m_samplersLive.load() + 1u);
      return sampler;
    } else {
      Logger::err("Failed to allocate sampler, using default one.");
      return m_default;
    }
  }


  void DxvkSamplerPool::releaseSampler(DxvkSampler* sampler) {
    std::unique_lock lock(m_mutex);

    // Back off if another thread has re-aquired the sampler. This is
    // safe since the ref count can only be incremented from zero when
    // the pool is locked.
    if (sampler->m_refCount.load())
      return;

    // It is also possible that two threads end up here while the ref
    // count is zero. Make sure to not add the sampler to the LRU list
    // more than once in that case.
    if (sampler->m_lruPrev || m_lruHead == sampler)
      return;

    // Add sampler to the end of the LRU list
    sampler->m_lruPrev = m_lruTail;
    sampler->m_lruNext = nullptr;

    if (m_lruTail)
      m_lruTail->m_lruNext = sampler;
    else
      m_lruHead = sampler;

    m_lruTail = sampler;

    // Don't need an atomic add for these
    m_samplersLive.store(m_samplersLive.load() - 1u);

    // Try to keep some samplers available for subsequent allocations
    if (m_samplers.size() > MinSamplerCount)
      destroyLeastRecentlyUsedSampler();
  }


  void DxvkSamplerPool::destroyLeastRecentlyUsedSampler() {
    DxvkSampler* sampler = m_lruHead;

    if (sampler) {
      freeSamplerIndex(sampler->m_index);
      m_lruHead = sampler->m_lruNext;

      if (m_lruHead)
        m_lruHead->m_lruPrev = nullptr;
      else
        m_lruTail = nullptr;

      m_samplers.erase(sampler->key());
      m_samplersTotal.store(m_samplers.size());
    }
  }


  uint16_t DxvkSamplerPool::allocateSamplerIndex() {
    if (m_freeList.empty()) {
      destroyLeastRecentlyUsedSampler();

      if (m_freeList.empty())
        return 0u;
    }

    uint16_t index = m_freeList.back();
    m_freeList.pop_back();
    return index;
  }


  void DxvkSamplerPool::freeSamplerIndex(uint16_t index) {
    m_freeList.push_back(index);
  }

}
