#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "QMapLibre::Core" for configuration "RelWithDebInfo"
set_property(TARGET QMapLibre::Core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(QMapLibre::Core PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libQMapLibre_x86_64.so"
  IMPORTED_SONAME_RELWITHDEBINFO "libQMapLibre_x86_64.so"
  )

list(APPEND _cmake_import_check_targets QMapLibre::Core )
list(APPEND _cmake_import_check_files_for_QMapLibre::Core "${_IMPORT_PREFIX}/lib/libQMapLibre_x86_64.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
