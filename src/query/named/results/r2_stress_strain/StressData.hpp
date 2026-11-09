#pragma once

#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"

#include <memory>
#include <string>
#include <vector>

namespace oor::query
{

// The stress or strain components a call asked for, from the providers in order, with von Mises, the principal values
// and the maximum shear completed where a table did not store them.
struct StressData
{
    ElementFlatSet set;
    std::vector<ScalarCompletion> completions; // one per flat of the set
    std::vector<ProviderAttempt> attempts;
    std::vector<std::int32_t> missing; // elements the call named that no way gave
    bool strain = false;
    ElementRequest request;
};

// Runs the chain of the stress (or strain) components. The way a missing quantity is handled follows `onMissing`: with
// "raise" nothing at all, or an element the call named that has no value, is a QuantityUnavailableError that lists the
// ways tried; "null" and "skip" go on with what was found.
[[nodiscard]] StressData resolveStress(const ResultQueryContext& context, const ElementRequest& request, bool strain);

// The block of rows of one flat for `stressTable` and `strainTable`.
[[nodiscard]] RowBlock stressBlock(const ResultQueryContext& context, const ElementFlat& flat,
                                   const ScalarCompletion& completion, std::int16_t provider, bool strain);

// Records in the provenance of a table made from the rows of `data` (an envelope, a ranking, a group) which providers
// the rows it summarises came from: each provider with the number of rows it gave and the `columns` the summary has of
// them.
void noteInputs(RowAccumulator& accumulator, const StressData& data, const std::vector<std::string>& columns);

// The names of the value fields a `quantity` argument may name (`vonMises`, `oxx`, `o1`, ...) for stress or strain.
[[nodiscard]] bool isStressQuantity(const std::string& quantity, bool strain);

// The values of a quantity in a flat: the field of that name, or for `maxPrincipal` the field o1 and for
// `minPrincipal` the smallest principal value: o3 for a solid, o2 for the others (e1, e2, e3 for strains).
[[nodiscard]] const std::vector<double>& quantityOf(const ElementFlat& flat, const std::string& quantity);

// The element rows (the unified columns, with `native` the tables of one element type as the file has them).
[[nodiscard]] std::shared_ptr<const QueryTable> stressTable(const ResultQueryContext& context, const QueryInfo& info,
                                                            bool strain);

// The parameters and columns of `stressTable` and `strainTable` (the same but for the names of the columns).
[[nodiscard]] QueryInfo stressTableInfo(bool strain);

// The largest and the smallest value of a quantity over everything that is not asked to be kept apart, for each
// element: the rows of an element over the subcases, steps, layers and locations (`perSubcase` keeps the subcases
// apart).
struct ElementExtreme
{
    std::size_t set = 0;
    std::int32_t eid = 0;
    std::string card;
    std::int32_t subcase = 0; // the subcase of the group (`perSubcase`); 0 otherwise
    double maxValue = 0.0;
    std::size_t maxSet = 0; // the result set where the largest value is
    std::int32_t maxSubcase = 0;
    double maxStep = 0.0;
    std::int32_t maxLayer = 0;
    std::string maxLocation;
    double minValue = 0.0;
    std::size_t minSet = 0;
    std::int32_t minSubcase = 0;
    double minStep = 0.0;
    std::int32_t minLayer = 0;
    std::string minLocation;
    Precision precision = Precision::Double;
};

// The extremes of `quantity` of every element that has a value (complex flats and rows without a value are left out),
// in the order of the elements, then of the subcases.
[[nodiscard]] std::vector<ElementExtreme> elementExtremes(const StressData& data, const std::string& quantity,
                                                          bool perSubcase);

// The extremes of a stress (or strain) for each property or material: the query behind `stressByProperty` and
// `stressByMaterial`.
[[nodiscard]] std::shared_ptr<const QueryTable> stressByGroup(const ResultQueryContext& context, const QueryInfo& info,
                                                              bool byMaterial);
[[nodiscard]] QueryInfo stressByGroupInfo(bool byMaterial);

} // namespace oor::query
