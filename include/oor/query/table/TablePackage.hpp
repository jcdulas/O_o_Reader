#pragma once

#include "oor/query/table/QueryTable.hpp"

#include <cstdint>
#include <vector>

namespace oor::query
{

// A query table as one block of bytes that keeps its Arrow layout exactly: the "table package", made to hand a table
// from this WebAssembly module to another one (the O_o_Solver module), which cannot read this module's memory. The
// receiver rebuilds the Arrow C Data Interface structures over the bytes, so it sees the same formats (int32,
// dictionary of texts, fixed-size lists, ...), the same buffers and the same schema metadata (the data contract
// version) as an Arrow consumer of ArrowExport would.
//
// Layout (all integers little-endian), described in docs/table-package.md:
//
//   "OOTP"  u32 version (1)  u64 headerBytes
//   header: the root node (the table: a struct array with one child per column), written depth first
//   padding to a multiple of 8 bytes
//   the buffers, each starting at a multiple of 8 bytes from the start of this area
//
//   node: string name, string format, i64 flags, u32 metadataCount, (string key, string value) * metadataCount,
//         i64 length, i64 nullCount, i64 offset,
//         u32 bufferCount, (u64 start, u64 byteLength) * bufferCount   (start = 2^64 - 1 for a null buffer),
//         u32 childCount, node * childCount,
//         u8 hasDictionary, node (when hasDictionary is 1)
//   string: u32 byteLength, UTF-8 bytes
[[nodiscard]] std::vector<std::uint8_t> packTable(const QueryTable& table);

} // namespace oor::query
