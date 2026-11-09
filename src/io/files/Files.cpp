#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/io/files/BufferedFile.hpp"
#include "oor/io/files/ByteSource.hpp"
#include "oor/io/files/MappedFile.hpp"

#include <algorithm>

namespace oor::io
{

MappedFile::MappedFile(const std::filesystem::path& path) : m_mapping(base::MemoryMapping::openReadOnly(path)) {}

std::span<const std::byte> MappedFile::read(std::uint64_t offset, std::size_t length)
{
    if (offset >= m_mapping.size())
        return {};
    const std::size_t available = static_cast<std::size_t>(m_mapping.size() - offset);
    return {m_mapping.data() + offset, std::min(length, available)};
}

BufferedFile::BufferedFile(const std::filesystem::path& path) : m_path(path), m_stream(path, std::ios::binary)
{
    if (!m_stream)
        throw base::IoError("cannot open file: " + base::pathToUtf8(path));
    m_stream.seekg(0, std::ios::end);
    const std::streamoff end = m_stream.tellg();
    if (end < 0)
        throw base::IoError("cannot get the size of file: " + base::pathToUtf8(path));
    m_size = static_cast<std::uint64_t>(end);
}

std::span<const std::byte> BufferedFile::read(std::uint64_t offset, std::size_t length)
{
    if (offset >= m_size)
        return {};
    const std::size_t wanted = static_cast<std::size_t>(std::min<std::uint64_t>(length, m_size - offset));
    m_buffer.resize(wanted);
    m_stream.clear();
    m_stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    m_stream.read(reinterpret_cast<char*>(m_buffer.data()), static_cast<std::streamsize>(wanted));
    const std::size_t got = static_cast<std::size_t>(m_stream.gcount());
    if (got != wanted)
        throw base::IoError("cannot read file: " + base::pathToUtf8(m_path));
    return {m_buffer.data(), got};
}

std::unique_ptr<ByteSource> openByteSource(const std::filesystem::path& path, bool preferMapped)
{
    if (preferMapped)
    {
        try
        {
            return std::make_unique<MappedFile>(path);
        }
        catch (const base::IoError&)
        {
            // Fall through: a file that cannot be mapped may still be readable.
        }
    }
    return std::make_unique<BufferedFile>(path);
}

} // namespace oor::io
