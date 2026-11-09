#pragma once

namespace oor::model
{

// The version of what the library exports to companion packages: the name, element type and
// shape of every column of every card table, the CSR form of variable-length columns, and the card keys, the columns of
// the tables of the named queries, and the layout of each result table and named result a companion package may hand
// back (`result_tables`, `named_results`). It changes whenever those layouts or the meanings of their values change.
// Version 75 uses descriptive element types, identical in model records and result containers.
inline constexpr int DATA_CONTRACT_VERSION = 75;

} // namespace oor::model
