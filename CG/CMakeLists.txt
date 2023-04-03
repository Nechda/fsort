cmake_minimum_required(VERSION 3.12)
project(gen-cg)

set(SOURCES
    gen-cg.cpp
    NMParser.cpp
)

set(BFD_LIBRARY /usr/local/lib/libbfd.so)

set(COMPILE_SETTINGS_DBG -g -O0 -w)
set(DBG_BINARY "${PROJECT_NAME}_dbg")


message(${BFD_LIBRARY})

add_executable(${DBG_BINARY} ${SOURCES})
target_compile_features(${DBG_BINARY} PRIVATE cxx_std_17)
target_compile_options(${DBG_BINARY} PRIVATE ${COMPILE_SETTINGS_DBG})
target_link_libraries(${DBG_BINARY} ${BFD_LIBRARY})

set(COMPILE_SETTINGS -O2 -DNDEBUG)
set(BINARY "${PROJECT_NAME}")

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_compile_options(${BINARY} PRIVATE ${COMPILE_SETTINGS})
target_link_libraries(${BINARY} ${BFD_LIBRARY})

find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/*.[ch]pp
        ${PROJECT_SOURCE_DIR}/*.[ch]
    )

    add_custom_target(
        clang-force-format
        COMMAND clang-format
        -i
        -style=file
        ${ALL_CXX_SOURCE_FILES}
    )
endif()