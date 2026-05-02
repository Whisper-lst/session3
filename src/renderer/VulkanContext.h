#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>

#include <vulkan/vulkan.h>

#include "Types.h"
#include "Logger.h"

namespace ve {

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();
    
    bool initialize(bool enableValidation = true);
    void cleanup();
    
    bool isInitialized() const { return m_initialized; }
    
    VkInstance getInstance() const { return m_instance; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkDevice getDevice() const { return m_device; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    VkCommandPool getCommandPool() const { return m_commandPool; }
    
    uint32_t getGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
    uint32_t getPresentQueueFamilyIndex() const { return m_presentQueueFamilyIndex; }
    
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, 
                                  VkImageTiling tiling, 
                                  VkFormatFeatureFlags features) const;
    
    VkFormat findDepthFormat() const;
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    void createBuffer(VkDeviceSize size, 
                      VkBufferUsageFlags usage, 
                      VkMemoryPropertyFlags properties, 
                      VkBuffer& buffer, 
                      VkDeviceMemory& bufferMemory);
    
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    void createImage(uint32_t width, 
                     uint32_t height, 
                     VkFormat format, 
                     VkImageTiling tiling, 
                     VkImageUsageFlags usage, 
                     VkMemoryPropertyFlags properties, 
                     VkImage& image, 
                     VkDeviceMemory& imageMemory);
    
    void transitionImageLayout(VkImage image, 
                               VkFormat format, 
                               VkImageLayout oldLayout, 
                               VkImageLayout newLayout);
    
    void copyBufferToImage(VkBuffer buffer, 
                           VkImage image, 
                           uint32_t width, 
                           uint32_t height);
    
    static VulkanContext& instance();

private:
    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();
    
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    bool isDeviceSuitable(VkPhysicalDevice device);
    int rateDeviceSuitability(VkPhysicalDevice device);
    
    VkSampleCountFlagBits getMaxUsableSampleCount();

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    
    uint32_t m_graphicsQueueFamilyIndex = 0;
    uint32_t m_presentQueueFamilyIndex = 0;
    
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    
    bool m_initialized = false;
    bool m_enableValidation = false;
    
    static const std::vector<const char*> s_validationLayers;
    static const std::vector<const char*> s_deviceExtensions;
};

}
