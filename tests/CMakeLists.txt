cmake_minimum_required(VERSION 3.15)

# 查找Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 查找测试源文件
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# 创建测试可执行文件
add_executable(OpenClaw-CPP-Tests ${TEST_SOURCES})

# 链接Google Test库
target_link_libraries(OpenClaw-CPP-Tests ${GTEST_LIBRARIES} pthread)

# 添加测试
add_test(OpenClaw-CPP-Tests OpenClaw-CPP-Tests)

# 设置包含目录
target_include_directories(OpenClaw-CPP-Tests 
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)