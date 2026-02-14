#include <windows.h>
#include <commctrl.h>

#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "assets/AssetManager.h"
#include "assets/AssetRegistry.h"
#include "assets/MaterialLibrary.h"
#include "assets/SceneRepository.h"
#include "camera/CameraComponent.h"
#include "camera/CameraSystem.h"
#include "core/Window.h"
#include "editor/ProjectBrowserWindow.h"
#include "editor/EditorUI.h"
#include "editor/CameraGizmo.h"
#include "input/InputSystem.h"
#include "renderer/Dx11Context.h"
#include "renderer/Dx11Renderer.h"
#include "scene/Scene.h"
#include "scene/SceneRenderer.h"
#include "scene/components/MeshRendererComponent.h"
#include "scene/components/NameComponent.h"
#include "scene/components/TransformComponent.h"

namespace
{
    constexpr int kCatalogTreeId = 9101;
    constexpr int kSceneTreeId = 9102;
    constexpr int kBtnDeleteEntityId = 9201;
    constexpr int kPanelHeight = 260;

    constexpr uint64_t kPayloadTypeNone = 0;
    constexpr uint64_t kPayloadTypeAsset = 1;
    constexpr uint64_t kPayloadTypeEntity = 2;

    uint64_t EncodePayload(uint64_t type, uint32_t value)
    {
        return (type << 32ull) | static_cast<uint64_t>(value);
    }

    uint64_t PayloadType(LPARAM payload)
    {
        return (static_cast<uint64_t>(payload) >> 32ull) & 0xFFFFFFFFull;
    }

    uint32_t PayloadValue(LPARAM payload)
    {
        return static_cast<uint32_t>(static_cast<uint64_t>(payload) & 0xFFFFFFFFull);
    }

    std::wstring ToWide(const std::string& text)
    {
        if (text.empty())
        {
            return L"";
        }
        const int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        std::wstring out(static_cast<size_t>(count), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), count);
        if (!out.empty() && out.back() == L'\0')
        {
            out.pop_back();
        }
        return out;
    }
}

class App final : public core::WindowEvents
{
public:
    bool Initialize(HINSTANCE instance)
    {
        instance_ = instance;
        if (!SelectProjectAndLoadScene())
        {
            return false;
        }

        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_TREEVIEW_CLASSES | ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icc);

        core::WindowConfig config{};
        config.title = L"DX11 ECS Engine";
        config.width = width_;
        config.height = height_;

        if (!window_.Create(instance, config, this))
        {
            MessageBoxA(nullptr, "Failed to create Win32 window.", "DX11 Error", MB_ICONERROR | MB_OK);
            return false;
        }

        window_.Show(SW_SHOW);
        width_ = window_.GetWidth();
        height_ = window_.GetHeight();

        CreateBackButton();
        CreateEditorPanels();

        if (!dxContext_.Initialize(window_.GetHandle(), width_, height_))
        {
            return false;
        }
        if (!renderer_.Initialize(dxContext_, assetManager_, materialLibrary_))
        {
            return false;
        }
        cameraSystem_.Update(cameraComponent_, camera::CameraInputState{}, 16.0f / 9.0f, 1.0f / 60.0f);
        renderer_.SetCameraMatrices(cameraComponent_.viewMatrix, cameraComponent_.projectionMatrix);
        editorUi_.Initialize(window_, dxContext_);

