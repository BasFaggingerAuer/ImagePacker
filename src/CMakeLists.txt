link_directories(${SDL_LIBRARY_DIR} ${SDLIMAGE_LIBRARY_DIR} ${SDLTTF_LIBRARY_DIR})
include_directories(${SDL_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${SDLTTF_INCLUDE_DIR})
include_directories(${IMGPACK_SOURCE_DIR}/src/)

add_executable(imgpack imgpack.cpp)
target_link_libraries(imgpack ${SDLTTF_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDL_LIBRARY})
