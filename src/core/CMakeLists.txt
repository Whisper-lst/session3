add_library(Core STATIC
    Types.h
    Logger.h
    Logger.cpp
)

target_include_directories(Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Core PUBLIC
    Qt6::Core
)
