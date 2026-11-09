#pragma once

#include "oor/model/cards/solution_control/cosimulationService/CosimulationServiceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CosimulationServiceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CosimulationServiceCard& card);
    [[nodiscard]] static std::optional<CosimulationServiceCard> read(FieldReader& reader);
    static void write(const CosimulationServiceCard& card, FieldWriter& writer);
};

// All the COSMSRV cards, in the category CosimulationService: the CSRVID is unique among them (IdSpace::PerCardType)
// and a COSMSEL names a card by it. GRPID1 and GRPID2 are references to COSMGRP cards, PHYINP to a COSMINP and PHYOUT
// to a COSMOUT; the SERV is a name, not a reference. The cards are kept whole next to a column of their CSRVIDs.
class CosimulationServiceTable final : public TypedTable<CosimulationServiceTable, CosimulationServiceCard>
{
public:
    static constexpr std::string_view NAME = "CosimulationService";
    static inline const CardTypeInfo INFO{NAME, CardCategory::CosimulationService, base::IdSpace::PerCardType};
    using Format = CosimulationServiceFormat;

    [[nodiscard]] static base::EntityId cardId(const CosimulationServiceCard& card) noexcept { return card.id; }
    std::uint32_t append(const CosimulationServiceCard& card, base::CardKey key);
    [[nodiscard]] CosimulationServiceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CosimulationServiceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CosimulationServiceCard> m_cards;
};

} // namespace oor::model
