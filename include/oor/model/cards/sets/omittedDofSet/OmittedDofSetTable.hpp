#pragma once

#include "oor/model/cards/sets/omittedDofSet/OmittedDofSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct OmittedDofSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OmittedDofSetCard& card);
    [[nodiscard]] static std::optional<OmittedDofSetCard> read(FieldReader& reader);
    static void write(const OmittedDofSetCard& card, FieldWriter& writer);
};

// All the OMIT cards. A card has no ID (IdSpace::None). The points and the components of every card are two lists in
// CSR form.
class OmittedDofSetTable final : public TypedTable<OmittedDofSetTable, OmittedDofSetCard>
{
public:
    static constexpr std::string_view NAME = "OmittedDofSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::None};
    using Format = OmittedDofSetFormat;

    [[nodiscard]] static base::EntityId cardId(const OmittedDofSetCard&) noexcept { return 0; }
    std::uint32_t append(const OmittedDofSetCard& card, base::CardKey key);
    [[nodiscard]] OmittedDofSetCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const OmittedDofSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }

private:
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<std::int32_t> m_component;
};

} // namespace oor::model
