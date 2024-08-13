#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "opencv_world" for configuration "RelWithDebInfo"
set_property(TARGET opencv_world APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(opencv_world PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/x64/vc16/lib/opencv_world3416.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/x64/vc16/bin/opencv_world3416.dll"
  )

list(APPEND _cmake_import_check_targets opencv_world )
list(APPEND _cmake_import_check_files_for_opencv_world "${_IMPORT_PREFIX}/x64/vc16/lib/opencv_world3416.lib" "${_IMPORT_PREFIX}/x64/vc16/bin/opencv_world3416.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
