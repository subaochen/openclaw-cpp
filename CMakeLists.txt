cmake_minimum_required(VERSION 3.15)
project(OpenClaw-CPP VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 项目名称和版本
set(PROJECT_NAME "OpenClaw-CPP")
set(PROJECT_VERSION "1.0.0")

# 查找源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 设置包含目录
target_include_directories(${PROJECT_NAME} 
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)

# 设置编译选项
if(MSVC)
    # Microsoft Visual C++ 特定选项
    target_compile_options(${PROJECT_NAME} PRIVATE /W3)
else()
    # GCC/Clang 特定选项
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 链接库
# 如果需要链接其他库，可以在此添加
# target_link_libraries(${PROJECT_NAME} PRIVATE ...)

# 安装目标
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装配置文件
install(FILES config.ini DESTINATION etc)

# 包含测试子目录
enable_testing()
add_subdirectory(tests)

# 创建构建系统
include(CPack)

# 生成安装脚本
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenClaw-CPP - A C++ implementation of OpenClaw")
set(CPACK_PACKAGE_VENDOR "OpenClaw Team")
set(CPACK_PACKAGE_CONTACT "support@openclaw.org")

# 设置打包格式
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)