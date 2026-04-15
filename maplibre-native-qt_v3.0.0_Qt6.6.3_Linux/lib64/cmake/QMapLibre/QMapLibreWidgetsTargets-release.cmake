#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "QMapLibre::Widgets" for configuration "Release"
set_property(TARGET QMapLibre::Widgets APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(QMapLibre::Widgets PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib64/libQMapLibreWidgets.so.3.0.0"
  IMPORTED_SONAME_RELEASE "libQMapLibreWidgets.so.3"
  )

list(APPEND _cmake_import_check_targets QMapLibre::Widgets )
list(APPEND _cmake_import_check_files_for_QMapLibre::Widgets "${_IMPORT_PREFIX}/lib64/libQMapLibreWidgets.so.3.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