        LoadCatalogTree();
        RefreshSceneTree();
        ApplyViewportLayout();
        return true;
    }

    int Run()
    {
        lastTickMs_ = GetTickCount64();
        while (window_.PumpMessages())
        {
            Render();
        }
        editorUi_.Shutdown();
        return 0;
    }

    void OnResize(uint32_t width, uint32_t height) override
    {
        width_ = width;
        height_ = height;
        RepositionBackButton();
        RepositionEditorPanels();
        dxContext_.Resize(width_, height_);
        ApplyViewportLayout();
    }

    void OnCloseRequested() override {}

    void OnMouseMove(int32_t x, int32_t y) override
    {
        const uint32_t viewportHeight = GetViewportHeight();
        cursorInsideViewport_ = (x >= 0 && y >= 0 && static_cast<uint32_t>(x) < width_ && static_cast<uint32_t>(y) < viewportHeight);
        if (!cursorInsideViewport_ || width_ == 0 || viewportHeight == 0)
        {
            return;
        }

        mouseNdcX_ = (static_cast<float>(x) / static_cast<float>(width_)) * 2.0f - 1.0f;
        mouseNdcY_ = 1.0f - (static_cast<float>(y) / static_cast<float>(viewportHeight)) * 2.0f;
        inputSystem_.SetMouseNdc(mouseNdcX_, mouseNdcY_);

        if (isDraggingEntity_ && selectedEntity_ != ecs::kInvalidEntity)
        {
            auto* t = scene_.Components().Get<scene::TransformComponent>(selectedEntity_);
            if (t)
            {
                const float dx = (mouseNdcX_ - dragStartMouseNdcX_);
                const float dy = (mouseNdcY_ - dragStartMouseNdcY_);

                // Mouse drag modes (without keyboard modifiers):
                // - LMB drag: X/Y plane
                // - Hold RMB while dragging: depth on Z axis
                if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
                {
                    t->local.position.z = dragStartPosition_.z + dy;
                }
                else
                {
                    t->local.position.x = dragStartPosition_.x + dx;
                    t->local.position.y = dragStartPosition_.y + dy;
                }
            }
        }
    }

    void OnMouseButton(bool leftDown) override
    {
        inputSystem_.SetLeftMouse(leftDown);
        if (!cursorInsideViewport_)
        {
            return;
        }

        if (leftDown)
        {
            if (selectedEntity_ != ecs::kInvalidEntity)
            {
                auto* t = scene_.Components().Get<scene::TransformComponent>(selectedEntity_);
                if (t)
                {
                    isDraggingEntity_ = true;
                    dragStartMouseNdcX_ = mouseNdcX_;
                    dragStartMouseNdcY_ = mouseNdcY_;
                    dragStartPosition_ = t->local.position;
                }
            }
            else if (!placementAsset_.empty())
            {
                math::Transform root{};
                root.position = math::Vec3{mouseNdcX_, mouseNdcY_, 0.0f};
                InstantiateAssetHierarchy(placementAsset_, root);
                SaveSceneFromEcs();
                RefreshSceneTree();
            }
        }
        else if (isDraggingEntity_)
        {
            isDraggingEntity_ = false;
            SaveSceneFromEcs();
            RefreshSceneTree();
        }
    }

    void OnMouseWheel(float delta) override
    {
        inputSystem_.AddWheelDelta(delta);
    }

    void OnBackToStartRequested() override
    {
        if (!SelectProjectAndLoadScene())
        {
            return;
        }
        renderer_.Initialize(dxContext_, assetManager_, materialLibrary_);
        LoadCatalogTree();
        RefreshSceneTree();
        ApplyViewportLayout();
    }

    void OnCommand(int id, int code) override
    {
        if (code != BN_CLICKED)
        {
            return;
        }

        if (id == kBtnDeleteEntityId)
        {
            DeleteSelectedEntity();
            return;
        }
    }

    void OnNotify(const NMHDR* hdr) override
    {
        if (!hdr)
        {
            return;
        }

        LPARAM payload = 0;
        if (hdr->code == TVN_SELCHANGEDW)
        {
            const auto* tvw = reinterpret_cast<const NMTREEVIEWW*>(hdr);
            payload = tvw ? tvw->itemNew.lParam : 0;
        }
        else if (hdr->code == TVN_SELCHANGEDA)
        {
            const auto* tva = reinterpret_cast<const NMTREEVIEWA*>(hdr);
            payload = tva ? tva->itemNew.lParam : 0;
        }
        else
        {
            return;
        }

        if (hdr->idFrom == kCatalogTreeId)
        {
            const uint64_t type = PayloadType(payload);
            if (type == kPayloadTypeAsset)
            {
                const uint32_t index = PayloadValue(payload);
                if (index < assetFileNames_.size())
                {
                    placementAsset_ = assetFileNames_[index];
                }
            }
            return;
        }

        if (hdr->idFrom == kSceneTreeId)
        {
            const uint64_t type = PayloadType(payload);
            if (type == kPayloadTypeEntity)
            {
                selectedEntity_ = static_cast<ecs::Entity>(PayloadValue(payload));
            }
            else
            {
                selectedEntity_ = ecs::kInvalidEntity;
            }
        }
    }

