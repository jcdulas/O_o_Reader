#pragma once

#include "oor/model/cards/loads/freeBodyLoad/FreeBodyLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FreeBodyLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreeBodyLoadCard& card);
    [[nodiscard]] static std::optional<FreeBodyLoadCard> read(FieldReader& reader);
    static void write(const FreeBodyLoadCard& card, FieldWriter& writer);
};

// All the FBODYLD cards. A load case has no number (IdSpace::None): it is found by its name, which the format reference
// says is unique (a repeated name is reported when the deck is read). The three fields are text columns.
class FreeBodyLoadTable final : public TypedTable<FreeBodyLoadTable, FreeBodyLoadCard>
{
public:
    static constexpr std::string_view NAME = "FreeBodyLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = FreeBodyLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const FreeBodyLoadCard&) noexcept { return 0; }
    std::uint32_t append(const FreeBodyLoadCard& card, base::CardKey key);
    [[nodiscard]] FreeBodyLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreeBodyLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the subsystem is named, not numbered
    // Two FBODYLD cards of one name are reported.
    void finishRead(Model& model, const ReadContext& context) override;

    // The name of a row, as written, without copying the card.
    [[nodiscard]] std::string nameAt(std::uint32_t row) const { return m_name.get(row); }
    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }
    [[nodiscard]] Column<std::string>& subsystemNameColumn() noexcept { return m_subsystemName; }
    [[nodiscard]] Column<std::string>& labelColumn() noexcept { return m_label; }

private:
    Column<std::string> m_name;
    Column<std::string> m_subsystemName;
    Column<std::string> m_label;
};

} // namespace oor::model
