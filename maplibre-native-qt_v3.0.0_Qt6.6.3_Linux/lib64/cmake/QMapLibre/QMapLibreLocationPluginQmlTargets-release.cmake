#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "QMapLibre::PluginQml" for configuration "Release"
set_property(TARGET QMapLibre::PluginQml APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(QMapLibre::PluginQml PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_RELEASE ""
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/qml/MapLibre/libdeclarative_locationplugin_maplibre.so"
  IMPORTED_NO_SONAME_RELEASE "TRUE"
  )

list(APPEND _cmake_import_check_targets QMapLibre::PluginQml )
list(APPEND _cmake_import_check_files_for_QMapLibre::PluginQml "${_IMPORT_PREFIX}/qml/MapLibre/libdeclarative_locationplugin_maplibre.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
