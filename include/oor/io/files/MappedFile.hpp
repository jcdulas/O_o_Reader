#pragma once

#include "oor/base/platform/MemoryMapping.hpp"
#include "oor/io/files/ByteSource.hpp"

#include <filesystem>

namespace oor::io
{

// A file mapped in memory: the fastest source, and the one used for decks and result files. Spans stay valid for the
// life of the object. Mapping a file that another process truncates is undefined (a limit of memory mapping).
class MappedFile final : public ByteSource
{
public:
    explicit MappedFile(const std::filesystem::path& path);

    [[nodiscard]] std::uint64_t size() const noexcept override { return m_mapping.size(); }
    [[nodiscard]] std::span<const std::byte> read(std::uint64_t offset, std::size_t length) override;
    [[nodiscard]] bool isStable() const noexcept override { return true; }

    // The whole file at once.
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return m_mapping.bytes(); }

private:
    base::MemoryMapping m_mapping;
};

} // namespace oor::io
