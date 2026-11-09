#pragma once

#include "oor/model/cards/loads/staticLoadSequence/StaticLoadSequenceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct StaticLoadSequenceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const StaticLoadSequenceCard& card);
    [[nodiscard]] static std::optional<StaticLoadSequenceCard> read(FieldReader& reader);
    static void write(const StaticLoadSequenceCard& card, FieldWriter& writer);
};

// All the LSEQ cards. The ID is the set ID that LOADSET picks (IdSpace::Group, category Load: it is a set of load cards
// like the others, and LID and TID refer to load sets). The EXCITEID is what the dynamic loads name as their EXCITEID:
// the table says so (definedExcitationId), and the reference check lets an EXCITEID that names one resolve.
class StaticLoadSequenceTable final : public TypedTable<StaticLoadSequenceTable, StaticLoadSequenceCard>
{
public:
    static constexpr std::string_view NAME = "StaticLoadSequence";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = StaticLoadSequenceFormat;

    [[nodiscard]] static base::EntityId cardId(const StaticLoadSequenceCard& card) noexcept { return card.id; }
    std::uint32_t append(const StaticLoadSequenceCard& card, base::CardKey key);
    [[nodiscard]] StaticLoadSequenceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const StaticLoadSequenceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    [[nodiscard]] bool definesExcitationIds() const noexcept override { return true; }
    [[nodiscard]] base::EntityId definedExcitationId(std::uint32_t row) const override
    {
        return m_excitationId.get(row);
    }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& excitationIdColumn() noexcept { return m_excitationId; }
    [[nodiscard]] Column<base::EntityId>& loadIdColumn() noexcept { return m_loadId; }
    [[nodiscard]] Column<base::EntityId>& tableIdColumn() noexcept { return m_tableId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_excitationId;
    Column<base::EntityId> m_loadId;
    Column<base::EntityId> m_tableId;
};

} // namespace oor::model
