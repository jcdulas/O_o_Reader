#pragma once

#include <cstddef>
#include <filesystem>
#include <span>

namespace oor::base
{

// A read-only view of a whole file in memory (CreateFileMapping on Windows, mmap elsewhere). This is the only place
// that talks to the operating system for file access. Move-only; the view ends with the object.
class MemoryMapping
{
public:
    MemoryMapping() = default;
    MemoryMapping(MemoryMapping&& other) noexcept;
    MemoryMapping& operator=(MemoryMapping&& other) noexcept;
    MemoryMapping(const MemoryMapping&) = delete;
    MemoryMapping& operator=(const MemoryMapping&) = delete;
    ~MemoryMapping();

    // Throws IoError (with the path in the message) when the file cannot be opened or mapped. An empty file gives an
    // empty mapping.
    [[nodiscard]] static MemoryMapping openReadOnly(const std::filesystem::path& path);

    [[nodiscard]] const std::byte* data() const noexcept { return m_data; }
    [[nodiscard]] std::size_t size() const noexcept { return m_size; }
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return {m_data, m_size}; }

    // Asks the system to read these ranges of a mapping ahead, in large reads, before they are touched: a page that is
    // read later is then in memory, where touching the formats of a mapping one by one reads the file a little at a
    // time. A hint only (nothing happens where the system does not offer it); the ranges stay valid while the mapping
    // lives.
    static void prefetch(std::span<const std::span<const std::byte>> ranges) noexcept;

private:
    void release() noexcept;

    const std::byte* m_data = nullptr;
    std::size_t m_size = 0;
    void* m_fileHandle = nullptr;    // Windows: HANDLE of the file
    void* m_mappingHandle = nullptr; // Windows: HANDLE of the mapping object
};

} // namespace oor::base
