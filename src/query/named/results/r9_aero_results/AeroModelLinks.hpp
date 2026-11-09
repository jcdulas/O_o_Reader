#pragma once

#include "oor/model/cards/aero/AeroBoxRanges.hpp"
#include "oor/query/results/ResultSets.hpp"
#include "oor/query/results/RowAccumulator.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

// What the aeroelastic result queries take from the model a result set is attached to: the CAERO that owns a box and
// the AESURF that has a label. A result set that has no model, a box no panel owns and a
// label no surface has give a null, never a guess; the queries record which in the provenance of their table.
//
// The IDs of a result file are the IDs of the deck the analysis ran. The attachment follows nodes and elements by card
// key through a renumbering, but not boxes: the boxes are looked up in the model as it is now.

namespace oor::query
{

// A column of IDs found for the rows of a result table, with the counts the provenance reports.
struct IdLookup
{
    std::vector<std::int32_t> id;
    std::vector<std::uint8_t> valid; // 0: null
    std::size_t found = 0;
    std::size_t withoutCard = 0; // the model has no card for the row
    std::size_t ambiguous = 0;   // several cards fit (two AESURF cards with one label)
};

class AeroModelLinks
{
public:
    explicit AeroModelLinks(const ResultQueryContext& context) : m_context(context) {}

    [[nodiscard]] bool hasModel(std::size_t set) const { return m_context.modelOf(set) != nullptr; }
    // " (the model was edited after the results were attached)" when it was, else nothing: the text a provider name
    // takes so that the provenance says the lookup is made in a model that may not be the one that was analysed.
    [[nodiscard]] std::string editedNote(std::size_t set) const;

    // The CAERO1 to CAERO5 that owns each box (`AeroBoxRanges`).
    [[nodiscard]] IdLookup panelsOfBoxes(std::size_t set, std::span<const std::int32_t> boxes);
    // The AESURF that has each label (compared without regard to the case and to the blanks around it).
    [[nodiscard]] IdLookup surfacesOfLabels(std::size_t set, const std::vector<std::string>& labels);

private:
    struct SetData
    {
        std::unique_ptr<model::AeroBoxRanges> boxes;
        std::unordered_map<std::string, std::int32_t> surfaces; // upper case label -> ID, 0 when two cards have it
        bool surfacesRead = false;
    };
    [[nodiscard]] SetData& dataOf(std::size_t set);

    const ResultQueryContext& m_context;
    std::map<const model::Model*, SetData> m_data;
};

// The words the provenance of a table uses for the ID column a lookup filled: the name of the column, how it was found,
// and why a row has none (the model has no card for it, the result set has no model, several cards fit).
struct LookupWords
{
    std::string column;
    std::string found;
    std::string withoutCard;
    std::string noModel;
    std::string ambiguous;
};

// Records in the provenance of the table how the IDs of `rows` rows were found, or why they are null. A provider name
// takes the note of an edited model (`AeroModelLinks::editedNote`).
void noteLookup(RowAccumulator& accumulator, const AeroModelLinks& links, std::size_t set, const IdLookup& lookup,
                std::size_t rows, const LookupWords& words);

// The ID of the CAERO that owns each element of the pressure table of a subcase when the force table of the same
// subcase lists the same elements in the same order (the forces of a result file are on an internal number that follows
// the order of the pressure table).
// `pressureBoxes` and `pressureKinds` are the element IDs and the element kinds (LS, ZIB, ...) of the pressure table,
// `forceKinds` the kinds of the force table. Empty when the two tables cannot be paired.
[[nodiscard]] std::vector<std::int32_t> boxesOfForces(std::span<const std::int32_t> pressureBoxes,
                                                      const std::vector<std::string>& pressureKinds,
                                                      const std::vector<std::string>& forceKinds);

} // namespace oor::query
