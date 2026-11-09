#pragma once

#include "oor/model/cards/properties/solidProp/SolidPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The fields of a PSOLID that take a word (or an integer that stands for one).
enum class SolidPropOption
{
    Integration, // IN
    Stress,      // STRESS
    Isop,        // ISOP
    Function     // FCTN
};

struct SolidPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SolidPropCard& card);
    [[nodiscard]] static std::optional<SolidPropCard> read(FieldReader& reader);
    static void write(const SolidPropCard& card, FieldWriter& writer);

    // The word a field holds for what is written in it: an integer (0, 2, ...) or a short spelling (SMEC) becomes the
    // full word, upper case. A blank is an empty text for IN, STRESS and ISOP and SMECH for FCTN. nullopt when the
    // field does not take it.
    [[nodiscard]] static std::optional<std::string> canonicalOption(SolidPropOption which, std::string_view text);
};

// All the PSOLID cards. IN, STRESS, ISOP and FCTN are lists of texts in Python.
class SolidPropTable final : public TypedTable<SolidPropTable, SolidPropCard>
{
public:
    static constexpr std::string_view NAME = "SolidProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = SolidPropFormat;

    SolidPropTable() : m_elementFunction(std::string(SolidPropCard::DEFAULT_ELEMENT_FUNCTION)) {}

    [[nodiscard]] static base::EntityId cardId(const SolidPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const SolidPropCard& card, base::CardKey key);
    [[nodiscard]] SolidPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SolidPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }
    [[nodiscard]] Column<std::string>& integrationColumn() noexcept { return m_integration; }
    [[nodiscard]] Column<std::string>& stressLocationColumn() noexcept { return m_stressLocation; }
    [[nodiscard]] Column<std::string>& integrationOrderColumn() noexcept { return m_integrationOrder; }
    [[nodiscard]] Column<std::string>& elementFunctionColumn() noexcept { return m_elementFunction; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<base::EntityId> m_matFrameId;
    Column<std::string> m_integration;
    Column<std::string> m_stressLocation;
    Column<std::string> m_integrationOrder;
    Column<std::string> m_elementFunction;
};

} // namespace oor::model
