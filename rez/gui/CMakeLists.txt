set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(rezstudio WIN32 MACOSX_BUNDLE
  main.cpp
  mainwindow.h mainwindow.cpp
  wizard.h wizard.cpp
  textureview.h textureview.cpp
  modelview.h modelview.cpp
  stringtable.h stringtable.cpp
  util.h util.cpp
  resources.qrc
)
target_compile_definitions(rezstudio PRIVATE REZSTUDIO_VERSION="${LITHTOOLS_VERSION}")
target_link_libraries(rezstudio PRIVATE lithtech Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
set_target_properties(rezstudio PROPERTIES
  MACOSX_BUNDLE_BUNDLE_NAME "Rez Studio"
  MACOSX_BUNDLE_GUI_IDENTIFIER "org.lithtools.rezstudio")
install(TARGETS rezstudio RUNTIME DESTINATION bin BUNDLE DESTINATION .)
