# Optional ImGuizmo Integration

This project can use **ImGuizmo + Dear ImGui** for transform gizmo interaction.

Because some environments block direct GitHub downloads, these files are treated as optional local vendor dependencies.

## Expected layout

- `third_party/ImGuizmo/ImGuizmo.h`
- `third_party/ImGuizmo/ImGuizmo.cpp`
- `third_party/imgui/imgui.h`
- `third_party/imgui/imgui.cpp`
- `third_party/imgui/imgui_draw.cpp`
- `third_party/imgui/imgui_tables.cpp`
- `third_party/imgui/imgui_widgets.cpp`
- `third_party/imgui/backends/imgui_impl_opengl3.h`
- `third_party/imgui/backends/imgui_impl_opengl3.cpp`

If these files are present, CMake enables `SIMPLE_VIEWER_HAS_IMGUIZMO=1` and the viewport uses ImGuizmo for manipulation. Otherwise, it falls back to the built-in gizmo.
