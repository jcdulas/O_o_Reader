#pragma once

#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <optional>
#include <vector>

namespace oor::analysis
{

// The non-structural mass that a set of the cards NSM, NSM1, NSML, NSML1 and NSMADD puts on the elements of a model.
//
// The set is selected by the Case Control command NSM = SID. The mass is on top of the NSM field of the property and of
// the element:
//   NSM, NSM1     VALUE is a mass per unit area (shells, shear panels) or per unit length (rods, tubes, bars, beams)
//   put on
//                 every element of the property (or on the element): the mass of an element is VALUE times its area or
//                 length;
//   NSML, NSML1   VALUE is a mass, distributed over the elements of the property (NSML: of each ID; NSML1: of all the
//                 IDs of the card together) in proportion to their area or length, so that the mass added is VALUE;
//   NSMADD        the set is the sum of the sets it lists; it takes precedence over cards of the same set ID.
// PCOMP and PCOMPG count as PSHELL, PBARL as PBAR, PBEAML and PBCOMP as PBEAM, ALL is every
// property of the family (or every element for ELEMENT), and an ID that names nothing is ignored (format reference:
// undefined property and element IDs are ignored). Elements without a length or an area (an arc of a CBEND, the
// elements between scalar points) get nothing.
class NonStructuralMass
{
public:
    // The IDs of the sets the model defines, increasing.
    [[nodiscard]] static std::vector<base::EntityId> setIds(const model::Model& model);
    // True when a card of the model defines the set.
    [[nodiscard]] static bool exists(const model::Model& model, base::EntityId set);
    // The set the Case Control selects with NSM = SID: the command above the subcases, else the one of the first
    // subcase that has it; nullopt when there is none.
    [[nodiscard]] static std::optional<base::EntityId> selectedByCaseControl(const model::Model& model);

    // The mass the set puts on each element. Throws MassError when the set is not defined, or when a lumped mass is
    // shared by line elements and area elements.
    [[nodiscard]] static NonStructuralMass resolve(const model::Model& model, base::EntityId set,
                                                   base::ThreadPool* pool = nullptr);

    [[nodiscard]] base::EntityId set() const noexcept { return m_set; }
    // The mass added to the element in this row of the table of this card type (0 when none).
    [[nodiscard]] double massOf(std::uint16_t type, std::uint32_t row) const noexcept
    {
        return type < m_added.size() && row < m_added[type].size() ? m_added[type][row] : 0.0;
    }
    // The sum of the mass added to all the elements.
    [[nodiscard]] double total() const noexcept { return m_total; }

private:
    base::EntityId m_set = 0;
    std::vector<std::vector<double>> m_added; // by card type, then by row
    double m_total = 0.0;
};

} // namespace oor::analysis
