#pragma once

#include "oor/base/threading/CancellationToken.hpp"
#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/io/hdf5/Hdf5Limits.hpp"
#include "oor/io/hdf5/Hdf5Type.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace oor::io
{

enum class Hdf5NodeKind
{
    Group,
    Dataset,
    NamedType,    // a committed data type
    SoftLink,     // a link by path, never followed by the walk
    ExternalLink, // a link to an object of another file, never followed
    Other
};

[[nodiscard]] const char* nodeKindName(Hdf5NodeKind kind) noexcept;

struct Hdf5Filter
{
    std::string name; // "deflate", "shuffle", "fletcher32", "szip", "nbit", "scaleoffset", or "filter 32001"
    std::uint32_t id = 0;
    bool optional = false;
    std::vector<std::uint32_t> parameters;
};

// An attribute of an object: its type, its shape and, when it is small and of a plain type (integer, float, string,
// enum, and arrays of them), its value.
struct Hdf5Attribute
{
    using Value = std::variant<std::monostate, std::vector<std::int64_t>, std::vector<std::uint64_t>,
                               std::vector<double>, std::vector<std::string>>;

    std::string name;
    Hdf5Type type;
    std::vector<std::uint64_t> shape; // empty for a scalar
    bool isNull = false;              // an empty dataspace: no value at all
    Value value;                      // std::monostate when the value was not read
    std::string unreadReason;         // why, when it was not ("larger than the cap", "compound type", ...)

    [[nodiscard]] std::uint64_t elementCount() const noexcept;
};

// One object (or link) of the file. Nothing in a node comes from reading dataset data.
struct Hdf5Node
{
    std::string name; // the last component ("" for the root)
    std::string path; // "/RESULTS/NODAL" ("/" for the root)
    Hdf5NodeKind kind = Hdf5NodeKind::Group;
    std::uint32_t depth = 0;
    std::uint32_t parent = 0; // index of the parent node (the root is its own parent)

    // Datasets and named types.
    std::optional<Hdf5Type> type;
    // Datasets. `shape` is empty for a scalar dataspace; an empty dataspace (null) has `isNull`.
    std::vector<std::uint64_t> shape;
    std::vector<std::uint64_t> maxShape; // UNLIMITED where the dimension can grow without bound
    bool isNull = false;
    std::string layout;                // "contiguous", "chunked", "compact", "virtual"
    std::vector<std::uint64_t> chunks; // empty unless chunked
    std::vector<Hdf5Filter> filters;
    std::uint64_t storageBytes = 0; // bytes allocated in the file for the data (compressed size for a filtered one)

    std::vector<Hdf5Attribute> attributes;
    std::vector<std::uint32_t> children; // indices in the tree, in the name order of the file

    // Soft link: the path it holds. External link: "file.h5::/path/in/it".
    std::string linkTarget;
    // A hard link to an object that the walk already listed under another path (the objects are listed once, which also
    // ends a loop of hard links): the path of the first listing. Such a node has no children of its own.
    std::string aliasOf;

    static constexpr std::uint64_t UNLIMITED = ~std::uint64_t{0};

    [[nodiscard]] std::uint64_t elementCount() const noexcept;
    [[nodiscard]] const Hdf5Attribute* attribute(const std::string& attributeName) const noexcept;
};

// The structure of an HDF5 file: groups, datasets, named types, links, attributes and the description of every dataset,
// read without reading a byte of dataset data. Soft links and external links are listed, never followed; a hard link
// to an object already listed is listed as an alias (so a loop of hard links ends).
class Hdf5Tree
{
public:
    // Throws IoError when the file cannot be opened or read, LimitExceededError when a cap is exceeded,
    // OperationCancelled on cancellation. `progress` counts the objects met.
    [[nodiscard]] static Hdf5Tree read(const std::filesystem::path& file, const Hdf5Limits& limits = {},
                                       const base::CancellationToken* token = nullptr,
                                       base::ProgressCounter* progress = nullptr);

    [[nodiscard]] const std::vector<Hdf5Node>& nodes() const noexcept { return m_nodes; }
    [[nodiscard]] const Hdf5Node& root() const noexcept { return m_nodes.front(); }
    // The node at an absolute path ("/a/b"), or nullptr.
    [[nodiscard]] const Hdf5Node* find(const std::string& path) const;
    [[nodiscard]] std::uint64_t datasetCount() const noexcept;

private:
    std::vector<Hdf5Node> m_nodes;
    std::unordered_map<std::string, std::uint32_t> m_index;
};

} // namespace oor::io
