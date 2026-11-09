#include "oor/model/cards/nodes/moduleBulkOptions/ModuleBulkOptionsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <string>
#include <string_view>

namespace oor::model
{

namespace
{

constexpr std::array<std::string_view, 7> TYPE_WORDS{
    "Primary", "Repeat", "Mirror", "Move", "External", "ExternalBinaryResults", "ExternalMatrixFile"};

} // namespace

std::optional<ModuleBulkOptionsCard> ModuleBulkOptionsFormat::read(FieldReader& reader)
{
    ModuleBulkOptionsCard card;
    card.module = readRequiredModuleOrAll(reader, 1, "id");
    card.kind = readKeyword(reader, 2, "kind", TYPE_WORDS, ModuleBulkOptionsType::Primary);
    card.sourceModuleId = reader.integer(3, "sourceModuleId", 0);
    card.method = readKeyword(reader, 4, "method", MODULE_BULK_METHOD_WORDS, ModuleBulkOptionsMethod::Manual);
    card.tolerance = reader.real(5, "tolerance", 0.0);
    if (!reader.isBlank(6))
        reader.error("card.bad-value",
                     "field 6 of an " + std::string(reader.cardName()) + " is not used and must be blank", "");
    card.unitNumber = reader.integer(7, "unitNumber", 0);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleBulkOptionsFormat::check(const ModuleBulkOptionsCard& card)
{
    if (const auto problem = checkModuleOrAll(card.module, "id", false))
        return problem;
    if (card.sourceModuleId < 0)
        return CardProblem{"card.bad-id", "{field} must be zero or a positive module number", "sourceModuleId"};
    if (const auto problem = checkReal(card.tolerance, "tolerance", RealRule::Finite))
        return problem;
    if (card.unitNumber < 0)
        return CardProblem{"card.bad-id", "{field} must be zero or a positive unit number", "unitNumber"};
    // MDBULK,ALL sets METHOD and TOL for the modules that have no entry of their own, so it has no type and
    // no reference module.
    if (card.module.all)
    {
        if (card.kind != ModuleBulkOptionsType::Primary)
            return CardProblem{"card.bad-value", "a card with ALL has no {field}", "kind"};
        if (card.sourceModuleId != 0)
            return CardProblem{"card.bad-value", "a card with ALL has no {field}", "sourceModuleId"};
    }
    return std::nullopt;
}

void ModuleBulkOptionsFormat::write(const ModuleBulkOptionsCard& card, FieldWriter& writer)
{
    writeModuleOrAll(writer, 1, card.module, false);
    writeKeywordUnlessDefault(writer, 2, TYPE_WORDS, card.kind, ModuleBulkOptionsType::Primary);
    writer.integerUnlessDefault(3, card.sourceModuleId, 0);
    writeKeywordUnlessDefault(writer, 4, MODULE_BULK_METHOD_WORDS, card.method, ModuleBulkOptionsMethod::Manual);
    writer.realUnlessDefault(5, card.tolerance, 0.0);
    writer.integerUnlessDefault(7, card.unitNumber, 0);
}

std::uint32_t ModuleBulkOptionsTable::append(const ModuleBulkOptionsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(cardId(card));
    m_cards.push(card);
    return row;
}

void ModuleBulkOptionsTable::store(std::uint32_t row, const ModuleBulkOptionsCard& card)
{
    m_id.set(row, cardId(card));
    m_cards.set(row, card);
}

void ModuleBulkOptionsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleBulkOptionsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).module = ModuleOrAll::fromId(id);
}

} // namespace oor::model