private:
    void NormalizeLegacySceneObjects(std::vector<assets::SceneObjectSpec>& objects) const
    {
        // Legacy scenes may contain one entry per mesh (meshIndex>=0) for the same asset.
        // New format expects one entity per asset (meshIndex=-1, primitiveIndex=-1).
        std::unordered_map<std::string, assets::SceneObjectSpec> merged{};
        merged.reserve(objects.size());

        for (const auto& obj : objects)
        {
            if (obj.assetPath.empty())
            {
                continue;
            }

            auto it = merged.find(obj.assetPath);
            if (it == merged.end())
            {
                assets::SceneObjectSpec root = obj;
                root.meshIndex = -1;
                root.primitiveIndex = -1;
                merged.emplace(obj.assetPath, std::move(root));
                continue;
            }

            // Keep transform from a full-asset entry when present.
            if (obj.meshIndex < 0 && obj.primitiveIndex < 0)
            {
                it->second.transform = obj.transform;
            }
        }

        objects.clear();
        objects.reserve(merged.size());
        for (auto& [_, spec] : merged)
        {
            objects.push_back(std::move(spec));
        }
    }

    bool SelectProjectAndLoadScene()
    {
        if (!editor::ProjectBrowserWindow::ShowModal(instance_, "projects", activeProjectPath_))
        {
            return false;
        }

        std::string sceneErr;
        if (!assets::SceneRepository::EnsureDefaultScene(activeProjectPath_, activeScenePath_, &sceneErr))
        {
            MessageBoxA(nullptr, sceneErr.c_str(), "Scene Error", MB_ICONERROR | MB_OK);
            return false;
        }

        std::string materialWarn;
        if (!materialLibrary_.LoadFromFile("assets/materials/materials.json", &materialWarn))
        {
            MessageBoxA(nullptr, "Failed to load material library.", "Material Error", MB_ICONERROR | MB_OK);
            return false;
        }

        std::vector<assets::SceneObjectSpec> objects;
        std::string loadErr;
        if (!assets::SceneRepository::LoadScene(activeScenePath_, objects, &loadErr))
        {
            MessageBoxA(nullptr, loadErr.c_str(), "Scene Load Error", MB_ICONERROR | MB_OK);
            return false;
        }
        NormalizeLegacySceneObjects(objects);

        scene_.Clear();
        for (const auto& obj : objects)
        {
            const ecs::Entity e = scene_.CreateEntity();
            scene_.Components().Add<scene::MeshRendererComponent>(
                e,
                scene::MeshRendererComponent{obj.assetPath, obj.meshIndex, obj.primitiveIndex, true});
            const std::string loadedName = std::filesystem::path(obj.assetPath).stem().string();
            scene_.Components().Add<scene::NameComponent>(e, scene::NameComponent{loadedName.empty() ? "Entity" : loadedName});
            scene::TransformComponent tr{};
            tr.local = obj.transform;
            scene_.Components().Add<scene::TransformComponent>(e, tr);
        }
        SaveSceneFromEcs();

        if (scene_.BuildRenderList().empty())
        {
            const ecs::Entity e = scene_.CreateEntity();
            scene_.Components().Add<scene::MeshRendererComponent>(e, scene::MeshRendererComponent{"triangle.gltf", -1, -1, true});
            scene_.Components().Add<scene::NameComponent>(e, scene::NameComponent{"triangle"});
            scene_.Components().Add<scene::TransformComponent>(e, scene::TransformComponent{});
            SaveSceneFromEcs();
        }
        selectedEntity_ = ecs::kInvalidEntity;
        return true;
    }

    void InstantiateAssetHierarchy(const std::string& assetFileName, const math::Transform& rootTransform)
    {
        // One asset import => one entity in scene.
        // Renderer uses meshIndex=-1 to draw all meshes from the glTF.
        const assets::AssetRenderConfig cfg = materialLibrary_.ResolveRenderConfigForAsset(assetFileName);
        math::Transform finalTransform = rootTransform;
        if (!cfg.objects.empty())
        {
            // Keep clicked placement for X/Y, but inherit authored Z/rotation/scale from material config.
            finalTransform.position.z = cfg.objects[0].transform.position.z;
            finalTransform.rotationRadians = cfg.objects[0].transform.rotationRadians;
            finalTransform.scale = cfg.objects[0].transform.scale;
        }

        const ecs::Entity rootEntity = scene_.CreateEntity();
        scene::TransformComponent rootTc{};
        rootTc.local = finalTransform;
        scene_.Components().Add<scene::TransformComponent>(rootEntity, rootTc);
        scene_.Components().Add<scene::NameComponent>(
            rootEntity,
            scene::NameComponent{std::filesystem::path(assetFileName).stem().string()});
        scene_.Components().Add<scene::MeshRendererComponent>(
            rootEntity,
            scene::MeshRendererComponent{assetFileName, -1, -1, true});
    }

    void SaveSceneFromEcs()
    {
        std::vector<assets::SceneObjectSpec> out;
        const auto renderList = scene_.BuildRenderList();
        out.reserve(renderList.size());
        for (const auto& item : renderList)
        {
            assets::SceneObjectSpec s{};
            s.assetPath = item.assetPath;
            s.meshIndex = item.meshIndex;
            s.primitiveIndex = item.primitiveIndex;
            if (const auto* t = scene_.Components().Get<scene::TransformComponent>(item.entity))
            {
                s.transform = t->local;
            }
            out.push_back(std::move(s));
        }
        std::string err;
        assets::SceneRepository::SaveScene(activeScenePath_, out, &err);
    }

    void DeleteSelectedEntity()
    {
        if (selectedEntity_ == ecs::kInvalidEntity)
        {
            return;
        }
        scene_.DestroyEntity(selectedEntity_);
        selectedEntity_ = ecs::kInvalidEntity;
        SaveSceneFromEcs();
        RefreshSceneTree();
    }

    void CreateBackButton()
    {
        backButton_ = CreateWindowExW(
            0, L"BUTTON", L"Trocar Projeto", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 140, 30, window_.GetHandle(),
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(core::Window::kBackToStartCommandId)),
            instance_, nullptr);
        RepositionBackButton();
    }

    void CreateEditorPanels()
    {
        catalogLabel_ = CreateWindowExW(
            0, L"STATIC", L"Catalog", WS_CHILD | WS_VISIBLE,
            14, 0, 300, 20, window_.GetHandle(), nullptr, instance_, nullptr);
        catalogTree_ = CreateWindowExW(
            WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
            WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | WS_BORDER,
            14, 0, 380, 190, window_.GetHandle(),
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCatalogTreeId)), instance_, nullptr);

        sceneLabel_ = CreateWindowExW(
            0, L"STATIC", L"Scene", WS_CHILD | WS_VISIBLE,
            408, 0, 300, 20, window_.GetHandle(), nullptr, instance_, nullptr);
        sceneTree_ = CreateWindowExW(
            WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
            WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | WS_BORDER,
            408, 0, 520, 190, window_.GetHandle(),
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSceneTreeId)), instance_, nullptr);

        btnDelete_ = CreateWindowExW(0, L"BUTTON", L"Delete", WS_CHILD | WS_VISIBLE, 0, 0, 80, 26,
            window_.GetHandle(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBtnDeleteEntityId)), instance_, nullptr);

        // Force readable text rendering in all child controls.
        const HFONT uiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        SendMessageW(catalogLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(sceneLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(catalogTree_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(sceneTree_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(btnDelete_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);

        TreeView_SetBkColor(catalogTree_, RGB(255, 255, 255));
        TreeView_SetTextColor(catalogTree_, RGB(0, 0, 0));
        TreeView_SetBkColor(sceneTree_, RGB(255, 255, 255));
        TreeView_SetTextColor(sceneTree_, RGB(0, 0, 0));

        RepositionEditorPanels();
    }

    HTREEITEM InsertTreeItem(HWND tree, HTREEITEM parent, const std::wstring& text, LPARAM payload, std::deque<std::wstring>& textPool)
    {
        textPool.push_back(text);
        TVINSERTSTRUCTW item{};
        item.hParent = parent;
        item.hInsertAfter = TVI_LAST;
        item.item.mask = TVIF_TEXT | TVIF_PARAM;
        item.item.pszText = const_cast<wchar_t*>(textPool.back().c_str());
        item.item.lParam = payload;
        return reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item)));
    }

    void RepositionBackButton()
    {
        if (backButton_)
        {
            MoveWindow(backButton_, static_cast<int>(width_) - 154, 14, 140, 30, TRUE);
        }
    }

    void RepositionEditorPanels()
    {
        const int panelTop = static_cast<int>(GetViewportHeight()) + 8;
        const int treeHeight = static_cast<int>(kPanelHeight) - 72;
        const int catalogWidth = static_cast<int>(width_ * 0.4f) - 20;
        const int sceneWidth = static_cast<int>(width_) - catalogWidth - 42;

        MoveWindow(catalogLabel_, 14, panelTop, 240, 18, TRUE);
        MoveWindow(catalogTree_, 14, panelTop + 18, catalogWidth, treeHeight, TRUE);
        MoveWindow(sceneLabel_, 22 + catalogWidth, panelTop, 240, 18, TRUE);
        MoveWindow(sceneTree_, 22 + catalogWidth, panelTop + 18, sceneWidth, treeHeight, TRUE);

        const int btnY = panelTop + 22 + treeHeight;
        MoveWindow(btnDelete_, 14, btnY, 80, 26, TRUE);
    }

    void LoadCatalogTree()
    {
        assetFileNames_.clear();
        catalogTextPool_.clear();
        TreeView_DeleteAllItems(catalogTree_);
        HTREEITEM rootItem = InsertTreeItem(catalogTree_, TVI_ROOT, L"Create", static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), catalogTextPool_);
        HTREEITEM assetsGroupItem = InsertTreeItem(catalogTree_, rootItem, L"Assets", static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), catalogTextPool_);

        assets::AssetRegistry registry;
        std::string err;
        registry.ScanGltf("assets/models", &err);
        const auto paths = registry.GetAssetPaths();
        for (const std::string& p : paths)
        {
            const std::string file = std::filesystem::path(p).filename().string();
            const uint32_t idx = static_cast<uint32_t>(assetFileNames_.size());
            assetFileNames_.push_back(file);

            std::wstring w = ToWide(file);
            InsertTreeItem(catalogTree_, assetsGroupItem, w, static_cast<LPARAM>(EncodePayload(kPayloadTypeAsset, idx)), catalogTextPool_);
        }
        InsertTreeItem(catalogTree_, rootItem, L"Lights (TODO)", static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), catalogTextPool_);
        InsertTreeItem(catalogTree_, rootItem, L"Gameplay Entities (TODO)", static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), catalogTextPool_);

        TreeView_Expand(catalogTree_, rootItem, TVE_EXPAND);
        TreeView_Expand(catalogTree_, assetsGroupItem, TVE_EXPAND);
        if (!assetFileNames_.empty())
        {
            placementAsset_ = assetFileNames_[0];
        }
    }

    void RefreshSceneTree()
    {
        sceneTextPool_.clear();
        TreeView_DeleteAllItems(sceneTree_);
        if (selectedEntity_ != ecs::kInvalidEntity && !scene_.Entities().IsAlive(selectedEntity_))
        {
            selectedEntity_ = ecs::kInvalidEntity;
        }
        HTREEITEM rootItem = InsertTreeItem(sceneTree_, TVI_ROOT, L"Scene", static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), sceneTextPool_);
        HTREEITEM assetsItem = InsertTreeItem(sceneTree_, rootItem, L"Assets", static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), sceneTextPool_);

        const auto* trStorage = scene_.Components().TryGetStorage<scene::TransformComponent>();
        if (trStorage)
        {
            std::unordered_map<ecs::Entity, std::vector<ecs::Entity>> childrenByParent;
            std::vector<ecs::Entity> roots;
            const auto& entities = trStorage->Entities();
            const auto& transforms = trStorage->Components();
            for (size_t i = 0; i < entities.size() && i < transforms.size(); ++i)
            {
                const ecs::Entity e = entities[i];
                const ecs::Entity p = transforms[i].parent;
                if (p == ecs::kInvalidEntity || !scene_.Entities().IsAlive(p))
                {
                    roots.push_back(e);
                }
                else
                {
                    childrenByParent[p].push_back(e);
                }
            }

            const std::function<void(ecs::Entity, HTREEITEM)> addNode =
                [&](ecs::Entity entity, HTREEITEM parentItem)
            {
                std::string objectName = "Entity";
                if (const auto* name = scene_.Components().Get<scene::NameComponent>(entity))
                {
                    if (!name->value.empty())
                    {
                        objectName = name->value;
                    }
                }

                std::string assetName = "-";
                int meshIndex = -1;
                int primitiveIndex = -1;
                if (const auto* mesh = scene_.Components().Get<scene::MeshRendererComponent>(entity))
                {
                    if (!mesh->assetPath.empty())
                    {
                        assetName = mesh->assetPath;
                    }
                    meshIndex = mesh->meshIndex;
                    primitiveIndex = mesh->primitiveIndex;
                }

                float x = 0.0f, y = 0.0f, z = 0.0f;
                if (const auto* t = scene_.Components().Get<scene::TransformComponent>(entity))
                {
                    x = t->local.position.x;
                    y = t->local.position.y;
                    z = t->local.position.z;
                }

                char buf[256];
                sprintf_s(buf, "%s (E%u)",
                    objectName.c_str(),
                    static_cast<unsigned int>(entity));
                const std::wstring w = ToWide(buf);

                const HTREEITEM thisItem = InsertTreeItem(
                    sceneTree_,
                    parentItem,
                    w,
                    static_cast<LPARAM>(EncodePayload(kPayloadTypeEntity, static_cast<uint32_t>(entity))),
                    sceneTextPool_);

                // Composite de informacoes da entidade.
                const HTREEITEM infoItem = InsertTreeItem(
                    sceneTree_,
                    thisItem,
                    L"Info",
                    static_cast<LPARAM>(EncodePayload(kPayloadTypeEntity, static_cast<uint32_t>(entity))),
                    sceneTextPool_);

                char transformBuf[256];
                sprintf_s(transformBuf, "Transform: X=%.2f Y=%.2f Z=%.2f", x, y, z);
                InsertTreeItem(sceneTree_, infoItem, ToWide(transformBuf), static_cast<LPARAM>(EncodePayload(kPayloadTypeEntity, static_cast<uint32_t>(entity))), sceneTextPool_);

                char meshBuf[256];
                sprintf_s(meshBuf, "Mesh: %s | mesh=%d | prim=%d", assetName.c_str(), meshIndex, primitiveIndex);
                InsertTreeItem(sceneTree_, infoItem, ToWide(meshBuf), static_cast<LPARAM>(EncodePayload(kPayloadTypeEntity, static_cast<uint32_t>(entity))), sceneTextPool_);

                // Tamanho (AABB local) do mesh, quando houver.
                if (assetName != "-" && meshIndex >= 0)
                {
                    const std::string fullPath = (std::filesystem::path("assets/models") / assetName).string();
                    std::string err;
                    const auto loaded = assetManager_.LoadGltf(fullPath, &err);
                    if (loaded && meshIndex < static_cast<int>(loaded->scene.meshes.size()))
                    {
                        const auto& meshData = loaded->scene.meshes[static_cast<size_t>(meshIndex)];
                        int primBegin = 0;
                        int primEnd = static_cast<int>(meshData.primitives.size());
                        if (primitiveIndex >= 0 && primitiveIndex < primEnd)
                        {
                            primBegin = primitiveIndex;
                            primEnd = primitiveIndex + 1;
                        }

                        math::Vec3 minV{
                            (std::numeric_limits<float>::max)(),
                            (std::numeric_limits<float>::max)(),
                            (std::numeric_limits<float>::max)()};
                        math::Vec3 maxV{
                            -(std::numeric_limits<float>::max)(),
                            -(std::numeric_limits<float>::max)(),
                            -(std::numeric_limits<float>::max)()};
                        bool hasVertex = false;

                        for (int p = primBegin; p < primEnd; ++p)
                        {
                            const auto& prim = meshData.primitives[static_cast<size_t>(p)];
                            for (const auto& v : prim.vertices)
                            {
                                hasVertex = true;
                                minV.x = (v.position.x < minV.x) ? v.position.x : minV.x;
                                minV.y = (v.position.y < minV.y) ? v.position.y : minV.y;
                                minV.z = (v.position.z < minV.z) ? v.position.z : minV.z;
                                maxV.x = (v.position.x > maxV.x) ? v.position.x : maxV.x;
                                maxV.y = (v.position.y > maxV.y) ? v.position.y : maxV.y;
                                maxV.z = (v.position.z > maxV.z) ? v.position.z : maxV.z;
                            }
                        }

                        if (hasVertex)
                        {
                            const math::Vec3 size{maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z};
                            char sizeBuf[256];
                            sprintf_s(sizeBuf, "Tamanho: W=%.3f H=%.3f D=%.3f", size.x, size.y, size.z);
                            InsertTreeItem(sceneTree_, infoItem, ToWide(sizeBuf), static_cast<LPARAM>(EncodePayload(kPayloadTypeEntity, static_cast<uint32_t>(entity))), sceneTextPool_);
                        }
                    }
                }

                // Files associados ao asset.
                if (assetName != "-")
                {
                    const HTREEITEM filesItem = InsertTreeItem(
                        sceneTree_,
                        thisItem,
                        L"Files",
                        static_cast<LPARAM>(EncodePayload(kPayloadTypeEntity, static_cast<uint32_t>(entity))),
                        sceneTextPool_);

                    std::set<std::string> files{};
                    const std::filesystem::path gltfPath = std::filesystem::path("assets/models") / assetName;
                    files.insert(gltfPath.filename().string());

                    const std::filesystem::path candidateBin = gltfPath.parent_path() / (gltfPath.stem().string() + ".bin");
                    if (std::filesystem::exists(candidateBin))
                    {
                        files.insert(candidateBin.filename().string());
                    }

                    std::string loadErr;
                    const auto loaded = assetManager_.LoadGltf(gltfPath.string(), &loadErr);
                    if (loaded)
                    {
                        for (const auto& mat : loaded->scene.materials)
                        {
                            if (!mat.baseColorTexturePath.empty())
                            {
                                files.insert(std::filesystem::path(mat.baseColorTexturePath).filename().string());
                            }
                        }
                    }

                    for (const auto& file : files)
                    {
                        InsertTreeItem(
                            sceneTree_,
                            filesItem,
                            ToWide(file),
                            static_cast<LPARAM>(EncodePayload(kPayloadTypeEntity, static_cast<uint32_t>(entity))),
                            sceneTextPool_);
                    }
                    TreeView_Expand(sceneTree_, filesItem, TVE_EXPAND);
                }

                TreeView_Expand(sceneTree_, infoItem, TVE_EXPAND);
            };

            for (const ecs::Entity root : roots)
            {
                addNode(root, assetsItem);
            }
        }

        TreeView_Expand(sceneTree_, rootItem, TVE_EXPAND);
        TreeView_Expand(sceneTree_, assetsItem, TVE_EXPAND);
    }

    uint32_t GetViewportHeight() const
    {
        if (height_ <= static_cast<uint32_t>(kPanelHeight + 8))
        {
            return height_ > 40 ? height_ - 40 : height_;
        }
        return height_ - static_cast<uint32_t>(kPanelHeight);
    }

    void ApplyViewportLayout()
    {
        const uint32_t viewportHeight = GetViewportHeight();
        dxContext_.SetViewport(0, 0, width_, viewportHeight);
        renderer_.OnResize(width_, viewportHeight);
    }

    editor::CameraGizmoDelta DrawCameraGizmo()
    {
        return cameraGizmo_.DrawCameraGizmo(cameraComponent_);
    }

    void UpdateCameraInput(float dtSeconds, const editor::CameraGizmoDelta& gizmoDelta)
    {
        const input::InputFrame frame = inputSystem_.BuildFrameInput();
        camera::CameraInputState inputState{};
        camera::UpdateCameraInput(frame, gizmoDelta, inputState);

        const float aspect = (GetViewportHeight() > 0)
            ? (static_cast<float>(width_) / static_cast<float>(GetViewportHeight()))
            : (16.0f / 9.0f);
        cameraSystem_.Update(cameraComponent_, inputState, aspect, dtSeconds);
        renderer_.SetCameraMatrices(cameraComponent_.viewMatrix, cameraComponent_.projectionMatrix);
    }

    void Render()
    {
        const uint64_t nowMs = GetTickCount64();
        float dtSeconds = static_cast<float>(nowMs - lastTickMs_) * 0.001f;
        if (dtSeconds <= 0.0f || dtSeconds > 0.2f)
        {
            dtSeconds = 1.0f / 60.0f;
        }
        lastTickMs_ = nowMs;

        const float clearColor[4] = {0.08f, 0.09f, 0.12f, 1.0f};
        if (!dxContext_.BeginFrame(clearColor))
        {
            return;
        }
        editorUi_.BeginFrame();
        const editor::CameraGizmoDelta gizmoDelta = DrawCameraGizmo();
        UpdateCameraInput(dtSeconds, gizmoDelta);
        editorUi_.Update(scene_, assetManager_, dtSeconds);
        sceneRenderer_.Render(scene_, dxContext_, renderer_);
        editorUi_.EndFrame();
        dxContext_.EndFrame();
    }

