cmake_minimum_required(VERSION 3.22)

set(PRIMROSE_LANGUAGES CXX)

option(PRIMROSE_ENABLE_GPU "Enable GPU" OFF)
message(STATUS "Primrose Enable GPU: ${PRIMROSE_ENABLE_GPU}")
if (PRIMROSE_ENABLE_GPU)
    set(CMAKE_CUDA_ARCHITECTURES "native")
    set(CMAKE_CUDA_COMPILER "/usr/local/cuda/bin/nvcc")
    list(APPEND PRIMROSE_LANGUAGES CUDA)
endif ()

project(Primrose LANGUAGES ${PRIMROSE_LANGUAGES})

include(FetchContent)

if (PRIMROSE_ENABLE_GPU)

    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)

    find_package(CUDAToolkit REQUIRED)
    include_directories(/usr/local/cuda/targets/x86_64-linux/include)
    link_directories(/usr/local/cuda/targets/x86_64-linux/lib)

endif ()

set(CMAKE_CXX_STANDARD 17)

set(PRIMROSE_SRC_FILES "")
add_subdirectory(src)

add_executable(Primrose
        ${PRIMROSE_SRC_FILES}
)

if (PRIMROSE_ENABLE_GPU)
    FetchContent_Declare(
            phantom_fhe
            GIT_REPOSITORY https://github.com/scwang98/phantom-fhe.git
            GIT_TAG main
    )
    FetchContent_MakeAvailable(phantom_fhe)
    target_link_libraries(Primrose PRIVATE Phantom CUDA::cudart)

    target_compile_definitions(Primrose PRIVATE PRIMROSE_ENABLE_GPU)
    set_target_properties(Primrose PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
else ()
    set(SEAL_USE_ZSTD OFF)
    set(SEAL_USE_ZLIB OFF)
    set(SEAL_USE_MSGSL OFF)
    set(SEAL_BUILD_EXAMPLES OFF)
    set(SEAL_USE_INTEL_HEXL OFF)
    set(SEAL_USE_ALIGNED_ALLOC OFF)
    set(SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT OFF)
    FetchContent_Declare(
            seal
            GIT_REPOSITORY https://github.com/scwang98/SEAL.git
            GIT_TAG primrose
    )
    FetchContent_MakeAvailable(seal)
    target_link_libraries(Primrose PRIVATE SEAL::seal)
endif ()

FetchContent_Declare(
        cnpy
        GIT_REPOSITORY https://github.com/rogersce/cnpy.git
        GIT_TAG master
)
FetchContent_MakeAvailable(cnpy)
target_include_directories(Primrose PRIVATE ${cnpy_SOURCE_DIR})
target_link_libraries(Primrose PRIVATE cnpy)

set(FAISS_ENABLE_GPU OFF)
set(FAISS_ENABLE_PYTHON OFF)
set(BUILD_TESTING OFF)
FetchContent_Declare(
        faiss
        GIT_REPOSITORY https://github.com/facebookresearch/faiss.git
        GIT_TAG v1.8.0
)
FetchContent_MakeAvailable(faiss)
target_link_libraries(Primrose PRIVATE faiss)
