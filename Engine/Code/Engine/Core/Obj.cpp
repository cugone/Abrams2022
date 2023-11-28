#include "Engine/Core/Obj.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/MtlReader.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Engine/Renderer/Renderer.hpp"

#include <format>
#include <numeric>
#include <sstream>
#include <string>

namespace FileUtils {

//Run only as an asynchronous operation highly recommended.
Obj::Obj(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    {
        const auto error_msg = std::format("Obj: {} failed to load.\nReason: It does not exist.\n", filepath);
        GUARANTEE_OR_DIE(FS::exists(filepath), error_msg.c_str());
    }
    filepath = FS::canonical(filepath);
    filepath.make_preferred();
    {
        const auto error_msg = std::format("Obj: {} failed to load.", filepath);
        GUARANTEE_OR_DIE(Load(filepath), error_msg.c_str());
    }
}

bool Obj::Load(std::filesystem::path filepath) noexcept {
    PROFILE_LOG_SCOPE_FUNCTION();

    namespace FS = std::filesystem;
    bool not_exist = !FS::exists(filepath);
    std::string valid_extension = ".obj";
    bool not_obj = StringUtils::ToLowerCase(filepath.extension().string()) != valid_extension;
    bool invalid = not_exist || not_obj;
    if(invalid) {
        DebuggerPrintf(std::format("{} is not a .obj file.\n", filepath));
        return false;
    }
    filepath = FS::canonical(filepath);
    filepath.make_preferred();
    return Parse(filepath);
}

bool Obj::Save(std::filesystem::path filepath) noexcept {
    PROFILE_LOG_SCOPE_FUNCTION();

    namespace FS = std::filesystem;
    filepath.make_preferred();

    m_is_saving = true;
    std::ostringstream buffer;
    buffer << std::fixed << std::setprecision(6);
    for(auto& v : m_verts) {
        buffer << "v " << v.x << ' ' << v.y << ' ' << v.z << '\n';
    }
    for(auto& v : m_normals) {
        //v.Normalize();
        buffer << "vn " << v.x << ' ' << v.y << ' ' << v.z << '\n';
    }
    for(auto& v : m_tex_coords) {
        buffer << "vt " << v.x << ' ' << v.y << ' ' << v.z << '\n';
    }
    bool has_vn = !m_normals.empty();
    bool has_vt = !m_tex_coords.empty();
    bool has_neither = !has_vt && !has_vn;
    for(auto iter = std::begin(m_face_idxs); iter != std::end(m_face_idxs); /* DO NOTHING */) {
        auto value1 = (*iter).a;
        auto value2 = (*iter).b;
        auto value3 = (*iter).c;
        buffer << "f ";
        buffer << (1 + value1);
        if(!has_neither) {
            buffer << '/';
            if(has_vt) {
                buffer << (1 + value2);
            }
            buffer << '/';
            if(has_vn) {
                buffer << (1 + value3);
            }
        }
        ++iter;
        value1 = (*iter).a;
        value2 = (*iter).b;
        value3 = (*iter).c;
        buffer << ' ';
        buffer << (1 + value1);
        if(!has_neither) {
            buffer << '/';
            if(has_vt) {
                buffer << (1 + value2);
            }
            buffer << '/';
            if(has_vn) {
                buffer << (1 + value3);
            }
        }
        ++iter;
        value1 = (*iter).a;
        value2 = (*iter).b;
        value3 = (*iter).c;
        buffer << ' ';
        buffer << (1 + value1);
        if(!has_neither) {
            buffer << '/';
            if(has_vt) {
                buffer << (1 + value2);
            }
            buffer << '/';
            if(has_vn) {
                buffer << (1 + value3);
            }
        }
        buffer << '\n';
        ++iter;
    }
    buffer.flush();
    if(FileUtils::WriteBufferToFile(buffer.str().data(), buffer.str().size(), filepath)) {
        m_is_saved = true;
        m_is_saving = false;
        return true;
    }
    m_is_saving = false;
    m_is_saved = false;
    return false;
}

bool Obj::IsLoaded() const noexcept {
    return m_is_loaded;
}

bool Obj::IsLoading() const noexcept {
    return m_is_loading;
}

bool Obj::IsSaving() const noexcept {
    return m_is_saving;
}

bool Obj::IsSaved() const noexcept {
    return m_is_saved;
}

const std::vector<Vertex3D>& Obj::GetVbo() const noexcept {
    return m_vbo;
}

const std::vector<unsigned int>& Obj::GetIbo() const noexcept {
    return m_ibo;
}

void Obj::Unload() noexcept {
    m_is_saved = false;
    m_is_saving = false;
    m_is_loading = false;
    m_is_loaded = false;
    m_face_idxs.clear();
    m_face_idxs.shrink_to_fit();
    m_normals.clear();
    m_normals.shrink_to_fit();
    m_tex_coords.clear();
    m_tex_coords.shrink_to_fit();
    m_verts.clear();
    m_verts.shrink_to_fit();
    m_ibo.clear();
    m_ibo.shrink_to_fit();
    m_vbo.clear();
    m_vbo.shrink_to_fit();
    m_materialName.clear();
    m_materialName.shrink_to_fit();
}

bool Obj::Parse(const std::filesystem::path& filepath) noexcept {
    PROFILE_LOG_SCOPE_FUNCTION();
    m_verts.clear();
    m_tex_coords.clear();
    m_normals.clear();
    m_vbo.clear();
    m_ibo.clear();
    m_face_idxs.clear();

    m_is_loaded = false;
    m_is_saving = false;
    m_is_saved = false;
    m_is_loading = true;
    if(auto buffer = FileUtils::ReadBinaryBufferFromFile(filepath)) {
        if(std::stringstream ss{}; ss.write(reinterpret_cast<const char*>(buffer->data()), buffer->size())) {
            buffer->clear();
            buffer->shrink_to_fit();
            ss.clear();
            ss.seekg(ss.beg);
            ss.seekp(ss.beg);
            std::string cur_line{};
            std::size_t vert_count{};
            unsigned long long line_index = 0;
            while(std::getline(ss, cur_line, '\n')) {
                if(StringUtils::StartsWith(cur_line, "v ")) {
                    ++vert_count;
                }
            }
            ss.clear();
            ss.seekg(ss.beg);
            ss.seekp(ss.beg);
            m_verts.reserve(vert_count);
            m_vbo.resize(vert_count);
            while(std::getline(ss, cur_line, '\n')) {
                ++line_index;
                cur_line = cur_line.substr(0, cur_line.find_first_of('#'));
                if(cur_line.empty()) {
                    continue;
                }
                cur_line = StringUtils::TrimWhitespace(cur_line);
                const auto&& [key, value] = StringUtils::SplitOnFirst(cur_line, ' ');
                if(key == "mtllib") {
                    auto folder = filepath.parent_path();
                    auto mtlpath = folder / value;
                    MtlReader mtl{};
                    if(!mtl.Parse(mtlpath)) {
                        DebuggerPrintf("Ill-formed material library in OBJ!\n");
                        PrintErrorToDebugger(filepath, "mtllib", line_index);
                        return false;
                    }
                    continue;
                } else if(key == "o") {
                    m_objectName = value;
                    continue;
                } else if(key == "usemtl") {
                    m_materialName = value;
                    continue;
                } else if(key == "v") {
                    auto elems = StringUtils::Split(value, ' ');
                    std::string v_str = {"["};
                    auto count = std::size_t{0u};
                    for(const auto e : elems) {
                        if(count++ > 0) {
                            v_str += ",";
                        }
                        v_str += e;
                    }
                    switch(elems.size()) {
                    case 4: /* DO NOTHING */ break;
                    case 3: v_str += ",1.0"; break;
                    case 2: v_str += ",0.0,1.0"; break;
                    case 1: v_str += ",0.0,0.0,1.0"; break;
                    default: PrintErrorToDebugger(filepath, "vertex", line_index); return false;
                    }
                    v_str += "]";
                    Vector4 v(v_str);
                    v.CalcHomogeneous();
                    m_verts.emplace_back(v);
                    continue;
                } else if(key == "vt") {
                    auto elems = StringUtils::Split(value, ' ');
                    std::string v_str = {"["};
                    auto count = std::size_t{0u};
                    for(const auto e : elems) {
                        if(count++ > 0) {
                            v_str += ",";
                        }
                        v_str += e;
                    }
                    switch(elems.size()) {
                    case 3: /* DO NOTHING */ break;
                    case 2: v_str += ",0.0"; break;
                    case 1: v_str += ",0.0,0.0"; break;
                    default: PrintErrorToDebugger(filepath, "texture coordinate", line_index); return false;
                    }
                    v_str += "]";
                    m_tex_coords.emplace_back(v_str);
                    continue;
                } else if(key == "vn") {
                    auto elems = StringUtils::Split(value, ' ');
                    std::string v_str = {"["};
                    auto count = std::size_t{0u};
                    for(const auto e : elems) {
                        if(count++ > 0) {
                            v_str += ",";
                        }
                        v_str += e;
                    }
                    if(elems.size() != 3) {
                        PrintErrorToDebugger(filepath, "vertex normal", line_index);
                        return false;
                    }
                    v_str += "]";
                    m_normals.emplace_back(v_str);
                    continue;
                } else if(key == "f") {
                    if(value.find('-') != std::string::npos) {
                        DebuggerPrintf("ERROR: OBJ implementation does not support relative reference numbers!\n");
                        PrintErrorToDebugger(filepath, "face index", line_index);
                        return false;
                    }
                    auto tris = StringUtils::Split(value, ' ');
                    if(tris.size() != 3) {
                        DebuggerPrintf("WARNING: Performance will be reduced when loading non-triangle polygons!\n");
                        PrintErrorToDebugger(filepath, "face triplet", line_index);
                    }
                    TriangulatePolygon(tris);
                    continue;
                }
            }
            m_ibo.shrink_to_fit();
            m_is_loaded = true;
            m_is_loading = false;
            return true;
        }
    }
    m_is_loading = false;
    return false;
}

void Obj::PrintErrorToDebugger(std::filesystem::path filepath, const std::string& elementType, unsigned long long line_index) const noexcept {
    namespace FS = std::filesystem;
    filepath = FS::canonical(filepath);
    filepath.make_preferred();
    DebuggerPrintf(std::format("{}({}): Invalid {}\n", filepath, line_index, elementType));
}

Vertex3D Obj::FaceTriToVertex(const std::string& t) const noexcept {
    auto elems = StringUtils::Split(t, '/', false);
    Vertex3D vertex{};
    auto elem_count = elems.size();
    for(auto i = 0u; i < elem_count; ++i) {
        switch(i) {
        case 0:
            if(!elems[0].empty()) {
                std::size_t cur_v = std::stoul(elems[0]);
                vertex.position = m_verts[cur_v - 1];
            }
            break;
        case 1:
            if(!elems[1].empty()) {
                std::size_t cur_vt = std::stoul(elems[1]);
                vertex.texcoords = Vector2{m_tex_coords[cur_vt - 1]};
            }
            break;
        case 2:
            if(!elems[2].empty()) {
                std::size_t cur_vn = std::stoul(elems[2]);
                vertex.normal = m_normals[cur_vn - 1];
            }
            break;
        default: break;
        }
    }
    return vertex;
}

Obj::FaceIdxs Obj::FaceTriToFaceIdx(const std::string& t) const noexcept {
    auto elems = StringUtils::Split(t, '/', false);
    decltype(m_face_idxs)::value_type face{};
    auto elem_count = elems.size();
    for(auto i = 0u; i < elem_count; ++i) {
        switch(i) {
        case 0:
            if(!elems[0].empty()) {
                face.a = std::stoul(elems[0]);
            } else {
                face.a = static_cast<std::size_t>(-1);
            }
            break;
        case 1:
            if(!elems[1].empty()) {
                face.b = std::stoul(elems[1]);
            } else {
                face.b = static_cast<std::size_t>(-1);
            }
            break;
        case 2:
            if(!elems[2].empty()) {
                face.c = std::stoul(elems[2]);
            } else {
                face.c = static_cast<std::size_t>(-1);
            }
            break;
        default: break;
        }
    }
    return face;
}

void Obj::TriangulatePolygon(const std::vector<std::string>& tris) noexcept {
    std::vector<std::size_t> vbo_idxs{};
    const auto tri_count = tris.size();
    vbo_idxs.resize(tri_count);
    auto ai = std::size_t{0u};
    for(const auto& t : tris) {
        auto elems = StringUtils::Split(t, '/', false);
        std::size_t cur_v = std::stoul(elems[0]);
        std::size_t vbo_index = cur_v - std::size_t{1u};
        vbo_idxs[ai++] = vbo_index;
    }
    for(auto i = std::size_t{0u}; i != tri_count; ++i) {
        m_vbo[vbo_idxs[i]] = FaceTriToVertex(tris[i]);
    }
    for(auto i = std::size_t{0u}; i != tri_count - 2; ++i) {
        m_ibo.push_back(static_cast<unsigned int>(vbo_idxs[0]));
        m_ibo.push_back(static_cast<unsigned int>(vbo_idxs[(i + 1) % tri_count]));
        m_ibo.push_back(static_cast<unsigned int>(vbo_idxs[(i + 2) % tri_count]));
        m_face_idxs.emplace_back(FaceTriToFaceIdx(tris[0]));
        m_face_idxs.emplace_back(FaceTriToFaceIdx(tris[(i + 1) % tri_count]));
        m_face_idxs.emplace_back(FaceTriToFaceIdx(tris[(i + 2) % tri_count]));
    }
}


} // namespace FileUtils
