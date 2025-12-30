# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "BlackHole_autogen"
  "CMakeFiles\\BlackHole_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\BlackHole_autogen.dir\\ParseCache.txt"
  )
endif()
