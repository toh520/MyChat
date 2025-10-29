# 服务器没有界面，所以它只需要Qt最核心的Core模块
find_package(Qt6 REQUIRED COMPONENTS Core
    Network
)

# 开启Qt的元对象自动编译功能！
set(CMAKE_AUTOMOC ON)

# 定义一个名叫 "server_app" 的可执行程序
# 它只由一个 main.cpp 文件构成
add_executable(server_app
    main.cpp
    chatserver.h
    chatserver.cpp
)

# 将我们的程序 "server_app" 和它需要的 Qt6::Core 库链接起来
target_link_libraries(server_app PRIVATE
    Qt6::Core
    Qt6::Network
)
