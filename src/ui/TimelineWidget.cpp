add_library(UI STATIC
    MainWindow.h
    MainWindow.cpp
    TimelineWidget.h
    TimelineWidget.cpp
    PreviewWidget.h
    PreviewWidget.cpp
    PropertyPanel.h
    PropertyPanel.cpp
    MediaLibraryWidget.h
    MediaLibraryWidget.cpp
    EffectPanel.h
    EffectPanel.cpp
    Toolbar.h
    Toolbar.cpp
)

target_include_directories(UI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(UI PUBLIC
    Core
    Video
    Timeline
    Renderer
    Effects
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::OpenGL
)
