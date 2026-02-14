#pragma once

#include <string>

namespace editor
{
    // Imports a glTF/glb file into modelsDirectory.
    // For .gltf, copies referenced buffer/image files as well.
    bool ImportModelWithDependencies(
        const std::string& sourceModelPath,
        const std::string& modelsDirectory,
        std::string& outImportedFileName,
        std::string* outError = nullptr);
}
