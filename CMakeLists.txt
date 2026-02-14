cmake_minimum_required(VERSION 3.20)

project(DX11Study LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dx11_m0 WIN32
    src/main.cpp
    src/assets/AssetManager.cpp
    src/assets/AssetCatalog.cpp
    src/assets/GltfLoader.cpp
    src/assets/MaterialLibrary.cpp
    src/assets/AssetRegistry.cpp
    src/assets/SceneRepository.cpp
    src/ecs/EntityManager.cpp
    src/editor/ProjectBrowserWindow.cpp
    src/editor/SceneHubWindow.cpp
    src/editor/AssetPickerWindow.cpp
    src/editor/MaterialEditorWindow.cpp
    src/editor/ModelImport.cpp
    src/editor/EditorUI.cpp
    src/editor/SceneView.cpp
    src/editor/SceneCamera.cpp
    src/editor/HierarchyPanel.cpp
    src/editor/InspectorPanel.cpp
    src/editor/GizmoSystem.cpp
    src/editor/CameraGizmo.cpp
    src/camera/CameraSystem.cpp
    src/input/InputSystem.cpp
    src/core/InputController.cpp
    src/core/CameraController.cpp
    src/core/Logger.cpp
    src/core/Window.cpp
    src/renderer/Dx11Context.cpp
    src/renderer/ShaderLibrary.cpp
    src/renderer/Dx11ShaderProgram.cpp
    src/renderer/Dx11Renderer.cpp
    src/scene/Scene.cpp
    src/scene/SceneRenderer.cpp
    src/scene/systems/TransformSystem.cpp
)

target_include_directories(dx11_m0 PRIVATE
    include
    third_party/tinygltf
    third_party/stb
)

target_link_libraries(dx11_m0 PRIVATE
    comdlg32
    comctl32
    d3d11
    dxgi
    dxguid
    d3dcompiler
)

if (MSVC)
    target_compile_options(dx11_m0 PRIVATE /W4 /permissive- /Zm300)
endif()

# Copy shaders next to the executable so runtime compilation can find them.
add_custom_command(TARGET dx11_m0 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:dx11_m0>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:dx11_m0>/assets"
)
