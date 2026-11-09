#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// What the five non-structural mass cards (NSM, NSM1, NSML, NSML1, NSMADD) have in common: the TYPE word that says what
// the IDs are, the (ID, VALUE) pairs of NSM and NSML, and the ID lists of NSM1 and NSML1 with their THRU, BY and ALL.
// One kind of value used by a family of cards: each card still reads, writes and checks its own fields.

// The properties and elements a non-structural mass can be put on (the TYPE field). A card keeps the neutral name of
// the card type (ShellProp, CompProp, CompPropGlobal, BarProp, BarShapeProp, BeamProp, BeamShapeProp,
// BeamLumpedAreaProp, RodProp, Rod2InlineSection, CurvedBeamProp, ShearPanelProp, TubeProp, ConicalShellProp,
// CrackTipProp2D), or the plain word ELEMENT; the deck writes their keywords (PSHELL, ...).
inline constexpr std::string_view NON_STRUCT_MASS_ELEMENT = "Element";
[[nodiscard]] bool isNonStructMassType(std::string_view type) noexcept;
// True for the types whose IDs are element IDs (ELEMENT and Rod2InlineSection).
[[nodiscard]] bool nonStructMassTypeIsElement(std::string_view type) noexcept;
// Reads the TYPE field: a neutral card name (or Element), or the word as written after an error when it is not a type.
[[nodiscard]] std::string readNonStructMassType(FieldReader& reader, int field);
[[nodiscard]] std::optional<CardProblem> checkNonStructMassType(const std::string& type);
// What an ID of a card of this TYPE refers to: an element (ELEMENT, Rod2InlineSection), a property, or nothing the
// library has (CrackTipProp2D).
[[nodiscard]] ReferenceTarget nonStructMassTarget(std::string_view type) noexcept;

// One (ID, VALUE) pair of NSM and NSML.
struct NonStructMassPair
{
    base::EntityId id = 0;
    double value = 0.0;

    [[nodiscard]] bool operator==(const NonStructMassPair&) const noexcept = default;
};

// The pairs from field `first` to the last used one (blank pairs, the padding of continuation lines, are skipped).
[[nodiscard]] std::vector<NonStructMassPair> readNonStructMassPairs(FieldReader& reader, int first);
// Writes the pairs from field `first` on.
void writeNonStructMassPairs(FieldWriter& writer, int first, const std::vector<NonStructMassPair>& pairs);
[[nodiscard]] std::optional<CardProblem> checkNonStructMassPairs(const std::vector<NonStructMassPair>& pairs);

// A run of the ID list of NSM1 and NSML1: one ID (first == last), "first THRU last", or "first THRU last BY by".
struct NonStructMassRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;
    base::EntityId increment = 1;

    [[nodiscard]] bool operator==(const NonStructMassRun&) const noexcept = default;
};

// The IDs of NSM1 and NSML1: the word ALL (every property of the type, every element), or the runs as written.
struct NonStructMassIdList
{
    bool all = false;
    std::vector<NonStructMassRun> runs;

    [[nodiscard]] bool operator==(const NonStructMassIdList&) const noexcept = default;
};

[[nodiscard]] NonStructMassIdList readNonStructMassIds(FieldReader& reader, int first);
void writeNonStructMassIds(FieldWriter& writer, int first, const NonStructMassIdList& ids);
[[nodiscard]] std::optional<CardProblem> checkNonStructMassIds(const NonStructMassIdList& ids);

// The IDs the runs stand for (a THRU range stands for every ID of it that is a multiple of BY from the first; the
// property or element need not exist), in the order of the card. Empty for ALL. More than base::MAX_EXPANDED_IDS
// IDs is a LimitExceededError.
[[nodiscard]] std::vector<base::EntityId> expandNonStructMassIds(const NonStructMassIdList& ids);

// The runs as the triples a table column keeps, and back.
[[nodiscard]] std::vector<std::array<base::EntityId, 3>> toTriples(const std::vector<NonStructMassRun>& runs);
[[nodiscard]] std::vector<NonStructMassRun> toNonStructMassRuns(std::span<const std::array<base::EntityId, 3>> triples);

// Visits the IDs of a column of runs with the target of the type: every end of every run, none of the BY increments.
void visitNonStructMassRuns(VariableLengthColumn<std::array<base::EntityId, 3>>& column, std::uint32_t row,
                            ReferenceVisitor& visitor, std::string_view type);

// Visits the IDs of a column of NSM / NSML IDs with the target of the type.
void visitNonStructMassIds(VariableLengthColumn<base::EntityId>& column, std::uint32_t row, ReferenceVisitor& visitor,
                           std::string_view type);

} // namespace oor::model
