# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "client\\CMakeFiles\\client_app_autogen.dir\\AutogenUsed.txt"
  "client\\CMakeFiles\\client_app_autogen.dir\\ParseCache.txt"
  "client\\client_app_autogen"
  "server\\CMakeFiles\\server_app_autogen.dir\\AutogenUsed.txt"
  "server\\CMakeFiles\\server_app_autogen.dir\\ParseCache.txt"
  "server\\server_app_autogen"
  )
endif()
