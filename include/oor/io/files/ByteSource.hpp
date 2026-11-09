#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace oor::io
{

// Read-only access to the bytes of a file. Two implementations: MappedFile (the whole file mapped in memory, spans
// stay valid for the life of the source) and BufferedFile (reads on demand, a span stays valid until the next read).
// Decks and result files are read through this interface so that the parsers do not care which one they get.
class ByteSource
{
public:
    virtual ~ByteSource() = default;

    [[nodiscard]] virtual std::uint64_t size() const noexcept = 0;

    // Up to `length` bytes starting at `offset`; fewer at the end of the file, none past it. Throws IoError when
    // the file cannot be read.
    [[nodiscard]] virtual std::span<const std::byte> read(std::uint64_t offset, std::size_t length) = 0;

    // True when read() spans stay valid until the source is destroyed (a mapped file): callers may keep them.
    [[nodiscard]] virtual bool isStable() const noexcept = 0;
};

// Opens `path` as a mapped file, or as a buffered file when it cannot be mapped (a network drive, a pipe).
// Throws IoError when the file cannot be opened at all.
[[nodiscard]] std::unique_ptr<ByteSource> openByteSource(const std::filesystem::path& path, bool preferMapped = true);

} // namespace oor::io
