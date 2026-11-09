#include "oor/io/zip/Zip.hpp"

#include "oor/base/errors/Errors.hpp"

#include <limits>
#include <zlib.h>

// The ZIP layout written here (the "APPNOTE" of the ZIP format, the parts every reader knows):
//   for each file:  local file header (30 bytes + name), then the data
//   then:           one central directory header per file (46 bytes + name)
//   then:           the end of central directory record (22 bytes)
// All numbers are little-endian.

namespace oor::io
{

namespace
{

constexpr std::uint32_t LOCAL_HEADER = 0x04034b50;
constexpr std::uint32_t CENTRAL_HEADER = 0x02014b50;
constexpr std::uint32_t END_OF_DIRECTORY = 0x06054b50;
constexpr std::uint16_t VERSION = 20;        // 2.0: deflate
constexpr std::uint16_t UTF8_NAMES = 0x0800; // general purpose flag: the names are UTF-8
constexpr std::uint16_t STORED = 0;
constexpr std::uint16_t DEFLATED = 8;
constexpr std::uint16_t DOS_DATE = (0 << 9) | (1 << 5) | 1; // 1980-01-01: the files carry no meaningful date
constexpr std::uint16_t DOS_TIME = 0;

void put16(std::string& out, std::uint16_t value)
{
    out += static_cast<char>(value & 0xFF);
    out += static_cast<char>((value >> 8) & 0xFF);
}

void put32(std::string& out, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8)
        out += static_cast<char>((value >> shift) & 0xFF);
}

std::uint16_t get16(std::string_view bytes, std::size_t at)
{
    if (at + 2 > bytes.size())
        throw base::IoError("ZIP: the archive is truncated");
    return static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[at]) |
                                      (static_cast<unsigned char>(bytes[at + 1]) << 8));
}

std::uint32_t get32(std::string_view bytes, std::size_t at)
{
    return static_cast<std::uint32_t>(get16(bytes, at)) | (static_cast<std::uint32_t>(get16(bytes, at + 2)) << 16);
}

std::uint32_t checkedSize(std::size_t size, const char* what)
{
    if (size > std::numeric_limits<std::uint32_t>::max())
        throw base::UsageError(std::string("ZIP: ") + what + " is larger than 4 GiB (ZIP64 is not written)");
    return static_cast<std::uint32_t>(size);
}

// The content compressed as raw deflate data (no zlib header), as ZIP stores it.
std::string deflateRaw(std::string_view content, int level)
{
    z_stream stream{};
    if (deflateInit2(&stream, level, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        throw base::IoError("ZIP: zlib cannot start a compression");
    std::string compressed(deflateBound(&stream, static_cast<uLong>(content.size())), '\0');
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(content.data()));
    stream.avail_in = static_cast<uInt>(content.size());
    stream.next_out = reinterpret_cast<Bytef*>(compressed.data());
    stream.avail_out = static_cast<uInt>(compressed.size());
    const int status = deflate(&stream, Z_FINISH);
    compressed.resize(stream.total_out);
    deflateEnd(&stream);
    if (status != Z_STREAM_END)
        throw base::IoError("ZIP: zlib could not compress a file");
    return compressed;
}

std::string inflateRaw(std::string_view compressed, std::size_t size)
{
    z_stream stream{};
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
        throw base::IoError("ZIP: zlib cannot start a decompression");
    std::string content(size, '\0');
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
    stream.avail_in = static_cast<uInt>(compressed.size());
    stream.next_out = reinterpret_cast<Bytef*>(content.data());
    stream.avail_out = static_cast<uInt>(content.size());
    const int status = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    if (status != Z_STREAM_END || stream.total_out != size)
        throw base::IoError("ZIP: a file does not decompress to its size");
    return content;
}

} // namespace

ZipWriter::ZipWriter(int compressionLevel) : m_level(compressionLevel)
{
    if (compressionLevel < 0 || compressionLevel > 9)
        throw base::UsageError("ZIP: the compression level is " + std::to_string(compressionLevel) +
                               ", it must be 0 to 9");
}

void ZipWriter::addFile(const std::string& name, std::string_view content)
{
    if (m_entries.size() >= 0xFFFF)
        throw base::UsageError("ZIP: an archive holds at most 65 535 files");
    Entry entry;
    entry.name = name;
    entry.size = checkedSize(content.size(), "a file");
    entry.crc = static_cast<std::uint32_t>(
        crc32(0L, reinterpret_cast<const Bytef*>(content.data()), static_cast<uInt>(content.size())));
    entry.offset = checkedSize(m_bytes.size(), "the archive");
    const std::string compressed = m_level > 0 ? deflateRaw(content, m_level) : std::string();
    entry.method = m_level > 0 ? DEFLATED : STORED;
    const std::string_view data = m_level > 0 ? std::string_view(compressed) : content;
    entry.compressedSize = checkedSize(data.size(), "a compressed file");

    put32(m_bytes, LOCAL_HEADER);
    put16(m_bytes, VERSION);
    put16(m_bytes, UTF8_NAMES);
    put16(m_bytes, entry.method);
    put16(m_bytes, DOS_TIME);
    put16(m_bytes, DOS_DATE);
    put32(m_bytes, entry.crc);
    put32(m_bytes, entry.compressedSize);
    put32(m_bytes, entry.size);
    put16(m_bytes, static_cast<std::uint16_t>(entry.name.size()));
    put16(m_bytes, 0); // no extra field
    m_bytes += entry.name;
    m_bytes += data;
    m_entries.push_back(std::move(entry));
}

