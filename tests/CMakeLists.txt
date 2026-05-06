find_package(Qt6 REQUIRED COMPONENTS Test Widgets)

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

# tst_locationindicatormanager
add_executable(tst_locationindicatormanager tst_locationindicatormanager.cpp
    ${CMAKE_SOURCE_DIR}/locationindicatormanager.cpp)
target_include_directories(tst_locationindicatormanager PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_locationindicatormanager PRIVATE Qt6::Test Qt6::Widgets QMapLibre::Core)
add_test(NAME tst_locationindicatormanager COMMAND tst_locationindicatormanager)
set_tests_properties(tst_locationindicatormanager PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=/home/guangbin/Qt/6.6.3/gcc_64/lib:$ENV{LD_LIBRARY_PATH}"
)

# tst_cameraanimation
add_executable(tst_cameraanimation tst_cameraanimation.cpp)
target_include_directories(tst_cameraanimation PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_cameraanimation PRIVATE Qt6::Test Qt6::Core)
add_test(NAME tst_cameraanimation COMMAND tst_cameraanimation)

# tst_gui - GUI end-to-end test
add_executable(tst_gui tst_gui.cpp
    ${CMAKE_SOURCE_DIR}/mainwindow.cpp
    ${CMAKE_SOURCE_DIR}/mapcontainer.cpp
    ${CMAKE_SOURCE_DIR}/scalebarwidget.cpp
    ${CMAKE_SOURCE_DIR}/controlpanelwidget.cpp
    ${CMAKE_SOURCE_DIR}/hxgisserver.cpp
    ${CMAKE_SOURCE_DIR}/geojsonbuilder.cpp
    ${CMAKE_SOURCE_DIR}/annotationmanager.cpp
    ${CMAKE_SOURCE_DIR}/routegeojsonbuilder.cpp
    ${CMAKE_SOURCE_DIR}/routemanager.cpp
    ${CMAKE_SOURCE_DIR}/locationindicatormanager.cpp
    ${CMAKE_SOURCE_DIR}/testrunner.cpp
)
target_include_directories(tst_gui PRIVATE ${CMAKE_SOURCE_DIR} ${HXGISSERVER_INCLUDE_DIR})
target_link_directories(tst_gui PRIVATE ${HXGISSERVER_DIR})
target_link_libraries(tst_gui PRIVATE
    Qt6::Test Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets
    QMapLibre::Widgets QMapLibre::Core
    plugin-HXGISServer
)
add_test(NAME tst_gui COMMAND tst_gui)
set_tests_properties(tst_gui PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=/home/guangbin/Qt/6.6.3/gcc_64/lib:$ENV{LD_LIBRARY_PATH}"
)
