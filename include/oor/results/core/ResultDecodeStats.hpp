#pragma once

#include <atomic>
#include <cstdint>

namespace oor::results
{

// What decoding has done since the process started (or since the last reset): counters for the tests that prove
// "nothing is decoded before it is asked for" and "a filter decodes only what it selects", and for the performance
// notes. Updated with relaxed atomic adds, once per chunk of work.
struct ResultDecodeStats
{
    std::atomic<std::uint64_t> tablesDecoded{0};  // decodes started (one per table that was loaded)
    std::atomic<std::uint64_t> entriesRead{0};    // blocks of results (idents and their data) a decode looked at
    std::atomic<std::uint64_t> rowsDecoded{0};    // rows (grid points) whose values were converted and stored
    std::atomic<std::uint64_t> sourceBytes{0};    // bytes of the file that a decode read the values of
    std::atomic<std::uint64_t> bytesAllocated{0}; // bytes allocated for decoded values
    std::atomic<std::uint64_t> idReads{0};        // IDs read to resolve a filter or to label the rows
    std::atomic<std::uint64_t> domainScanRows{
        0}; // rows of a DOMAIN_ID column scanned for want of an /INDEX table (HDF5)

    void reset() noexcept
    {
        tablesDecoded = 0;
        entriesRead = 0;
        rowsDecoded = 0;
        sourceBytes = 0;
        bytesAllocated = 0;
        idReads = 0;
        domainScanRows = 0;
    }
};

[[nodiscard]] ResultDecodeStats& decodeStats() noexcept;

} // namespace oor::results
