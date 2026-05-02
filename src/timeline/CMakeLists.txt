add_library(Timeline STATIC
    Timeline.h
    Timeline.cpp
    Track.h
    Track.cpp
    Clip.h
    Clip.cpp
    TimelineCursor.h
    TimelineCursor.cpp
    Sequence.h
    Sequence.cpp
)

target_include_directories(Timeline PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Timeline PUBLIC
    Core
    Video
)
