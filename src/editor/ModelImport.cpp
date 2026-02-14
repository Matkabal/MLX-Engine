#include "editor/ModelImport.h"
#include "core/Logger.h"

#include <filesystem>
#include <fstream>

#include "json.hpp"

namespace editor
{
    namespace
    {
        bool CopyOneFile(
            const std::filesystem::path& src,
            const std::filesystem::path& dst,
            std::string* outError)
        {
            std::error_code ec;
            std::filesystem::create_directories(dst.parent_path(), ec);
            if (ec)
            {
                if (outError)
                {
                    *outError = "Failed creating directory: " + dst.parent_path().string();
                }
                return false;
            }

            std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec)
            {
                if (outError)
                {
                    *outError = "Failed copying file: " + src.string();
                }
                return false;
            }
            return true;
        }
    } // namespace

    bool ImportModelWithDependencies(
        const std::string& sourceModelPath,
        const std::string& modelsDirectory,
        std::string& outImportedFileName,
        std::string* outError)
    {
        LOG_METHOD("ModelImport", "ImportModelWithDependencies");
        namespace fs = std::filesystem;

        const fs::path srcPath(sourceModelPath);
        if (!fs::exists(srcPath) || !fs::is_regular_file(srcPath))
        {
            LOG_ERROR("ModelImport", "ImportModelWithDependencies", std::string("Source model does not exist: ") + sourceModelPath);
            if (outError)
            {
                *outError = "Source model does not exist: " + sourceModelPath;
            }
            return false;
        }

        fs::path dstRoot(modelsDirectory);
        std::error_code ec;
        fs::create_directories(dstRoot, ec);
        if (ec)
        {
            LOG_ERROR("ModelImport", "ImportModelWithDependencies", std::string("Failed creating models directory: ") + modelsDirectory);
            if (outError)
            {
                *outError = "Failed creating models directory: " + modelsDirectory;
            }
            return false;
        }

        const std::string ext = srcPath.extension().string();
        const fs::path dstModelPath = dstRoot / srcPath.filename();
        if (!CopyOneFile(srcPath, dstModelPath, outError))
        {
            return false;
        }

        if (ext == ".glb" || ext == ".GLB")
        {
            outImportedFileName = dstModelPath.filename().string();
            return true;
        }

        // For .gltf, copy relative dependencies referenced by URI.
        nlohmann::json j;
        try
        {
            std::ifstream in(srcPath);
            in >> j;
        }
        catch (const std::exception& ex)
        {
            if (outError)
            {
                *outError = std::string("Failed reading glTF JSON: ") + ex.what();
            }
            return false;
        }

        const fs::path srcDir = srcPath.parent_path();
        auto copyUriIfNeeded = [&](const nlohmann::json& uriNode) -> bool {
            if (!uriNode.is_string())
            {
                return true;
            }
            const std::string uri = uriNode.get<std::string>();
            if (uri.rfind("data:", 0) == 0)
            {
                return true;
            }

            const fs::path depSrc = srcDir / uri;
            const fs::path depDst = dstRoot / uri;
            return CopyOneFile(depSrc, depDst, outError);
        };

        if (j.contains("buffers") && j["buffers"].is_array())
        {
            for (const auto& b : j["buffers"])
            {
                if (b.contains("uri") && !copyUriIfNeeded(b["uri"]))
                {
                    return false;
                }
            }
        }

        if (j.contains("images") && j["images"].is_array())
        {
            for (const auto& img : j["images"])
            {
                if (img.contains("uri") && !copyUriIfNeeded(img["uri"]))
                {
                    return false;
                }
            }
        }

        outImportedFileName = dstModelPath.filename().string();
        return true;
    }
}
