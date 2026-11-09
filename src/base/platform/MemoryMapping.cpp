#include "oor/base/platform/MemoryMapping.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#endif

namespace oor::base
{

#ifdef _WIN32
void MemoryMapping::prefetch(std::span<const std::span<const std::byte>> ranges) noexcept
{
    // PrefetchVirtualMemory is in Windows 8 and later: looked up, so that an older system just skips the hint
    struct Range
    {
        void* address;
        SIZE_T bytes;
    };
    using Prefetch = BOOL(WINAPI*)(HANDLE, ULONG_PTR, Range*, ULONG);
    static const Prefetch PREFETCH_VIRTUAL_MEMORY = []
    {
        const HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
        return kernel != nullptr ? reinterpret_cast<Prefetch>(
                                       reinterpret_cast<void*>(GetProcAddress(kernel, "PrefetchVirtualMemory")))
                                 : nullptr;
    }();
    if (PREFETCH_VIRTUAL_MEMORY == nullptr || ranges.empty())
        return;
    std::vector<Range> entries;
    entries.reserve(ranges.size());
    for (const std::span<const std::byte> range : ranges)
        if (!range.empty())
            entries.push_back(Range{const_cast<std::byte*>(range.data()), range.size()});
    if (!entries.empty())
        (void)PREFETCH_VIRTUAL_MEMORY(GetCurrentProcess(), entries.size(), entries.data(), 0);
}
#else
void MemoryMapping::prefetch(std::span<const std::span<const std::byte>> ranges) noexcept
{
    const auto page = static_cast<std::uintptr_t>(sysconf(_SC_PAGESIZE));
    for (const std::span<const std::byte> range : ranges)
    {
        if (range.empty())
            continue;
        const auto start = reinterpret_cast<std::uintptr_t>(range.data()) / page * page;
        const auto end = reinterpret_cast<std::uintptr_t>(range.data()) + range.size();
        (void)madvise(reinterpret_cast<void*>(start), end - start, MADV_WILLNEED);
    }
}
#endif

MemoryMapping::MemoryMapping(MemoryMapping&& other) noexcept
    : m_data(std::exchange(other.m_data, nullptr)),
      m_size(std::exchange(other.m_size, 0)),
      m_fileHandle(std::exchange(other.m_fileHandle, nullptr)),
      m_mappingHandle(std::exchange(other.m_mappingHandle, nullptr))
{
}

MemoryMapping& MemoryMapping::operator=(MemoryMapping&& other) noexcept
{
    if (this != &other)
    {
        release();
        m_data = std::exchange(other.m_data, nullptr);
        m_size = std::exchange(other.m_size, 0);
        m_fileHandle = std::exchange(other.m_fileHandle, nullptr);
        m_mappingHandle = std::exchange(other.m_mappingHandle, nullptr);
    }
    return *this;
}

MemoryMapping::~MemoryMapping()
{
    release();
}

#ifdef _WIN32

void MemoryMapping::release() noexcept
{
    if (m_data != nullptr)
        UnmapViewOfFile(m_data);
    if (m_mappingHandle != nullptr)
        CloseHandle(m_mappingHandle);
    if (m_fileHandle != nullptr)
        CloseHandle(m_fileHandle);
    m_data = nullptr;
    m_size = 0;
    m_fileHandle = nullptr;
    m_mappingHandle = nullptr;
}

MemoryMapping MemoryMapping::openReadOnly(const std::filesystem::path& path)
{
    const std::string shownPath = pathToUtf8(path);
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        throw IoError("cannot open file: " + shownPath);
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(file, &fileSize))
    {
        CloseHandle(file);
        throw IoError("cannot get the size of file: " + shownPath);
    }
    MemoryMapping mapping;
    if (fileSize.QuadPart == 0)
    {
        CloseHandle(file);
        return mapping;
    }
    HANDLE mappingObject = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (mappingObject == nullptr)
    {
        CloseHandle(file);
        throw IoError("cannot map file: " + shownPath);
    }
    void* view = MapViewOfFile(mappingObject, FILE_MAP_READ, 0, 0, 0);
    if (view == nullptr)
    {
        CloseHandle(mappingObject);
        CloseHandle(file);
        throw IoError("cannot map file: " + shownPath);
    }
    mapping.m_data = static_cast<const std::byte*>(view);
    mapping.m_size = static_cast<std::size_t>(fileSize.QuadPart);
    mapping.m_fileHandle = file;
    mapping.m_mappingHandle = mappingObject;
    return mapping;
}

#else

void MemoryMapping::release() noexcept
{
    if (m_data != nullptr)
        munmap(const_cast<std::byte*>(m_data), m_size);
    m_data = nullptr;
    m_size = 0;
}

MemoryMapping MemoryMapping::openReadOnly(const std::filesystem::path& path)
{
    const std::string shownPath = pathToUtf8(path);
    const int file = open(path.c_str(), O_RDONLY);
    if (file < 0)
        throw IoError("cannot open file: " + shownPath);
    struct stat status;
    if (fstat(file, &status) != 0)
    {
        close(file);
        throw IoError("cannot get the size of file: " + shownPath);
    }
    MemoryMapping mapping;
    if (status.st_size == 0)
    {
        close(file);
        return mapping;
    }
    void* view = mmap(nullptr, static_cast<std::size_t>(status.st_size), PROT_READ, MAP_PRIVATE, file, 0);
    close(file); // the mapping stays valid after the descriptor is closed
    if (view == MAP_FAILED)
        throw IoError("cannot map file: " + shownPath);
    mapping.m_data = static_cast<const std::byte*>(view);
    mapping.m_size = static_cast<std::size_t>(status.st_size);
    return mapping;
}

#endif

} // namespace oor::base
