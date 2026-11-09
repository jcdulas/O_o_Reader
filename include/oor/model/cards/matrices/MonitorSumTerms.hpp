#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// What the cards that add monitor results together (MONSUM, MONSUM1, MONSUMT) have in common, one kind of value each:
// the kind of monitor result a card merges (the MTYPE words), the name of a monitor result that another entry names,
// and the lists of (new component, monitor type, name, components, coefficient) of MONSUM and MONSUM1, which have one
// grammar of lines. Each card still lays out, reads, writes and checks its own head fields by hand.
namespace oor::model
{

// ---------------------------------------------------------------------------------------------------------------------
// The kind of monitor result
// ---------------------------------------------------------------------------------------------------------------------

// The MTYPE words of the format: the aerodynamic and the structural MONPNT1 (AMONPNT1, SMONPNT1), the average
// displacements (AMONDSP1, SMONDSP1) and the MONPNT3. The first four name the kind of monitor point, not a card (a
// MONPNT1 is aerodynamic or structural by its COMP); the last is a card, named by its neutral name.
enum class MonitorResultType : std::uint8_t
{
    AeroPoint,
    AeroDisplacement,
    StructuralPoint,
    StructuralDisplacement,
    LoadPoint
};

inline constexpr std::array<std::string_view, 5> MONITOR_RESULT_WORDS{
    "AeroMonitorIntegratedLoad", "AeroMonitorDisplacement", "StructMonitorIntegratedLoad", "StructMonitorDisplacement",
    "MonitorNodeForceSum3"};

// A set of monitor types, as the bits of the enumerators: what a card accepts.
[[nodiscard]] constexpr std::uint32_t monitorTypeBit(MonitorResultType type) noexcept
{
    return 1U << static_cast<unsigned>(type);
}

// Every type (MONSUM), and the three whose results are forces and moments (MONSUM1 and MONSUMT).
inline constexpr std::uint32_t ALL_MONITOR_TYPES =
    monitorTypeBit(MonitorResultType::AeroPoint) | monitorTypeBit(MonitorResultType::AeroDisplacement) |
    monitorTypeBit(MonitorResultType::StructuralPoint) | monitorTypeBit(MonitorResultType::StructuralDisplacement) |
    monitorTypeBit(MonitorResultType::LoadPoint);
inline constexpr std::uint32_t FORCE_MONITOR_TYPES = monitorTypeBit(MonitorResultType::AeroPoint) |
                                                     monitorTypeBit(MonitorResultType::StructuralPoint) |
                                                     monitorTypeBit(MonitorResultType::LoadPoint);

// The format reference calls the force and moment monitor points and the average displacement ones "similar" within the
// family: the results of one sum must be of one family.
[[nodiscard]] constexpr bool isDisplacementType(MonitorResultType type) noexcept
{
    return type == MonitorResultType::AeroDisplacement || type == MonitorResultType::StructuralDisplacement;
}

// The word a record writes for a type: its neutral name.
[[nodiscard]] std::string monitorTypeWord(MonitorResultType type);

// The accepted words as a phrase for a message: "AMONPNT1, SMONPNT1 or {card:MonitorNodeForceSum3}" (the reader puts
// the keyword of the card in place of the placeholder, see CardProblem).
[[nodiscard]] std::string acceptedMonitorTypes(std::uint32_t accepted);

// The word of field `field`, in any case, if it is one of the accepted ones; an error is reported otherwise (a blank
// field is not an error here: the caller says whether a type is required).
[[nodiscard]] std::optional<MonitorResultType> readMonitorType(FieldReader& reader, int field, const char* name,
                                                               std::uint32_t accepted);

// The problem with a type that a card does not accept, if there is one. `field` is a literal.
[[nodiscard]] std::optional<CardProblem> monitorTypeProblem(MonitorResultType type, std::uint32_t accepted,
                                                            const char* field);

// The name of a monitor result that a card names (the NAMEij of a sum, the NAME of a monitor group member): one to
// eight characters with no blank, comma or '$', like the NAME of a monitor point. It is compared without regard to the
// case and kept in upper case, as the NAME of the point it names is. `field` is a literal.
[[nodiscard]] std::optional<CardProblem> monitoredNameProblem(std::string_view name, const char* field);

// ---------------------------------------------------------------------------------------------------------------------
// The terms of a MONSUM and a MONSUM1
// ---------------------------------------------------------------------------------------------------------------------

// One term of the sum that fills the components `newAxes` of the new monitor result: the components `axes` of the
// monitor result `name` (of the kind `type`) times `coefficient`. A card keeps its terms in one list in the order of
// the lines; the terms with one `newAxes` in a row are the block of one NEWAXISj, and the terms of a block with one
// `type` in a row are a section of it.
struct MonitorSumTerm
{
    std::int32_t newAxes = 0; // NEWAXISj: digits 1 to 6, each at most once, kept in ascending order
    MonitorResultType type = MonitorResultType::AeroPoint;
    std::string name;
    std::int32_t axes = 0;    // AXESij: blank is the components of NEWAXISj, stored resolved
    double coefficient = 1.0; // COEFij

