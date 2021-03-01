set(IMGUI_SRC_DIR ${SRC_DIR}/imgui)

set(IMGUI_SOURCE
  ${IMGUI_SRC_DIR}/imgui.cpp
  ${IMGUI_SRC_DIR}/imgui_demo.cpp
  ${IMGUI_SRC_DIR}/imgui_tables.cpp
  ${IMGUI_SRC_DIR}/imgui_draw.cpp
  ${IMGUI_SRC_DIR}/imgui_widgets.cpp
)
add_library(ImGui STATIC ${IMGUI_SOURCE})
target_include_directories(ImGui PUBLIC ${SRC_DIR}/imgui)
target_compile_options(ImGui PRIVATE
  $<$<CXX_COMPILER_ID:Clang,GNU>:
    -Wno-unused-variable
  >
)

install(TARGETS ImGui RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
