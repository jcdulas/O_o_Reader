#pragma once

#include "oor/query/providers/quantities/ElementRows.hpp"
#include "oor/query/providers/quantities/QuantityProvider.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace oor::query
{

using ElementProvider = QuantityProvider<ElementRequest, ElementFlatSet>;
using ElementChain = ProviderChain<ElementRequest, ElementFlatSet>;

// The kinds of element the unified stress, strain and force tables give rows for.
enum class ElementFamily : std::uint8_t
{
    Shell, // CQUAD4, CTRIA3, CQUAD8, CTRIA6, CQUADR, CTRIAR (the plain tables; the laminates have their own)
    Solid, // CTETRA, CPENTA, CHEXA, CPYRAM
    Rod,   // CROD, CONROD, CTUBE
    Shear, // CSHEAR
    Bar,   // CBAR: the ends A and B
    Beam,  // CBEAM: the stations
    Other  // not in the unified tables
};

[[nodiscard]] ElementFamily familyOfContainer(const std::string& elementName) noexcept;

// The value fields of a stress flat (the columns of `stressTable`) and of a strain flat (those of `strainTable`).
[[nodiscard]] const std::vector<std::string>& stressFields();
[[nodiscard]] const std::vector<std::string>& strainFields();

// A flat of `rows` rows with every value field NaN (a field no provider fills stays null).
[[nodiscard]] ElementFlat newFlat(std::size_t rows, bool strain);

// The rows of one stress or strain table as a flat, with the fields the family holds filled from the table's components
// and the others NaN. Nothing when the table is not of a family of the unified tables, or the request keeps no row of
// it.
[[nodiscard]] std::optional<ElementFlat> flattenStressTable(const ResultQueryContext& context, const TableRef& ref,
                                                            bool strain, const ElementRequest& request);

// Fills von Mises, the principal values and the maximum shear that a flat does not hold, from its components (the
// quantity the table stores is never recomputed). Returns the number of rows that got each, and what was computed in
// each row, as bits: the text of the `source` column is made from them only when a table asks for it (most rows of a
// large table share one).
struct ScalarCompletion
{
    static constexpr std::uint8_t VON_MISES = 1;
    static constexpr std::uint8_t PRINCIPAL = 2;
    static constexpr std::uint8_t MAX_SHEAR = 4;

    std::int64_t vonMises = 0;
    std::int64_t principal = 0;
    std::int64_t maxShear = 0;
    bool strain = false;
    std::vector<std::uint8_t> computed; // one per row: VON_MISES | PRINCIPAL | MAX_SHEAR, 0 for none

    // What was computed in a row, for the `source` column ("vonMises, maxShear computed from the components"); empty
    // for none.
    [[nodiscard]] std::string note(std::size_t row) const;
    // The same text for the bits of a row.
    [[nodiscard]] static std::string noteOf(std::uint8_t bits, bool strain);
};
[[nodiscard]] ScalarCompletion completeScalars(ElementFlat& flat, bool strain);

// The value fields of one row of a stress or strain flat, in the order of stressFields() (strainFields()).
inline constexpr std::size_t STRESS_FIELD_COUNT = 14;
using StressRow = std::array<double, STRESS_FIELD_COUNT>;
// What completeScalars does to one row of a shell (`solid` false) or a solid: the bits of what it computes and, with
// `values`, the values it fills (without, the row is left as it is: the bits alone, which depend only on the fields
// that are NaN). The same arithmetic as completeScalars, which calls it.
[[nodiscard]] std::uint8_t completeScalarRow(StressRow& row, bool strain, bool solid, bool values);

// What a chain gave for a request: the rows, what each provider did, and the elements the request named that no way
// gave.
struct ElementResolution
{
    ElementFlatSet set;
    std::vector<ProviderAttempt> attempts;
    std::vector<std::int32_t> missing;
};

// Runs a chain and applies `onMissing`: with "raise", nothing at all or an element the request named that has no value
// is a QuantityUnavailableError that lists the ways tried (`what` names the quantity); with "null", the elements named
// that have no value get a row of nulls (the value fields `fields`) at each (subcase, step) the other rows have; with
// "skip" they are left out. Nothing at all gives an empty set under "null" and "skip".
[[nodiscard]] ElementResolution resolveElements(const ResultQueryContext& context, const ElementChain& chain,
                                                const ElementRequest& request, const std::string& what,
                                                const std::vector<std::string>& fields);

// The chains of the element quantities.
[[nodiscard]] const ElementChain& stressChain();
[[nodiscard]] const ElementChain& strainChain();
[[nodiscard]] const ElementChain& shellResultantChain();
// The forces of the shear panels: the mean shear flow of each CSHEAR and the axial forces of the
// effective rods of its edges, recovered from the translations of its grid points; the fields of its flats are
// shearPanelForceFields().
[[nodiscard]] const ElementChain& shearPanelForceChain();
// `shearFlow rod12 rod23 rod34 rod41`.
[[nodiscard]] const std::vector<std::string>& shearPanelForceFields();
[[nodiscard]] const ElementChain& oneDimensionalForceChain();
[[nodiscard]] const ElementChain& fastenerForceChain();
// The heat flux of the elements: the table of the result file, else the conductivity times the temperature gradient.
[[nodiscard]] const ElementChain& heatFluxChain();
// The value fields of the flats of the heat flux chain: `gradientX gradientY gradientZ fluxX fluxY fluxZ`.
[[nodiscard]] const std::vector<std::string>& heatFluxFields();

// The value fields of the flats of the force chains: `axial shear1 shear2 torque moment1 moment2` for the
// one-dimensional elements, the same with `shear` for the fasteners, `nx ny nxy mx my mxy qx qy` for the shells.
[[nodiscard]] const std::vector<std::string>& oneDimensionalFields();
[[nodiscard]] const std::vector<std::string>& fastenerFields();

// A stored table of forces read for a block (`readBlock`; a companion package, a companion package): the decoded table
// and, for each row it gives, its place, without the flats of the providers (a flat of a table is some 110 bytes a row,
// made and freed for every subcase: most of the cost of a block). The rows of a table are its rows in order, each
// twice for a bar (its end A, then its end B), at each of its steps (a table in memory may give rows of elements not
// asked for: it is shared whole, the block leaves them out); the place of a row is the one the chain of its kind gives
// it, the values those of the fields of the kind (`oneDimensionalFields()`, the components of `shellForces`) from
// `source[end][field]`, the component of the table (-1: NaN).
//
// The stored stress and strain tables are read the same way: `source[0][field]` is then the
// component of each value field of stressFields() (strainFields()), the layer of a row is a place of its own, the
// fiber distance of the first row of a place is its position (`position`), and the scalars a table does not store are
// completed row by row as completeScalars completes them (`completion`).
struct DirectForceTable
{
    // the completion of the derived scalars of a stress or strain row: none (the forces, rods, shear panels), of a
    // shell, of a solid (whose principal values are sorted first, as flattenStressTable sorts them)
    enum class Completion : std::uint8_t
    {
        None,
        Shell,
        Solid
    };
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    std::string elementName;
    bool stepped = false; // the steps of the table are cases of their own (NaN otherwise)
    bool bar = false;     // two rows of each row of the table, its ends
    std::shared_ptr<const results::ResultData> decoded;
    std::array<std::array<int, 16>, 2> source{};
    int position = -1; // the component whose value at the first step is the position of a place (-1: the station)
    Completion completion = Completion::None;
    bool strain = false; // the rows are strains (the text of a completion)
    // per row given (a row of the table, an end of a bar), shared by the tables of the same rows: x/L (NaN when
    // unknown), the place (a code of the names of the kind: 0 endA, 1 endB, 2 station of a bar or a beam; 0 center,
    // 1 corner of a shell or a solid), the grid point and the layer (null: 0 for every row)
    std::shared_ptr<const std::vector<double>> station;
    std::shared_ptr<const std::vector<std::uint8_t>> location;
    std::shared_ptr<const std::vector<std::int32_t>> node;
    std::shared_ptr<const std::vector<std::int32_t>> layer;
};
using StationForceTable = DirectForceTable;
// The names of the place codes of the tables of bars and beams (endA, endB, station) and of shells (center, corner),
// in the order of the names.
[[nodiscard]] const std::vector<std::string>& stationPlaceNames();
[[nodiscard]] const std::vector<std::string>& shellPlaceNames();
// The tables of the shells for the request, or nothing when the providers of `shellResultantChain` must be asked (they
// are then what a block reads): a request of properties, places, layers or a frame other than the stored one, the
// element or the material frame; a force table of shear panels; a stress table of shells whose rows the force tables
// do not all cover (the stresses would then be integrated), or any with every element asked for; a complex table, a
// provider switched off, or an element asked for that no table has a row of. The tables are those the stored-table
// provider reads, turned as it turns them. `source` is set to the text of the provider of the rows.
[[nodiscard]] std::optional<std::vector<DirectForceTable>> shellForceTables(const ResultQueryContext& context,
                                                                            const ElementRequest& request,
                                                                            std::string& source);
// The name that switches off the direct read of the shells (the test of its equality with the providers').
inline constexpr const char* SHELL_FORCE_SWITCH = "stored forces of shells read straight into a block";
// The tables of the bars and beams for the request, or nothing when the providers of `oneDimensionalForceChain` must
// be asked (they are then what a block reads): a request of other elements (rods, which may come from their stresses),
// of properties, places or layers, of every element when a rod's stress is there, a complex table, a provider switched
// off, or an element asked for that no table has a row of. `source` is set to the text of the provider of the rows.
[[nodiscard]] std::optional<std::vector<StationForceTable>> stationForceTables(const ResultQueryContext& context,
                                                                               const ElementRequest& request,
                                                                               std::string& source);
// The name that switches off the direct read of bars and beams (the test of its equality with the providers').
inline constexpr const char* STATION_FORCE_SWITCH = "stored forces of bars and beams read straight into a block";
// The stored stress (or strain) tables of shells, solids, rods and shear panels for the request, or nothing when the
// providers of `stressChain` (`strainChain`) must be asked (they are then what a block reads): a request of
// properties, places or layers; a table of bars or beams with a row of an element asked for, or any with every element;
// a complex table, a provider switched off, an element asked for that no table has a row of; with a model, a table
// another provider of the chain starts from (the strains or the stresses, the forces of shells and rods) unless the
// stored tables of its subcase and element type have every element asked for of that type (its rows would all be
// dropped), or every element is asked for. The tables are those StoredTableProvider reads, in its order and turned to
// the frame of the call as it turns them. `source` is set to the text of the provider of the rows.
[[nodiscard]] std::optional<std::vector<DirectForceTable>> storedStressTables(const ResultQueryContext& context,
                                                                              const ElementRequest& request,
                                                                              bool strain, std::string& source);
// The name that switches off the direct read of the stresses and strains (the test of its equality with the
// providers').
inline constexpr const char* STRESS_TABLE_SWITCH = "stored stresses and strains read straight into a block";

// The ways of obtaining von Mises, the principal values and the maximum shear, for the catalogue of the quantities.
[[nodiscard]] QuantityInfo derivedScalarInfo();

// The text of the reasons a recomputation gave nothing, from the elements whose model data were missing.
[[nodiscard]] std::string joinReasons(const std::map<std::string, std::int64_t>& reasons);

} // namespace oor::query
