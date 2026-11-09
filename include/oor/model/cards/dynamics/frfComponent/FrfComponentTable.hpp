#pragma once

#include "oor/model/cards/dynamics/frfComponent/FrfComponentCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FrfComponentFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FrfComponentCard& card);
    [[nodiscard]] static std::optional<FrfComponentCard> read(FieldReader& reader);
    static void write(const FrfComponentCard& card, FieldWriter& writer);
};

// All the FRFCOMP cards. The COMPID is unique among them (IdSpace::PerCardType, category FrfComponent): the other FRF
// cards name a component by it (ReferenceTarget::FrfComponent), or by its name. The card names no other card.
class FrfComponentTable final : public TypedTable<FrfComponentTable, FrfComponentCard>
{
public:
    static constexpr std::string_view NAME = "FrfComponent";
    static inline const CardTypeInfo INFO{NAME, CardCategory::FrfComponent, base::IdSpace::PerCardType};
    using Format = FrfComponentFormat;

    FrfComponentTable() : m_lengthScaleFactor(1.0), m_forceScaleFactor(1.0) {}

    [[nodiscard]] static base::EntityId cardId(const FrfComponentCard& card) noexcept { return card.id; }
    std::uint32_t append(const FrfComponentCard& card, base::CardKey key);
    [[nodiscard]] FrfComponentCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FrfComponentCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }
    // The medium as the number of its FrfMedium: 0 the database, 1 a binary results file, 2 a universal file.
    [[nodiscard]] Column<std::int32_t>& mediumColumn() noexcept { return m_medium; }
    [[nodiscard]] Column<std::int32_t>& unitNumberColumn() noexcept { return m_unitNumber; }
    [[nodiscard]] Column<double>& lengthScaleFactorColumn() noexcept { return m_lengthScaleFactor; }
    [[nodiscard]] Column<double>& forceScaleFactorColumn() noexcept { return m_forceScaleFactor; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_name;
    Column<std::int32_t> m_medium;
    Column<std::int32_t> m_unitNumber;
    Column<double> m_lengthScaleFactor;
    Column<double> m_forceScaleFactor;
};

} // namespace oor::model
