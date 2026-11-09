#include "oor/model/cards/dynamics/frfComponent/FrfComponentTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/dynamics/FrfComponentReference.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <string>
#include <string_view>

namespace oor::model
{

namespace
{

// The words the Case Control FRF command's FrfAssemblyOutput keyword reserves: a component may not be called by
// one.
constexpr std::array<std::string_view, 4> RESERVED_NAMES{"ConnectionInfo", "All", "Component", "Assembly"};

// The words of the MEDIUM field, in the order of FrfMedium.
constexpr std::array<std::string_view, 3> MEDIUM_WORDS{"Database", "BinaryResultsFile", "UniversalFile"};

bool isReservedName(std::string_view name)
{
    for (const std::string_view reserved : RESERVED_NAMES)
    {
        if ((reserved == name))
            return true;
    }
    return false;
}

} // namespace

std::optional<FrfComponentCard> FrfComponentFormat::read(FieldReader& reader)
{
    FrfComponentCard card;
    card.id = reader.requiredInteger(1, "id");
    card.name = std::string(reader.word(2));
    card.medium = readRequiredKeyword<FrfMedium>(reader, 3, "medium", MEDIUM_WORDS);
    card.unitNumber = reader.integer(4, "unitNumber", 0);
    card.lengthScaleFactor = reader.real(5, "lengthScaleFactor", 1.0);
    card.forceScaleFactor = reader.real(6, "forceScaleFactor", 1.0);
    if (reader.lastUsedField() > 6)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("forceScaleFactor"),
                     "forceScaleFactor");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FrfComponentFormat::check(const FrfComponentCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (!card.name.empty())
    {
        if (const auto problem = checkFrfName(card.name, "componentName"))
            return problem;
        if (isReservedName(card.name))
        {
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) +
                                   " may not be ConnectionInfo, All, Component or Assembly: the FrfAssemblyOutput "
                                   "keyword reserves them",
                               "componentName"};
        }
    }
    if (card.unitNumber < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "unitNumber"};
    // A binary or universal file is read through a unit that the FMS ASSIGN statement gives.
    if (card.unitNumber == 0 && card.medium != FrfMedium::Database)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " is required unless " +
                               CardProblem::fieldNamed("medium") + " is Database",
                           "unitNumber"};
    if (const auto problem = checkReal(card.lengthScaleFactor, "lengthScaleFactor", RealRule::Positive))
        return problem;
    return checkReal(card.forceScaleFactor, "forceScaleFactor", RealRule::Positive);
}

void FrfComponentFormat::write(const FrfComponentCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (!card.name.empty())
        writer.text(2, card.name);
    writeKeyword(writer, 3, MEDIUM_WORDS, card.medium);
    writer.integerUnlessDefault(4, card.unitNumber, 0);
    writer.realUnlessDefault(5, card.lengthScaleFactor, 1.0);
    writer.realUnlessDefault(6, card.forceScaleFactor, 1.0);
}

std::uint32_t FrfComponentTable::append(const FrfComponentCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_name.push(card.name);
    m_medium.push(static_cast<std::int32_t>(card.medium));
    m_unitNumber.push(card.unitNumber);
    m_lengthScaleFactor.push(card.lengthScaleFactor);
    m_forceScaleFactor.push(card.forceScaleFactor);
    return row;
}

FrfComponentCard FrfComponentTable::card(std::uint32_t row) const
{
    FrfComponentCard card;
    card.id = m_id.get(row);
    card.name = m_name.get(row);
    card.medium = static_cast<FrfMedium>(m_medium.get(row));
    card.unitNumber = m_unitNumber.get(row);
    card.lengthScaleFactor = m_lengthScaleFactor.get(row);
    card.forceScaleFactor = m_forceScaleFactor.get(row);
    return card;
}

void FrfComponentTable::store(std::uint32_t row, const FrfComponentCard& card)
{
    m_id.set(row, card.id);
    m_name.set(row, card.name);
    m_medium.set(row, static_cast<std::int32_t>(card.medium));
    m_unitNumber.set(row, card.unitNumber);
    m_lengthScaleFactor.set(row, card.lengthScaleFactor);
    m_forceScaleFactor.set(row, card.forceScaleFactor);
}

void FrfComponentTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_name.keepRows(rows);
    m_medium.keepRows(rows);
    m_unitNumber.keepRows(rows);
    m_lengthScaleFactor.keepRows(rows);
    m_forceScaleFactor.keepRows(rows);
}

} // namespace oor::model
