#pragma once

#include "oor/query/named/QueryValue.hpp"

#include <string>
#include <vector>

namespace oor::query
{

// What `query_catalog()` says of one parameter of a named query.
struct ParameterInfo
{
    std::string name;
    ParameterKind kind = ParameterKind::Text;
    std::string defaultText; // as the docs show it: "None", "30.0", "\"edge\""
    std::string description;
};

// The Arrow type of a column, for the catalogue.
enum class ColumnKind : std::uint8_t
{
    Int32,
    Int64,
    Float64,
    Bool,
    Text,       // utf8
    Dictionary, // dictionary<utf8> (card type names)
    IntList,    // list<int32>
    RealList,   // list<float64>
    TextList,   // list<utf8>
    Result,     // a value read from a result table: float32 for a 32-bit file, float64 for a 64-bit one (and a mix)
    Step        // the pseudo column `[step]`: the column of the steps the analysis has (time, freq, mode, loadStep)
};

[[nodiscard]] std::string_view columnKindName(ColumnKind kind) noexcept;

// One column of the result. `editable` marks the columns that `apply` can write back.
struct ColumnSpec
{
    std::string name;
    ColumnKind kind = ColumnKind::Int32;
    bool editable = false;
};

// The columns of one form of the result: a query with a `detail` or `by` parameter has one set for each value.
struct ColumnSet
{
    std::string detail; // the value of the parameter that selects it; empty for the default form
    std::vector<ColumnSpec> columns;
};

// Everything the catalogue says of a named query. Written by hand in the class of each query.
struct QueryInfo
{
    std::string name;
    std::string family; // "T1" ... "T14", "R1" ... "R10"
    std::string description;
    // The parameter whose value selects the column set ("detail", "by"); empty when the columns do not vary.
    std::string detailParameter;
    // The queries of the R families read results: they take the parameters common to the result queries, and give the
    // columns `[step]` and the complex columns of the data they read. `needsModel` is true when the query cannot run
    // on a result file alone.
    bool resultQuery = false;
    bool needsModel = false;
    // The column the `envelope` parameter of the result queries decides by (the value whose extreme over the analyses
    // is the critical one); empty when the query takes no envelope.
    std::string envelopeValue;
    // The columns that identify an item for the envelope (the step columns are added); empty: every column that is not
    // a real number, `analysis` and the provenance. `maxDisplacement` names only its subcase: its `nodeId` is the
    // answer.
    std::vector<std::string> envelopeKeys;
    std::vector<ParameterInfo> parameters;
    std::vector<ColumnSet> columnSets; // the first one is the default
    std::string defaultSort;           // in words: "loopId, position"
    // Why the query is deprecated and when it goes (a DeprecationWarning at each call from Python); empty for a
    // query that stays.
    std::string deprecation;
    [[nodiscard]] const ColumnSet& defaultColumns() const { return columnSets.front(); }
    // The column set for a value of the detail parameter (the default one when the value names none).
    [[nodiscard]] const ColumnSet& columnsFor(const std::string& detail) const;
};

// Short ways to write a ColumnSet.
class ColumnListBuilder
{
public:
    ColumnListBuilder& int32(const std::string& name, bool editable = false);
    ColumnListBuilder& int64(const std::string& name, bool editable = false);
    ColumnListBuilder& real(const std::string& name, bool editable = false);
    ColumnListBuilder& flag(const std::string& name, bool editable = false);
    ColumnListBuilder& text(const std::string& name, bool editable = false);
    ColumnListBuilder& dictionary(const std::string& name, bool editable = false);
    ColumnListBuilder& intList(const std::string& name, bool editable = false);
    ColumnListBuilder& realList(const std::string& name, bool editable = false);
    ColumnListBuilder& textList(const std::string& name, bool editable = false);
    // A value of a result table (float32 or float64 as the file is) and the pseudo column `[step]`.
    ColumnListBuilder& result(const std::string& name);
    ColumnListBuilder& results(const std::string& names);
    ColumnListBuilder& step();
    // The three columns `<prefix>` followed by each suffix: ("normal", "XYZ") gives normalX, normalY, normalZ.
    ColumnListBuilder& vector3(const std::string& prefix, const std::string& suffixes = "xyz", bool editable = false);
    // Several real columns with the same options, from a space separated list of names ("xmin ymin zmin").
    ColumnListBuilder& reals(const std::string& names, bool editable = false);
    [[nodiscard]] ColumnSet build(std::string detail = {}) const;

private:
    std::vector<ColumnSpec> m_columns;
};

[[nodiscard]] ParameterInfo parameter(std::string name, ParameterKind kind, std::string defaultText,
                                      std::string description);

} // namespace oor::query
