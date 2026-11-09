#pragma once

#include "oor/model/cards/loads/freeBodyLoadSubcase/FreeBodyLoadSubcaseCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FreeBodyLoadSubcaseFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreeBodyLoadSubcaseCard& card);
    [[nodiscard]] static std::optional<FreeBodyLoadSubcaseCard> read(FieldReader& reader);
    static void write(const FreeBodyLoadSubcaseCard& card, FieldWriter& writer);
};

// All the FBODYSB cards. A subsystem has no number (IdSpace::None): it is found by its name, which identifies it for
// the FBODYLD that names it (a repeated name is reported as a warning when the deck is read). The two sets are
// references (ReferenceTarget::Set); a blank ELEMSET is not visited.
class FreeBodyLoadSubcaseTable final : public TypedTable<FreeBodyLoadSubcaseTable, FreeBodyLoadSubcaseCard>
{
public:
    static constexpr std::string_view NAME = "FreeBodyLoadSubcase";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = FreeBodyLoadSubcaseFormat;

    [[nodiscard]] static base::EntityId cardId(const FreeBodyLoadSubcaseCard&) noexcept { return 0; }
    std::uint32_t append(const FreeBodyLoadSubcaseCard& card, base::CardKey key);
    [[nodiscard]] FreeBodyLoadSubcaseCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreeBodyLoadSubcaseCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // Two FBODYSB cards of one name are reported.
    void finishRead(Model& model, const ReadContext& context) override;

    // The name of a row, as written, without copying the card.
    [[nodiscard]] std::string nameAt(std::uint32_t row) const { return m_names.get(row); }
    [[nodiscard]] Column<std::string>& namesColumn() noexcept { return m_names; }
    [[nodiscard]] Column<base::EntityId>& nodeSetIdColumn() noexcept { return m_nodeSetId; }
    [[nodiscard]] Column<base::EntityId>& elementSetIdColumn() noexcept { return m_elementSetId; }
    [[nodiscard]] Column<std::string>& excludedForcesColumn() noexcept { return m_excludedForces; }
    [[nodiscard]] Column<std::string>& labelColumn() noexcept { return m_label; }

private:
    Column<std::string> m_names;
    Column<base::EntityId> m_nodeSetId;
    Column<base::EntityId> m_elementSetId;
    Column<std::string> m_excludedForces;
    Column<std::string> m_label;
};

} // namespace oor::model
