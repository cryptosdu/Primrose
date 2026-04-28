
set(PRIMROSE_UTIL_FILES "")
add_subdirectory(util)

set(PRIMROSE_SRC_FILES ${PRIMROSE_SRC_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/params.h
        ${CMAKE_CURRENT_LIST_DIR}/models.h
        ${CMAKE_CURRENT_LIST_DIR}/client.h
        ${CMAKE_CURRENT_LIST_DIR}/client.cpp
        ${CMAKE_CURRENT_LIST_DIR}/server.h
        ${CMAKE_CURRENT_LIST_DIR}/server.cpp
        ${CMAKE_CURRENT_LIST_DIR}/calculator.h
        ${CMAKE_CURRENT_LIST_DIR}/calculator.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fhe_adapter.h
        ${CMAKE_CURRENT_LIST_DIR}/fhe_adapter.cu
        ${PRIMROSE_UTIL_FILES}
)

set(PRIMROSE_SRC_FILES ${PRIMROSE_SRC_FILES} PARENT_SCOPE)
