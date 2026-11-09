#pragma once

#include "oor/base/threading/CancellationToken.hpp"
#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/io/hdf5/Hdf5Limits.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace oor::io
{

enum class Hdf5ValueKind
{
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float32,
    Float64,
    Bool,        // stored as the enum {FALSE = 0, TRUE = 1} over one byte, as h5py does
    FixedString, // `stringBytes` bytes per string, padded with NUL
    VarString    // variable-length UTF-8 strings, in `strings`
};

// An array to write: one element type, a shape and the values. The values are borrowed: the memory behind `data` must
// stay valid until the write returns (the Python layer keeps the NumPy arrays alive).
struct Hdf5WriteArray
{
    Hdf5ValueKind kind = Hdf5ValueKind::Int64;
    std::size_t stringBytes = 0;      // FixedString only
    bool utf8 = false;                // FixedString only: the character set recorded in the file
    std::vector<std::uint64_t> shape; // empty: a scalar
    const void* data = nullptr;       // product(shape) values in C order (not used by VarString)
    std::vector<std::string> strings; // VarString: product(shape) strings

    // Bytes of one value (the pointer size for VarString).
    [[nodiscard]] std::size_t itemSize() const noexcept;
    [[nodiscard]] std::uint64_t elementCount() const noexcept;
};

// A member of a compound dataset: a leaf (an array whose first dimension is the number of rows; the other dimensions
// make the member an array type) or a nested compound (`members` not empty).
struct Hdf5WriteColumn
{
    std::string name;
    Hdf5WriteArray array;
    std::vector<Hdf5WriteColumn> members;
};

struct Hdf5WriteAttribute
{
    std::string name;
    Hdf5WriteArray array;
};

struct Hdf5WriteDataset
{
    std::string path;
    // A plain dataset when `columns` is empty, else a compound dataset of one dimension.
    Hdf5WriteArray array;
    std::vector<Hdf5WriteColumn> columns;
    // Rows of one chunk (the other dimensions are whole). 0: contiguous, unless a filter is asked for (then the chunk
    // is about one megabyte).
    std::uint64_t chunkRows = 0;
    int deflate = 0; // 0 (none) to 9
    bool shuffle = false;
    // The first dimension can grow (maximum size unlimited, as in the the solver files): the dataset is chunked, and the chunk
    // may be larger than the rows written.
    bool resizable = false;
    // A compound dataset is laid out as a C struct (each member at a multiple of its alignment, the size a multiple of
    // the largest: a member after a 4-byte string starts at the next multiple of 8, as in the the solver files) instead of
    // packed. The padding bytes are zero.
    bool alignMembers = false;
    std::vector<Hdf5WriteAttribute> attributes;
};

struct Hdf5WriteGroup
{
    std::string path; // "/" for the root group
    std::vector<Hdf5WriteAttribute> attributes;
};

struct Hdf5WriteLink
{
    std::string path;
    // Soft link: a path in the file. Hard link: the path of an object already written. External link: `file` is not
    // empty and `target` is a path in that file (the link is written, never followed).
    std::string target;
    std::string file;
    bool hard = false;
};

struct Hdf5WriteRequest
{
    std::vector<Hdf5WriteGroup> groups;
    std::vector<Hdf5WriteDataset> datasets;
    std::vector<Hdf5WriteLink> links;
};

// Writes the file: groups (and the groups on the paths of datasets and links), datasets, then links. The file is
// written to a temporary file in the same folder and renamed over the target when it is complete, so a failure, a
// cancellation or a crash never leaves a half-written file at `target` (nor an earlier file replaced). Throws
// UsageError for a request that makes no sense (shapes that disagree, a bad path, a compression level out of range),
// LimitExceededError for a cap, IoError for a library or disk failure, OperationCancelled.
void writeHdf5(const std::filesystem::path& target, const Hdf5WriteRequest& request, const Hdf5Limits& limits = {},
               const base::CancellationToken* token = nullptr, base::ProgressCounter* progress = nullptr);

} // namespace oor::io
