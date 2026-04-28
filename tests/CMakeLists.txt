find_package(Qt6 REQUIRED COMPONENTS Test)

# Set RPATH for Qt libraries at test runtime
set(CMAKE_BUILD_RPATH "${CMAKE_PREFIX_PATH}/lib" "${Qt6_DIR}/../../../lib")

add_executable(tst_placeholder tst_placeholder.cpp)
target_link_libraries(tst_placeholder PRIVATE Qt6::Test)
add_test(NAME tst_placeholder COMMAND tst_placeholder)

add_executable(tst_mapannotation tst_mapannotation.cpp)
target_include_directories(tst_mapannotation PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_mapannotation PRIVATE Qt6::Test)
add_test(NAME tst_mapannotation COMMAND tst_mapannotation)

add_executable(tst_geojsonbuilder tst_geojsonbuilder.cpp ${CMAKE_SOURCE_DIR}/geojsonbuilder.cpp)
target_include_directories(tst_geojsonbuilder PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_geojsonbuilder PRIVATE Qt6::Test Qt6::Core)
add_test(NAME tst_geojsonbuilder COMMAND tst_geojsonbuilder)

add_executable(tst_annotationmanager tst_annotationmanager.cpp ${CMAKE_SOURCE_DIR}/annotationmanager.cpp ${CMAKE_SOURCE_DIR}/geojsonbuilder.cpp)
target_include_directories(tst_annotationmanager PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_annotationmanager PRIVATE Qt6::Test Qt6::Core Qt6::Gui QMapLibre::Core)
add_test(NAME tst_annotationmanager COMMAND tst_annotationmanager)
set_tests_properties(tst_annotationmanager PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=/home/guangbin/Qt/6.6.3/gcc_64/lib:$ENV{LD_LIBRARY_PATH}"
)

# tst_maproutesegment
add_executable(tst_maproutesegment tst_maproutesegment.cpp)
target_include_directories(tst_maproutesegment PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_maproutesegment PRIVATE Qt6::Test Qt6::Gui)
add_test(NAME tst_maproutesegment COMMAND tst_maproutesegment)

# tst_routegeojsonbuilder
add_executable(tst_routegeojsonbuilder tst_routegeojsonbuilder.cpp ${CMAKE_SOURCE_DIR}/routegeojsonbuilder.cpp)
target_include_directories(tst_routegeojsonbuilder PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_routegeojsonbuilder PRIVATE Qt6::Test Qt6::Core Qt6::Gui)
add_test(NAME tst_routegeojsonbuilder COMMAND tst_routegeojsonbuilder)

# tst_routemanager
add_executable(tst_routemanager tst_routemanager.cpp ${CMAKE_SOURCE_DIR}/routemanager.cpp ${CMAKE_SOURCE_DIR}/routegeojsonbuilder.cpp)
target_include_directories(tst_routemanager PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_routemanager PRIVATE Qt6::Test Qt6::Core Qt6::Gui QMapLibre::Core)
add_test(NAME tst_routemanager COMMAND tst_routemanager)
set_tests_properties(tst_routemanager PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=/home/guangbin/Qt/6.6.3/gcc_64/lib:$ENV{LD_LIBRARY_PATH}"
)
