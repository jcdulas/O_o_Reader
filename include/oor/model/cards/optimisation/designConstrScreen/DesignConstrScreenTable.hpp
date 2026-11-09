#pragma once

#include "oor/model/cards/optimisation/designConstrScreen/DesignConstrScreenCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct DesignConstrScreenFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignConstrScreenCard& card);
    [[nodiscard]] static std::optional<DesignConstrScreenCard> read(FieldReader& reader);
    static void write(const DesignConstrScreenCard& card, FieldWriter& writer);
};

// All the DSCREEN cards. A card has no number: it is found by its response type (`model.dscreen["DISP"]`), which a deck
// gives once (the check `design.dscreen-repeated`).
class DesignConstrScreenTable final : public TypedTable<DesignConstrScreenTable, DesignConstrScreenCard>
{
public:
    static constexpr std::string_view NAME = "DesignConstrScreen";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = DesignConstrScreenFormat;

    DesignConstrScreenTable()
        : m_threshold(DesignConstrScreenCard::DEFAULT_THRESHOLD),
          m_maxRetained(DesignConstrScreenCard::DEFAULT_MAX_RETAINED)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const DesignConstrScreenCard&) noexcept { return 0; }
    std::uint32_t append(const DesignConstrScreenCard& card, base::CardKey key);
    [[nodiscard]] DesignConstrScreenCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DesignConstrScreenCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<std::string>& responseKindColumn() noexcept { return m_responseKind; }
    [[nodiscard]] Column<double>& thresholdColumn() noexcept { return m_threshold; }
    [[nodiscard]] Column<std::int32_t>& maxRetainedColumn() noexcept { return m_maxRetained; }

private:
    Column<std::string> m_responseKind;
    Column<double> m_threshold;
    Column<std::int32_t> m_maxRetained;
};

} // namespace oor::model
