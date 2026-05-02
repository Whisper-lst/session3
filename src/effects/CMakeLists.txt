add_library(Effects STATIC
    Effect.h
    Effect.cpp
    EffectManager.h
    EffectManager.cpp
    Transition.h
    Transition.cpp
    TransitionManager.h
    TransitionManager.cpp
    ColorCorrection.h
    ColorCorrection.cpp
    BlurEffect.h
    BlurEffect.cpp
    ChromaKey.h
    ChromaKey.cpp
)

target_include_directories(Effects PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Effects PUBLIC
    Core
    Video
    Renderer
)
