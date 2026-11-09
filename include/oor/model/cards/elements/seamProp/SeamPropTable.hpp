#pragma once

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/elements/seamProp/SeamPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct SeamPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SeamPropCard& card);
    [[nodiscard]] static std::optional<SeamPropCard> read(FieldReader& reader);
    static void write(const SeamPropCard& card, FieldWriter& writer);
};

// All the PSEAM properties. The ID is a property ID, in the space every property shares; MID names a material. A blank
// T is a NaN in its column.
class SeamPropTable final : public TypedTable<SeamPropTable, SeamPropCard>
{
public:
    static constexpr std::string_view NAME = "SeamProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = SeamPropFormat;

    SeamPropTable()
        : m_kind(std::string(SeamPropCard::DEFAULT_TYPE)),
          m_thickness(BLANK_REAL),
          m_integration(SeamPropCard::DEFAULT_INTEGRATION)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const SeamPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const SeamPropCard& card, base::CardKey key);
    [[nodiscard]] SeamPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SeamPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<double>& widthColumn() noexcept { return m_width; }
    [[nodiscard]] Column<double>& thicknessColumn() noexcept { return m_thickness; }
    [[nodiscard]] Column<std::int32_t>& integrationColumn() noexcept { return m_integration; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<std::string> m_kind;
    Column<double> m_width;
    Column<double> m_thickness;
    Column<std::int32_t> m_integration;
};

} // namespace oor::model
