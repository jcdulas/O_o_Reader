#pragma once

#include "oor/io/files/ByteSource.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

namespace oor::io
{

// A file read on demand through one reusable buffer. Slower than MappedFile, but works where mapping does not. The
// span returned by read() is valid until the next read(). Not thread-safe.
class BufferedFile final : public ByteSource
{
public:
    explicit BufferedFile(const std::filesystem::path& path);

    [[nodiscard]] std::uint64_t size() const noexcept override { return m_size; }
    [[nodiscard]] std::span<const std::byte> read(std::uint64_t offset, std::size_t length) override;
    [[nodiscard]] bool isStable() const noexcept override { return false; }

private:
    std::filesystem::path m_path;
    std::ifstream m_stream;
    std::uint64_t m_size = 0;
    std::vector<std::byte> m_buffer;
};

} // namespace oor::io
