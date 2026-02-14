#include <windows.h>
#include <commctrl.h>

#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "assets/AssetManager.h"
#include "assets/AssetRegistry.h"
#include "assets/MaterialLibrary.h"
#include "assets/SceneRepository.h"
#include "core/Window.h"
#include "editor/ProjectBrowserWindow.h"
#include "editor/EditorUI.h"
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
    constexpr int kBtnMoveXpId = 9202;
    constexpr int kBtnMoveXmId = 9203;
    constexpr int kBtnMoveYpId = 9204;
    constexpr int kBtnMoveYmId = 9205;
    constexpr int kBtnMoveZpId = 9206;
    constexpr int kBtnMoveZmId = 9207;
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
        renderer_.OnMouseMoveNdc(mouseNdcX_, mouseNdcY_);
    }

    void OnMouseButton(bool leftDown) override
    {
        if (leftDown && cursorInsideViewport_ && !placementAsset_.empty())
        {
            math::Transform root{};
            root.position = math::Vec3{mouseNdcX_, mouseNdcY_, 0.0f};
            InstantiateAssetHierarchy(placementAsset_, root);
            SaveSceneFromEcs();
            RefreshSceneTree();
        }
        renderer_.OnMouseButton(leftDown);
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

        if (id == kBtnMoveXpId) { MoveSelectedEntity(0.1f, 0.0f, 0.0f); return; }
        if (id == kBtnMoveXmId) { MoveSelectedEntity(-0.1f, 0.0f, 0.0f); return; }
        if (id == kBtnMoveYpId) { MoveSelectedEntity(0.0f, 0.1f, 0.0f); return; }
        if (id == kBtnMoveYmId) { MoveSelectedEntity(0.0f, -0.1f, 0.0f); return; }
        if (id == kBtnMoveZpId) { MoveSelectedEntity(0.0f, 0.0f, 0.1f); return; }
        if (id == kBtnMoveZmId) { MoveSelectedEntity(0.0f, 0.0f, -0.1f); return; }
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
        const std::string fullPath = (std::filesystem::path("assets/models") / assetFileName).string();
        std::string err;
        const auto asset = assetManager_.LoadGltf(fullPath, &err);
        if (!asset)
        {
            return;
        }

        const ecs::Entity rootEntity = scene_.CreateEntity();
        scene::TransformComponent rootTc{};
        rootTc.local = rootTransform;
        scene_.Components().Add<scene::TransformComponent>(rootEntity, rootTc);
        scene_.Components().Add<scene::NameComponent>(
            rootEntity,
            scene::NameComponent{std::filesystem::path(assetFileName).stem().string() + "_root"});

        std::vector<ecs::Entity> nodeEntities(asset->scene.nodes.size(), ecs::kInvalidEntity);
        for (size_t i = 0; i < asset->scene.nodes.size(); ++i)
        {
            nodeEntities[i] = scene_.CreateEntity();
        }

        for (size_t i = 0; i < asset->scene.nodes.size(); ++i)
        {
            const auto& node = asset->scene.nodes[i];
            const ecs::Entity e = nodeEntities[i];
            const std::string nodeName = !node.name.empty()
                ? node.name
                : (std::filesystem::path(assetFileName).stem().string() + "_node_" + std::to_string(i));
            scene_.Components().Add<scene::NameComponent>(e, scene::NameComponent{nodeName});

            scene::TransformComponent tc{};
            tc.local = node.localTransform;
            tc.parent = (node.parentIndex >= 0 && node.parentIndex < static_cast<int>(nodeEntities.size()))
                ? nodeEntities[static_cast<size_t>(node.parentIndex)]
                : rootEntity;
            scene_.Components().Add<scene::TransformComponent>(e, tc);

            if (node.meshIndex >= 0)
            {
                scene_.Components().Add<scene::MeshRendererComponent>(
                    e,
                    scene::MeshRendererComponent{assetFileName, node.meshIndex, -1, true});
            }
        }
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

    void MoveSelectedEntity(float dx, float dy, float dz)
    {
        if (selectedEntity_ == ecs::kInvalidEntity)
        {
            return;
        }
        auto* t = scene_.Components().Get<scene::TransformComponent>(selectedEntity_);
        if (!t)
        {
            return;
        }
        t->local.position.x += dx;
        t->local.position.y += dy;
        t->local.position.z += dz;
        SaveSceneFromEcs();
        RefreshSceneTree();
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
        btnXp_ = CreateWindowExW(0, L"BUTTON", L"+X", WS_CHILD | WS_VISIBLE, 0, 0, 45, 26,
            window_.GetHandle(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBtnMoveXpId)), instance_, nullptr);
        btnXm_ = CreateWindowExW(0, L"BUTTON", L"-X", WS_CHILD | WS_VISIBLE, 0, 0, 45, 26,
            window_.GetHandle(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBtnMoveXmId)), instance_, nullptr);
        btnYp_ = CreateWindowExW(0, L"BUTTON", L"+Y", WS_CHILD | WS_VISIBLE, 0, 0, 45, 26,
            window_.GetHandle(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBtnMoveYpId)), instance_, nullptr);
        btnYm_ = CreateWindowExW(0, L"BUTTON", L"-Y", WS_CHILD | WS_VISIBLE, 0, 0, 45, 26,
            window_.GetHandle(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBtnMoveYmId)), instance_, nullptr);
        btnZp_ = CreateWindowExW(0, L"BUTTON", L"+Z", WS_CHILD | WS_VISIBLE, 0, 0, 45, 26,
            window_.GetHandle(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBtnMoveZpId)), instance_, nullptr);
        btnZm_ = CreateWindowExW(0, L"BUTTON", L"-Z", WS_CHILD | WS_VISIBLE, 0, 0, 45, 26,
            window_.GetHandle(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBtnMoveZmId)), instance_, nullptr);

        // Force readable text rendering in all child controls.
        const HFONT uiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        SendMessageW(catalogLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(sceneLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(catalogTree_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(sceneTree_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(btnDelete_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(btnXp_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(btnXm_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(btnYp_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(btnYm_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(btnZp_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(btnZm_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);

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
        int x = 14;
        MoveWindow(btnDelete_, x, btnY, 80, 26, TRUE); x += 90;
        MoveWindow(btnXp_, x, btnY, 45, 26, TRUE); x += 50;
        MoveWindow(btnXm_, x, btnY, 45, 26, TRUE); x += 55;
        MoveWindow(btnYp_, x, btnY, 45, 26, TRUE); x += 50;
        MoveWindow(btnYm_, x, btnY, 45, 26, TRUE); x += 55;
        MoveWindow(btnZp_, x, btnY, 45, 26, TRUE); x += 50;
        MoveWindow(btnZm_, x, btnY, 45, 26, TRUE);
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
        selectedEntity_ = ecs::kInvalidEntity;
        HTREEITEM rootItem = InsertTreeItem(sceneTree_, TVI_ROOT, L"Scene", static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), sceneTextPool_);
        HTREEITEM assetsGroupItem = InsertTreeItem(sceneTree_, rootItem, L"Assets", static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), sceneTextPool_);
        HTREEITEM entitiesGroupItem = InsertTreeItem(sceneTree_, rootItem, L"Entities", static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), sceneTextPool_);

        const auto renderList = scene_.BuildRenderList();
        std::unordered_map<std::string, uint32_t> counts;
        for (const auto& item : renderList)
        {
            counts[item.assetPath]++;
        }
        for (const auto& [asset, count] : counts)
        {
            char buf[256];
            sprintf_s(buf, "%s (%u)", asset.c_str(), static_cast<unsigned int>(count));
            std::wstring w = ToWide(buf);
            InsertTreeItem(sceneTree_, assetsGroupItem, w, static_cast<LPARAM>(EncodePayload(kPayloadTypeNone, 0)), sceneTextPool_);
        }

        for (const auto& item : renderList)
        {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            if (const auto* t = scene_.Components().Get<scene::TransformComponent>(item.entity))
            {
                x = t->local.position.x;
                y = t->local.position.y;
                z = t->local.position.z;
            }

            std::string objectName = "Entity";
            if (const auto* name = scene_.Components().Get<scene::NameComponent>(item.entity))
            {
                if (!name->value.empty())
                {
                    objectName = name->value;
                }
            }

            char buf[256];
            sprintf_s(buf, "%s (E%u) | %s | X=%.2f Y=%.2f Z=%.2f",
                objectName.c_str(),
                static_cast<unsigned int>(item.entity),
                item.assetPath.c_str(),
                x, y, z);
            std::wstring w = ToWide(buf);
            InsertTreeItem(
                sceneTree_,
                entitiesGroupItem,
                w,
                static_cast<LPARAM>(EncodePayload(kPayloadTypeEntity, static_cast<uint32_t>(item.entity))),
                sceneTextPool_);
        }

        TreeView_Expand(sceneTree_, rootItem, TVE_EXPAND);
        TreeView_Expand(sceneTree_, assetsGroupItem, TVE_EXPAND);
        TreeView_Expand(sceneTree_, entitiesGroupItem, TVE_EXPAND);
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
    HWND btnXp_ = nullptr;
    HWND btnXm_ = nullptr;
    HWND btnYp_ = nullptr;
    HWND btnYm_ = nullptr;
    HWND btnZp_ = nullptr;
    HWND btnZm_ = nullptr;
    core::Window window_{};
    renderer::Dx11Context dxContext_{};
    renderer::Dx11Renderer renderer_{};
    editor::EditorUI editorUi_{};
    uint32_t width_ = 1280;
    uint32_t height_ = 720;
    float mouseNdcX_ = 0.0f;
    float mouseNdcY_ = 0.0f;
    bool cursorInsideViewport_ = false;
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
