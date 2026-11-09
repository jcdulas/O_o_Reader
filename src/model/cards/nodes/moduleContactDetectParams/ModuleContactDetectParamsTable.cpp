#include "oor/model/cards/nodes/moduleContactDetectParams/ModuleContactDetectParamsTable.hpp"

#include "oor/model/cards/nonlinear_contact/contactPairGeomParams/ContactPairGeomParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPairPhysicalParams/ContactPairPhysicalParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactParams/ContactParamsTable.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

namespace
{

// The parameters the card allows, and DELSLD. The YES or NO ones are
// words; FTYPE is BLCOUL or BLSHEAR, or 6 or 7; FRIC is a real or the ID of a table.
constexpr ContactParameterRule OWN_RULES[] = {
    {"ContactKind", ContactParameterKind::Text},     {"DetectionDistance", ContactParameterKind::Real},
    {"BeamContact", ContactParameterKind::Text},     {"EdgeContact", ContactParameterKind::Text},
    {"FrictionKind", ContactParameterKind::Any},     {"FrictionCoefficient", ContactParameterKind::Number},
    {"IgnoreThickness", ContactParameterKind::Text}, {"InitialStiffness", ContactParameterKind::Text},
    {"SelfContact", ContactParameterKind::Text},     {"DeleteSolids", ContactParameterKind::Text},
};

// The card types whose keywords the names take, in the order of rules(): its own, then the borrowed ones.
constexpr std::string_view OWNERS[] = {ModuleContactDetectParamsTable::NAME, ContactParamsTable::NAME,
                                       ContactPairPhysicalParamsTable::NAME, ContactPairGeomParamsTable::NAME};

// Fields 1 and 2 are blank: the first parameter is in field 3, as in a BCAUTOP.
constexpr int FIRST_PAIR_FIELD = 3;

} // namespace

std::span<const ContactParameterRule> ModuleContactDetectParamsFormat::rules()
{
    // The card's own names come first, so that FTYPE and FRIC keep the kinds the format of MDBCATP gives them.
    static const std::vector<ContactParameterRule> ALL = []
    {
        std::vector<ContactParameterRule> rules(std::begin(OWN_RULES), std::end(OWN_RULES));
        for (const auto other : {ContactParamsFormat::rules(), ContactPairPhysicalParamsFormat::rules(),
                                 ContactPairGeomParamsFormat::rules()})
            rules.insert(rules.end(), other.begin(), other.end());
        return rules;
    }();
    return ALL;
}

std::optional<ModuleContactDetectParamsCard> ModuleContactDetectParamsFormat::read(FieldReader& reader)
{
    ModuleContactDetectParamsCard card;
    if (!reader.isBlank(1) || !reader.isBlank(2))
        reader.error(
            "card.bad-value",
            "fields 1 and 2 of an " + std::string(reader.cardName()) + " are blank: the first parameter is in field 3",
            "parameterName");
    card.params = readContactParameters(reader, FIRST_PAIR_FIELD, rules(), OWNERS);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleContactDetectParamsFormat::check(const ModuleContactDetectParamsCard& card)
{
    return checkContactParameters(card.params, rules(), OWNERS);
}

void ModuleContactDetectParamsFormat::write(const ModuleContactDetectParamsCard& card, FieldWriter& writer)
{
    writeContactParameters(writer, FIRST_PAIR_FIELD, card.params, OWNERS);
}

std::uint32_t ModuleContactDetectParamsTable::append(const ModuleContactDetectParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void ModuleContactDetectParamsTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-module-contact-detect-params",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data");
}

} // namespace oor::model
