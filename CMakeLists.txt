cmake_minimum_required(VERSION 3.16)
project(cxlkvs)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(CMAKE_EXE_LINKER_FLAGS "-latomic -lisal -lmemcached -lnuma -lpthread -lstdc++")

# include_directories(${PROJECT_SOURCE_DIR}/include)

# 添加可执行文件：列出所有 .cpp 文件（不需要 .h）

set(INCLUDE_DIRECTORIES include)

add_executable(insert
    src/bufferqueue.cpp
    src/cxlkvs.cpp
    src/insert.cpp
    src/meta.cpp
    src/objectindex.cpp
    src/stripe_index.cpp
    src/utils.cpp
)

target_include_directories(insert PRIVATE ${INCLUDE_DIRECTORIES})

target_link_libraries(insert PRIVATE stdc++ numa pthread atomic isal)

add_executable(object_index_test
    test/object_index_test.cpp
    src/objectindex.cpp
)

target_include_directories(object_index_test PRIVATE ${INCLUDE_DIRECTORIES})

target_link_libraries(object_index_test PRIVATE stdc++ numa pthread atomic isal)