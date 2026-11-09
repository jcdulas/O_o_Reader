#pragma once

#include "oor/model/cards/dynamics/complexEigenPoles/ComplexEigenPolesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ComplexEigenPolesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ComplexEigenPolesCard& card);
    [[nodiscard]] static std::optional<ComplexEigenPolesCard> read(FieldReader& reader);
    static void write(const ComplexEigenPolesCard& card, FieldWriter& writer);
};

// All the EIGP cards. The ID is the ID of the EIGC the poles go with: it is shared by the EIGP cards of the set
// (IdSpace::Group, category ComplexMethod, which CMETHOD selects with the EIGC) and is renumbered with the EIGC, as the
// category is one space (RenumberSpace::ComplexMethod). An EIGP whose EIGC is missing is not reported (it would find
// itself in the category). A second pole that is not there has multiplicity 0 in its column.
class ComplexEigenPolesTable final : public TypedTable<ComplexEigenPolesTable, ComplexEigenPolesCard>
{
public:
    static constexpr std::string_view NAME = "ComplexEigenPoles";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ComplexMethod, base::IdSpace::Group};
    using Format = ComplexEigenPolesFormat;

    [[nodiscard]] static base::EntityId cardId(const ComplexEigenPolesCard& card) noexcept { return card.id; }
    std::uint32_t append(const ComplexEigenPolesCard& card, base::CardKey key);
    [[nodiscard]] ComplexEigenPolesCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ComplexEigenPolesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // see the class comment

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& realPart1Column() noexcept { return m_realPart1; }
    [[nodiscard]] Column<double>& imaginaryPart1Column() noexcept { return m_imaginaryPart1; }
    [[nodiscard]] Column<std::int32_t>& multiplicity1Column() noexcept { return m_multiplicity1; }
    [[nodiscard]] Column<double>& realPart2Column() noexcept { return m_realPart2; }
    [[nodiscard]] Column<double>& imaginaryPart2Column() noexcept { return m_imaginaryPart2; }
    [[nodiscard]] Column<std::int32_t>& multiplicity2Column() noexcept { return m_multiplicity2; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_realPart1;
    Column<double> m_imaginaryPart1;
    Column<std::int32_t> m_multiplicity1{1};
    Column<double> m_realPart2;
    Column<double> m_imaginaryPart2;
    Column<std::int32_t> m_multiplicity2; // 0: no second pole
};

} // namespace oor::model
