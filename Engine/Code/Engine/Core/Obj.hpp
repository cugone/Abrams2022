#pragma once

#include "Engine/Renderer/Vertex3D.hpp"

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

class Renderer;

namespace FileUtils {

class Obj {
public:
    Obj() = default;
    Obj(const Obj& other) = default;
    Obj(Obj&& other) = default;
    Obj& operator=(const Obj& rhs) = default;
    Obj& operator=(Obj&& rhs) = default;
    explicit Obj(std::filesystem::path filepath) noexcept;
    ~Obj() = default;

    [[nodiscard]] const std::vector<Vertex3D>& GetVbo() const noexcept;
    [[nodiscard]] const std::vector<unsigned int>& GetIbo() const noexcept;

    void Unload() noexcept;
    [[nodiscard]] bool Load(std::filesystem::path filepath) noexcept;
    [[nodiscard]] bool Save(std::filesystem::path filepath) noexcept;
    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool IsLoading() const noexcept;
    [[nodiscard]] bool IsSaving() const noexcept;
    [[nodiscard]] bool IsSaved() const noexcept;

protected:
private:
    [[nodiscard]] bool Parse(const std::filesystem::path& filepath) noexcept;

    void PrintErrorToDebugger(std::filesystem::path filepath, const std::string& elementType, unsigned long long line_index) const noexcept;

    Vertex3D FaceTriToVertex(const std::string& t) const noexcept;

    struct FaceIdxs {
        std::size_t a;
        std::size_t b;
        std::size_t c;
    };
    FaceIdxs FaceTriToFaceIdx(const std::string& t) const noexcept;

    void TriangulatePolygon(const std::vector<std::string>& tris) noexcept;

    std::string m_materialName{};
    std::string m_objectName{};
    std::vector<Vertex3D> m_vbo{};
    std::vector<unsigned int> m_ibo{};
    std::vector<Vector3> m_verts{};
    std::vector<Vector3> m_tex_coords{};
    std::vector<Vector3> m_normals{};
    std::vector<FaceIdxs> m_face_idxs{};
    std::atomic_bool m_is_loaded = false;
    std::atomic_bool m_is_loading = false;
    std::atomic_bool m_is_saving = false;
    std::atomic_bool m_is_saved = false;
};

} // namespace FileUtils