private:
    assets::AssetManager assetManager_{};
    assets::MaterialLibrary materialLibrary_{};
    scene::Scene scene_{};
    scene::SceneRenderer sceneRenderer_{};
    std::string activeProjectPath_{};
    std::string activeScenePath_{};
    std::vector<std::string> assetFileNames_{};
    std::deque<std::wstring> catalogTextPool_{};
    std::deque<std::wstring> sceneTextPool_{};
    std::string placementAsset_{};
    ecs::Entity selectedEntity_ = ecs::kInvalidEntity;
    HINSTANCE instance_ = nullptr;
    HWND backButton_ = nullptr;
    HWND catalogLabel_ = nullptr;
    HWND sceneLabel_ = nullptr;
    HWND catalogTree_ = nullptr;
    HWND sceneTree_ = nullptr;
    HWND btnDelete_ = nullptr;
    core::Window window_{};
    renderer::Dx11Context dxContext_{};
    renderer::Dx11Renderer renderer_{};
    editor::EditorUI editorUi_{};
    input::InputSystem inputSystem_{};
    camera::CameraComponent cameraComponent_{};
    camera::CameraSystem cameraSystem_{};
    editor::CameraGizmo cameraGizmo_{};
    uint32_t width_ = 1280;
    uint32_t height_ = 720;
    float mouseNdcX_ = 0.0f;
    float mouseNdcY_ = 0.0f;
    bool cursorInsideViewport_ = false;
    bool isDraggingEntity_ = false;
    float dragStartMouseNdcX_ = 0.0f;
    float dragStartMouseNdcY_ = 0.0f;
    math::Vec3 dragStartPosition_{};
    uint64_t lastTickMs_ = 0;
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    App app{};
    if (!app.Initialize(hInstance))
    {
        return -1;
    }
    return app.Run();
}
