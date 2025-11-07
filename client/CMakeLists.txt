# 告诉CMake，我们的项目需要用到Qt6的Widgets模块
find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Network
    Multimedia #语音通话模块
)

# 开启Qt的自动化工具，它们会帮我们自动处理.ui文件和信号槽
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 定义一个名叫 "client_app" 的可执行程序
# 并告诉CMake这个程序是由下面这几个文件编译而来的
add_executable(client_app
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    chatwindow.h chatwindow.cpp chatwindow.ui
)

# 最后，把我们的程序 "client_app" 和它需要的 Qt6::Widgets 库链接起来
target_link_libraries(client_app PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)