std::string ZipWriter::finish()
{
    const std::uint32_t directoryOffset = checkedSize(m_bytes.size(), "the archive");
    for (const Entry& entry : m_entries)
    {
        put32(m_bytes, CENTRAL_HEADER);
        put16(m_bytes, VERSION); // made by
        put16(m_bytes, VERSION); // needed to extract
        put16(m_bytes, UTF8_NAMES);
        put16(m_bytes, entry.method);
        put16(m_bytes, DOS_TIME);
        put16(m_bytes, DOS_DATE);
        put32(m_bytes, entry.crc);
        put32(m_bytes, entry.compressedSize);
        put32(m_bytes, entry.size);
        put16(m_bytes, static_cast<std::uint16_t>(entry.name.size()));
        put16(m_bytes, 0); // extra field
        put16(m_bytes, 0); // comment
        put16(m_bytes, 0); // disk
        put16(m_bytes, 0); // internal attributes
        put32(m_bytes, 0); // external attributes
        put32(m_bytes, entry.offset);
        m_bytes += entry.name;
    }
    const std::uint32_t directorySize = checkedSize(m_bytes.size() - directoryOffset, "the central directory");
    put32(m_bytes, END_OF_DIRECTORY);
    put16(m_bytes, 0); // this disk
    put16(m_bytes, 0); // the disk of the directory
    put16(m_bytes, static_cast<std::uint16_t>(m_entries.size()));
    put16(m_bytes, static_cast<std::uint16_t>(m_entries.size()));
    put32(m_bytes, directorySize);
    put32(m_bytes, directoryOffset);
    put16(m_bytes, 0); // no comment
    m_entries.clear();
    return std::move(m_bytes);
}

std::vector<ZipFile> readZip(std::string_view bytes)
{
    if (bytes.size() < 22)
        throw base::IoError("ZIP: the archive is too short");
    const std::size_t end = bytes.size() - 22; // no comment: the record is the last 22 bytes
    if (get32(bytes, end) != END_OF_DIRECTORY)
        throw base::IoError("ZIP: no end of central directory record");
    const std::uint16_t count = get16(bytes, end + 10);
    std::size_t at = get32(bytes, end + 16);
    std::vector<ZipFile> files;
    for (std::uint16_t index = 0; index < count; ++index)
    {
        if (get32(bytes, at) != CENTRAL_HEADER)
            throw base::IoError("ZIP: a central directory header is damaged");
        const std::uint16_t method = get16(bytes, at + 10);
        const std::uint32_t crc = get32(bytes, at + 16);
        const std::uint32_t compressedSize = get32(bytes, at + 20);
        const std::uint32_t size = get32(bytes, at + 24);
        const std::uint16_t nameLength = get16(bytes, at + 28);
        const std::uint16_t extraLength = get16(bytes, at + 30);
        const std::uint16_t commentLength = get16(bytes, at + 32);
        const std::uint32_t offset = get32(bytes, at + 42);
        ZipFile file;
        file.name = std::string(bytes.substr(at + 46, nameLength));
        at += 46 + nameLength + extraLength + commentLength;

        if (get32(bytes, offset) != LOCAL_HEADER)
            throw base::IoError("ZIP: the local header of " + file.name + " is damaged");
        const std::size_t data = offset + 30 + get16(bytes, offset + 26) + get16(bytes, offset + 28);
        if (data + compressedSize > bytes.size())
            throw base::IoError("ZIP: the data of " + file.name + " run past the archive");
        const std::string_view stored = bytes.substr(data, compressedSize);
        if (method == STORED)
            file.content = std::string(stored);
        else if (method == DEFLATED)
            file.content = inflateRaw(stored, size);
        else
            throw base::IoError("ZIP: " + file.name + " uses a compression method other than deflate");
        if (crc32(0L, reinterpret_cast<const Bytef*>(file.content.data()), static_cast<uInt>(file.content.size())) !=
            crc)
            throw base::IoError("ZIP: the checksum of " + file.name + " is wrong");
        files.push_back(std::move(file));
    }
    return files;
}

} // namespace oor::io
