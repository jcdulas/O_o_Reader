#include "oor/io/hdf5/Hdf5GlobalHeapCheck.hpp"

#include "oor/base/errors/OorError.hpp"
#include "oor/io/files/MappedFile.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <system_error>

namespace oor::io
{

namespace
{

// The superblock signature, at byte 0, 512, 1024, 2048, ... of the file (HDF5 file format specification, II.A).
constexpr std::array<unsigned char, 8> SUPERBLOCK_SIGNATURE = {0x89, 'H', 'D', 'F', '\r', '\n', 0x1a, '\n'};
constexpr std::size_t DEFAULT_LENGTH_BYTES = 8;

// A little-endian unsigned integer of `size` bytes (1 to 8) at `at`.
std::uint64_t loadLittle(std::span<const std::byte> bytes, std::size_t at, std::size_t size)
{
    std::uint64_t value = 0;
    for (std::size_t index = size; index-- > 0;)
        value = (value << 8) | std::to_integer<std::uint64_t>(bytes[at + index]);
    return value;
}

// The "size of lengths" of the superblock (bytes 14 of versions 0 and 1, byte 10 of versions 2 and 3): the width of
// the collection and object sizes of the global heap. 8 when the superblock cannot be found or reads oddly.
std::size_t lengthBytesOf(std::span<const std::byte> bytes)
{
    for (std::size_t at = 0; at + 16 <= bytes.size(); at = at == 0 ? 512 : at * 2)
    {
        bool matches = true;
        for (std::size_t index = 0; index < SUPERBLOCK_SIGNATURE.size() && matches; ++index)
            matches = std::to_integer<unsigned char>(bytes[at + index]) == SUPERBLOCK_SIGNATURE[index];
        if (!matches)
            continue;
        const auto version = std::to_integer<unsigned>(bytes[at + 8]);
        const auto size = std::to_integer<std::size_t>(bytes[at + (version <= 1 ? 14 : 10)]);
        return size == 2 || size == 4 || size == 8 ? size : DEFAULT_LENGTH_BYTES;
    }
    return DEFAULT_LENGTH_BYTES;
}

// The first problem of the collection whose signature is at `at`, walked as libhdf5 walks it when it loads it
// (H5HG__cache_heap_deserialize): from the end of the header, each object is an index of 2 bytes, a reference count
// of 2, 4 reserved bytes and a size of `lengthBytes`, then its data padded to 8 bytes; the object of index 0 is the
// free space, whose size includes its header and is not padded; a tail too short for an object header is free space.
std::optional<std::string> collectionProblem(std::span<const std::byte> bytes, std::size_t at, std::size_t lengthBytes)
{
    const std::size_t headerBytes = 8 + lengthBytes;
    const std::uint64_t declared = loadLittle(bytes, at + 8, lengthBytes);
    const std::string where = "the global heap collection at byte " + std::to_string(at);
    if (declared < headerBytes || declared > bytes.size() - at)
        return where + " declares " + std::to_string(declared) + " bytes, " +
               (declared < headerBytes ? "less than its header"
                                       : "past the end of the file (" + std::to_string(bytes.size()) + " bytes)");
    const std::size_t end = at + static_cast<std::size_t>(declared);
    const std::size_t objectHeaderBytes = 8 + lengthBytes;
    std::size_t position = at + headerBytes;
    while (position < end)
    {
        const std::size_t left = end - position;
        if (left < objectHeaderBytes)
            break;
        const auto index = static_cast<unsigned>(loadLittle(bytes, position, 2));
        const std::uint64_t size = loadLittle(bytes, position + 8, lengthBytes);
        // compared before any sum: a size near 2^64 would wrap the padding and the step
        if (size > left)
            return where + ": the object " + std::to_string(index) + " at byte " + std::to_string(position) + " has " +
                   std::to_string(size) + " bytes, past the end of the collection";
        const std::uint64_t step = index == 0 ? size : objectHeaderBytes + (size + 7) / 8 * 8;
        if (step == 0)
            return where + ": the free-space object at byte " + std::to_string(position) +
                   " has a size of 0, which libhdf5 would read forever";
        if (step > left)
            return where + ": the object " + std::to_string(index) + " at byte " + std::to_string(position) +
                   " runs past the end of the collection";
        position += static_cast<std::size_t>(step);
    }
    return std::nullopt;
}

} // namespace

std::optional<std::string> findGlobalHeapProblem(std::span<const std::byte> bytes)
{
    const std::size_t lengthBytes = lengthBytesOf(bytes);
    const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    for (std::size_t at = text.find("GCOL"); at != std::string_view::npos; at = text.find("GCOL", at + 1))
    {
        // signature, version 1, three reserved zero bytes, the collection size: anything else is not a collection
        // (libhdf5 refuses to load a damaged header with an error, it does not loop on it)
        if (at + 8 + lengthBytes > bytes.size())
            continue;
        if (std::to_integer<unsigned>(bytes[at + 4]) != 1 || std::to_integer<unsigned>(bytes[at + 5]) != 0 ||
            std::to_integer<unsigned>(bytes[at + 6]) != 0 || std::to_integer<unsigned>(bytes[at + 7]) != 0)
            continue;
        if (std::optional<std::string> problem = collectionProblem(bytes, at, lengthBytes))
            return problem;
    }
    return std::nullopt;
}

std::optional<std::string> findGlobalHeapProblem(const std::filesystem::path& file)
{
    std::error_code error;
    if (std::filesystem::file_size(file, error) == 0 || error)
        return std::nullopt;
    try
    {
        const MappedFile mapped(file);
        return findGlobalHeapProblem(mapped.bytes());
    }
    catch (const base::OorError&)
    {
        return std::nullopt;
    }
}

} // namespace oor::io
