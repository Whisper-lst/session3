add_library(Video STATIC
    VideoDecoder.h
    VideoDecoder.cpp
    VideoEncoder.h
    VideoEncoder.cpp
    FrameConverter.h
    FrameConverter.cpp
    MediaReader.h
    MediaReader.cpp
    MediaWriter.h
    MediaWriter.cpp
)

target_include_directories(Video PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AVCODEC_INCLUDE_DIR}
    ${AVFORMAT_INCLUDE_DIR}
    ${AVUTIL_INCLUDE_DIR}
    ${SWSCALE_INCLUDE_DIR}
    ${SWRESAMPLE_INCLUDE_DIR}
)

target_link_libraries(Video PUBLIC
    Core
    ${AVCODEC_LIBRARY}
    ${AVFORMAT_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${SWSCALE_LIBRARY}
    ${SWRESAMPLE_LIBRARY}
)
