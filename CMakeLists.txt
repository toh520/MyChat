# 声明CMake最低版本要求和项目名称
cmake_minimum_required(VERSION 3.16)
project(Mychat LANGUAGES CXX)

# 添加子目录，CMake会自动去这些目录里寻找它们各自的CMakeLists.txt文件
add_subdirectory(client)
add_subdirectory(server)