    [[nodiscard]] bool operator==(const MonitorSumTerm&) const noexcept = default;
};

// The terms and the layout they were read in: the format draws the sum twice, with the first two fields of a line the
// other way round. In the first layout (`alternate` false) a line starts with its NEWAXISj and then its MTYPE; in the
// alternate layout it starts with the MTYPE and then the NEWAXISj. The alternate layout writes the MTYPE once for the
// lines that go on with it (the format draws it once, in the first line), the first layout writes it on the first line
// of every block and section.
struct MonitorSumBody
{
    bool alternate = false;
    std::vector<MonitorSumTerm> terms;

    [[nodiscard]] bool operator==(const MonitorSumBody&) const noexcept = default;
};

// Each line after the head fields of the card holds, in the library numbering (field 1 is the first data field and
// each line has eight):
//   layout 1:    field 1 NEWAXISj (blank on a line that goes on), field 2 MTYPE (blank on a line that goes on), then
//   two
//                triples NAME, AXES, COEF in fields 3 to 5 and 6 to 8;
//   alternate:   field 1 MTYPE, field 2 NEWAXISj, then the triples; the MTYPE is blank on a line that goes on with the
//   type
//                above it, and the NEWAXISj on a line that goes on with its block.
// A NEWAXISj that has no MTYPE on its line goes on with the type of the line above, so the type may be given on the
// first line only; a line that changes the type and repeats the NEWAXISj starts a second section of the same block.
// `firstLine` is the first of these lines (1 for a MONSUM, 2 for a MONSUM1 whose line 1 holds the location). Lines that
// are blank are padding. A triple with a NAME and no AXES takes the components of its NEWAXISj; a blank COEF is 1.0.
// `accepted` is the set of types the card takes; `newAxesName` is the format reference name of the NEWAXIS field for
// the messages (a literal). Errors are reported and the terms read so far are given.
[[nodiscard]] MonitorSumBody readMonitorSumTerms(FieldReader& reader, int firstLine, std::uint32_t accepted,
                                                 const char* newAxesName);

// Writes the lines from line `firstLine`, two terms to a line, a new line for each block and each section.
void writeMonitorSumTerms(FieldWriter& writer, int firstLine, const MonitorSumBody& body);

// The first thing wrong with the terms: at least one term; names, types and sets of components valid; one family of
// types; the NEWAXISj of the blocks share no component; the components of a block's terms are the
// components of its NEWAXISj or, for a block that fills one component, any single component.
// `newAxesName` is a literal.
[[nodiscard]] std::optional<CardProblem> checkMonitorSumTerms(const MonitorSumBody& body, std::uint32_t accepted,
                                                              const char* newAxesName);

} // namespace oor::model
