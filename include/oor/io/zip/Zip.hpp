#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::io
{

// A ZIP archive built in memory: the container of an Excel workbook (.xlsx), which is a ZIP of XML files. The files are
// compressed with deflate (zlib); level 0 stores them as they are. The archive has no ZIP64 extension: the whole
// archive and each file stay under 4 GiB, and it holds at most 65 535 files (UsageError past these limits).
class ZipWriter
{
public:
    // `compressionLevel`: 0 (stored) to 9; 6 is zlib's balance of size and speed.
    explicit ZipWriter(int compressionLevel = 6);

    // Adds a file; `name` is its path in the archive ("xl/workbook.xml"), with '/' between folders.
    void addFile(const std::string& name, std::string_view content);
    // The bytes of the archive: the files added, then the central directory. The writer is empty afterwards.
    [[nodiscard]] std::string finish();

private:
    struct Entry
    {
        std::string name;
        std::uint32_t crc = 0;
        std::uint32_t compressedSize = 0;
        std::uint32_t size = 0;
        std::uint32_t offset = 0;
        std::uint16_t method = 0;
    };

    int m_level;
    std::string m_bytes;
    std::vector<Entry> m_entries;
};

// A file of a ZIP archive, read back.
struct ZipFile
{
    std::string name;
    std::string content;
};

// The files of a ZIP archive (stored or deflated), in the order of its central directory. Throws IoError for an archive
// this reader does not understand. Made for the tests of what ZipWriter writes, not for any ZIP file.
[[nodiscard]] std::vector<ZipFile> readZip(std::string_view bytes);

} // namespace oor::io
