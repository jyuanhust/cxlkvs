cmake_minimum_required(VERSION 3.16)
project(cxlkvs)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(CMAKE_EXE_LINKER_FLAGS "-latomic -lisal -lmemcached -lnuma -lpthread -lstdc++")

# include_directories(${PROJECT_SOURCE_DIR}/include)


set(INCLUDE_DIRECTORIES include)


# 对src下的带有main的cpp文件执行编译
foreach(TEST_NAME main)
    add_executable(${TEST_NAME} 
        src/${TEST_NAME}.cpp
        src/bufferqueue.cpp
        src/cxlkvs.cpp
        src/meta.cpp
        src/objectindex.cpp
        src/stripe_index.cpp
        src/utils.cpp
        src/update.cpp
    )
  
  target_link_libraries(${TEST_NAME} PUBLIC stdc++ numa pthread atomic isal)
  target_include_directories(${TEST_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
endforeach()



# 对test文件夹下的cpp文件执行编译
foreach(TEST_NAME kvs_test object_index_test thread_contact)
    add_executable(${TEST_NAME} 
        test/${TEST_NAME}.cpp
        src/bufferqueue.cpp
        src/cxlkvs.cpp
        src/meta.cpp
        src/objectindex.cpp
        src/stripe_index.cpp
        src/utils.cpp
    )
  
  target_link_libraries(${TEST_NAME} PUBLIC stdc++ numa pthread atomic isal)
  target_include_directories(${TEST_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
endforeach()
