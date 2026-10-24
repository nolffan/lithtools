add_executable(rez_selftest selftest.cpp)
target_link_libraries(rez_selftest PRIVATE lithtech)
add_test(NAME rez_selftest COMMAND rez_selftest ${CMAKE_CURRENT_BINARY_DIR}/selftest-work)
add_executable(abc_verify abc_verify.cpp)
target_link_libraries(abc_verify PRIVATE lithtech)

# DTX decoding over whole archives, and texture viewer screenshots.
add_executable(dtx_decode_all dtx_decode_all.cpp)
target_link_libraries(dtx_decode_all PRIVATE lithtech)
if(TARGET Qt6::Widgets)
  set(CMAKE_AUTOMOC ON)
  add_executable(dtx_view_shots dtx_view_shots.cpp ../gui/textureview.h ../gui/textureview.cpp)
  target_link_libraries(dtx_view_shots PRIVATE lithtech Qt6::Widgets)
endif()
if(TARGET Qt6::Widgets)
  qt_add_executable(abc_preview_shot abc_preview_shot.cpp ../gui/modelview.cpp ../gui/modelview.h)
  target_include_directories(abc_preview_shot PRIVATE ../gui)
  set_target_properties(abc_preview_shot PROPERTIES AUTOMOC ON)
  target_link_libraries(abc_preview_shot PRIVATE lithtech Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
endif()
add_executable(pe_strings_test pe_strings_test.cpp)
target_link_libraries(pe_strings_test PRIVATE lithtech)
# A screenshot of the string table editor with a DLL loaded (manual check):
#   QT_QPA_PLATFORM=offscreen pe_gui_shot <dll> <out.png>
if(TARGET Qt6::Widgets)
  add_executable(pe_gui_shot pe_gui_shot.cpp ${PROJECT_SOURCE_DIR}/gui/stringtable.cpp ${PROJECT_SOURCE_DIR}/gui/stringtable.h)
  set_target_properties(pe_gui_shot PROPERTIES AUTOMOC ON)
  target_include_directories(pe_gui_shot PRIVATE ${PROJECT_SOURCE_DIR}/gui)
  target_link_libraries(pe_gui_shot PRIVATE lithtech Qt6::Widgets)
endif()
# .ltb reading over the extracted models (out/assets/ltb), and value dumps
# to compare with plugins/blender's ltb.py.
add_executable(ltb_verify ltb_verify.cpp)
target_link_libraries(ltb_verify PRIVATE lithtech)
