add_library(Renderer STATIC
    VulkanRenderer.h
    VulkanRenderer.cpp
    VulkanContext.h
    VulkanContext.cpp
    ShaderManager.h
    ShaderManager.cpp
    FrameBuffer.h
    FrameBuffer.cpp
    RenderPass.h
    RenderPass.cpp
    Texture.h
    Texture.cpp
)

target_include_directories(Renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Renderer PUBLIC
    Core
    Vulkan::Vulkan
    Qt6::Gui
    Qt6::OpenGL
)
