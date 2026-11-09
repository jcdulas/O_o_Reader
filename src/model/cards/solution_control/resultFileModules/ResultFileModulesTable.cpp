#include "oor/model/cards/solution_control/resultFileModules/ResultFileModulesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr int PAIRS_PER_LINE = 4; // MDIDi and UDIDi side by side in fields 2 to 9

// The format reference field of the MDIDi of the pair `position` (0 to 3) of a line; its UDIDi is the next field.
constexpr int pairFormatField(int position) noexcept
{
    return 2 + 2 * position;
}

} // namespace

std::optional<ResultFileModulesCard> ResultFileModulesFormat::read(FieldReader& reader)
{
    ResultFileModulesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.configuration = reader.integer(2, "configuration", 0);
    requireBlankLineFields(reader, 0, 4, 9, "first");
    const int lastLine = lastUsedLine(reader);
    for (int line = 1; line <= lastLine && !reader.failed(); ++line)
    {
        for (int position = 0; position < PAIRS_PER_LINE; ++position)
        {
            const int moduleField = fatigueField(line, pairFormatField(position));
            const int userNameField = moduleField + 1;
            if (reader.isBlank(moduleField) && reader.isBlank(userNameField))
                continue;
            ResultFileModulesModule module;
            module.moduleId = reader.requiredInteger(moduleField, "moduleIds");
            module.userNameId = reader.requiredInteger(userNameField, "userNameIds");
            card.modules.push_back(module);
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ResultFileModulesFormat::check(const ResultFileModulesCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.configuration, "configuration"))
        return problem;
    if (card.modules.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one pair of a module and its file, MDID0 and UDID0", "moduleIds"};
    for (std::size_t index = 0; index < card.modules.size(); ++index)
    {
        const ResultFileModulesModule& module = card.modules[index];
        if (module.moduleId < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "moduleIds"};
        if (const auto problem = checkRequiredId(module.userNameId, "userNameIds"))
            return problem;
        for (std::size_t earlier = 0; earlier < index; ++earlier)
        {
            if (card.modules[earlier].moduleId == module.moduleId)
                return CardProblem{"card.bad-value",
                                   "the module " + std::to_string(module.moduleId) +
                                       " has one file: " + std::string(CardProblem::FIELD) + " is listed once",
                                   "moduleIds"};
        }
    }
    return std::nullopt;
}

void ResultFileModulesFormat::write(const ResultFileModulesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.configuration > 0)
        writer.integer(2, card.configuration);
    for (std::size_t index = 0; index < card.modules.size(); ++index)
    {
        const int line = 1 + static_cast<int>(index) / PAIRS_PER_LINE;
        const int moduleField = fatigueField(line, pairFormatField(static_cast<int>(index) % PAIRS_PER_LINE));
        writer.integer(moduleField, card.modules[index].moduleId);
        writer.integer(moduleField + 1, card.modules[index].userNameId);
    }
}

std::uint32_t ResultFileModulesTable::append(const ResultFileModulesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ResultFileModulesTable::store(std::uint32_t row, const ResultFileModulesCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ResultFileModulesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ResultFileModulesTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ResultFileModulesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ResultFileModulesCard& card = m_cards.edit(row);
    visitId(visitor, "configuration", ReferenceTarget::UserDefinedName, card.configuration);
    for (ResultFileModulesModule& module : card.modules)
        visitId(visitor, "userNameIds", ReferenceTarget::UserDefinedName, module.userNameId);
}

} // namespace oor::model
