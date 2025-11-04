# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\bool_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\bool_autogen.dir\\ParseCache.txt"
  "bool_autogen"
  )
endif()
