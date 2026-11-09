#include "oor/model/cards/materials/viscoElasticMat/ViscoElasticMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIXED_TERM_COUNT = 5;
constexpr int LINE = 8;
constexpr int FIRST_TERM_FIELD = 9;

constexpr const char* TERM_NAMES[4] = {"shearModulus", "deviatoricRelaxationTime", "bulkModulus",
                                       "volumetricRelaxationTime"};
constexpr const char* ISOLIN_NAMES[4] = {"shortTimeShearModulus", "longTimeShearModulus", "decayConstant", "viscosity"};
constexpr const char* ORTHO_NAMES[10] = {
    "deviatoricRelaxationTime", "youngsModulusX", "youngsModulusY", "youngsModulusZ", "poissonRatioXY",
    "poissonRatioYZ",           "poissonRatioZX", "shearModulusXY", "shearModulusYZ", "shearModulusZX"};

// The four fields of a term of the fixed layout: the first term is in fields 5 to 8, the others (index from 1) have
// their G and TD on the second line and their K and TV on the third.
void fixedTermFields(std::size_t term, int (&fields)[4])
{
    if (term == 0)
    {
        fields[0] = 5;
        fields[1] = 6;
        fields[2] = 7;
        fields[3] = 8;
        return;
    }
    const int offset = 2 * static_cast<int>(term - 1);
    fields[0] = 9 + offset;
    fields[1] = 10 + offset;
    fields[2] = 17 + offset;
    fields[3] = 18 + offset;
}

// The fields of a term of ORTHO that the term starting at `first` uses: seven on its first line, three on its second.
void orthoTermFields(int first, int (&fields)[10])
{
    for (int index = 0; index < 7; ++index)
        fields[index] = first + index;
    for (int index = 0; index < 3; ++index)
        fields[7 + index] = first + LINE + index;
}

bool blockHasValues(const FieldReader& reader, int first, int count)
{
    for (int field = first; field < first + count; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

bool isFinite(const std::array<double, 4>& values)
{
    for (const double value : values)
    {
        if (!std::isfinite(value))
            return false;
    }
    return true;
}

} // namespace

bool ViscoElasticMatFormat::isFixedLayout(ViscoElasticMatModel model) noexcept
{
    return model == ViscoElasticMatModel::Iso || model == ViscoElasticMatModel::Mooney ||
           model == ViscoElasticMatModel::Ogden || model == ViscoElasticMatModel::Foam;
}

const char* ViscoElasticMatFormat::modelWord(ViscoElasticMatModel model) noexcept
{
    switch (model)
    {
    case ViscoElasticMatModel::Iso:
        return "IsotropicForm";
    case ViscoElasticMatModel::Iso1:
        return "IsotropicForm1";
    case ViscoElasticMatModel::Mooney:
        return "MooneyRivlin";
    case ViscoElasticMatModel::Ogden:
        return "Ogden";
    case ViscoElasticMatModel::Foam:
        return "Foam";
    case ViscoElasticMatModel::Mooney1:
        return "MooneyRivlinModel1";
    case ViscoElasticMatModel::Ogden1:
        return "OgdenModel1";
    case ViscoElasticMatModel::Foam1:
        return "FoamMaterial1";
    case ViscoElasticMatModel::Ortho:
        return "OrthotropicForm";
    case ViscoElasticMatModel::IsoLin:
        return "IsotropicLinear";
    }
    return "IsotropicForm1";
}

std::optional<ViscoElasticMatModel> ViscoElasticMatFormat::parseModel(std::string_view text) noexcept
{
    static constexpr ViscoElasticMatModel ALL[] = {ViscoElasticMatModel::Iso,    ViscoElasticMatModel::Iso1,
                                                   ViscoElasticMatModel::Mooney, ViscoElasticMatModel::Ogden,
                                                   ViscoElasticMatModel::Foam,   ViscoElasticMatModel::Mooney1,
                                                   ViscoElasticMatModel::Ogden1, ViscoElasticMatModel::Foam1,
                                                   ViscoElasticMatModel::Ortho,  ViscoElasticMatModel::IsoLin};
    if ((text == "Linear"))
        return ViscoElasticMatModel::Iso1;
    for (const ViscoElasticMatModel model : ALL)
    {
        if ((text == modelWord(model)))
            return model;
    }
    return std::nullopt;
}

std::optional<ViscoElasticMatCard> ViscoElasticMatFormat::read(FieldReader& reader)
{
    ViscoElasticMatCard card;
    card.id = reader.requiredInteger(1, "id");
    if (const std::string_view word = reader.text(2); !word.empty())
    {
        if (const auto model = parseModel(word))
            card.model = *model;
        else
            reader.error("card.bad-value",
                         std::string("model") + " is not one the card knows: \"" + std::string(word) + "\"", "model");
    }
    const int last = reader.lastUsedField();
    if (card.model == ViscoElasticMatModel::IsoLin)
    {
        // The SOL 700 form: the four constants of the model in fields 3 to 6.
        std::array<double, 4> term{};
        for (std::size_t index = 0; index < 4; ++index)
            term[index] = reader.real(3 + static_cast<int>(index), ISOLIN_NAMES[index], 0.0);
        requireBlankFields(reader, {}, 6);
        card.terms.push_back(term);
    }
    else if (card.model == ViscoElasticMatModel::Ortho)
    {
        // Two lines a term: seven values on the first, three on the second.
        requireBlankFields(reader, {3, 4, 5, 6, 7, 8}, last);
        for (int first = FIRST_TERM_FIELD; first <= last && !reader.failed(); first += 2 * LINE)
        {
            if (!blockHasValues(reader, first, 2 * LINE))
                continue;
            int fields[10];
            orthoTermFields(first, fields);
            std::array<double, 10> term{};
            for (std::size_t index = 0; index < 10; ++index)
                term[index] = reader.real(fields[index], ORTHO_NAMES[index], 0.0);
            for (const int gap :
                 {first + 7, first + LINE + 3, first + LINE + 4, first + LINE + 5, first + LINE + 6, first + LINE + 7})
            {
                if (!reader.isBlank(gap))
                {
                    reader.error("card.bad-value",
                                 "field " + std::to_string(gap) + " of a " + std::string(reader.cardName()) +
                                     " of model ORTHO is not used and must be blank",
                                 "model");
                    break;
                }
            }
            card.orthoTerms.push_back(term);
        }
    }
    else if (isFixedLayout(card.model))
    {
        card.solidThermalExpansion = reader.real(3, "solidThermalExpansion", 0.0);
        card.liquidThermalExpansion = reader.real(4, "liquidThermalExpansion", 0.0);
        std::size_t count = 0;
        std::array<std::array<double, 4>, FIXED_TERM_COUNT> terms{};
        for (std::size_t term = 0; term < FIXED_TERM_COUNT; ++term)
        {
            int fields[4];
            fixedTermFields(term, fields);
            bool given = false;
            for (std::size_t index = 0; index < 4; ++index)
            {
                given = given || !reader.isBlank(fields[index]);
                terms[term][index] = reader.real(fields[index], TERM_NAMES[index], 0.0);
            }
            if (given)
                count = term + 1;
        }
        card.terms.assign(terms.begin(), terms.begin() + static_cast<std::ptrdiff_t>(count));
        requireBlankFields(reader, {}, 24);
    }
    else
    {
        // One line a term; the first line holds only the model and the two coefficients.
        card.solidThermalExpansion = reader.real(3, "solidThermalExpansion", 0.0);
        card.liquidThermalExpansion = reader.real(4, "liquidThermalExpansion", 0.0);
        requireBlankFields(reader, {5, 6, 7, 8}, last);
        for (int first = FIRST_TERM_FIELD; first <= last && !reader.failed(); first += LINE)
        {
            if (!blockHasValues(reader, first, LINE))
                continue;
            std::array<double, 4> term{};
            for (std::size_t index = 0; index < 4; ++index)
                term[index] = reader.real(first + static_cast<int>(index), TERM_NAMES[index], 0.0);
            for (int gap = first + 4; gap < first + LINE; ++gap)
            {
                if (!reader.isBlank(gap))
                {
                    reader.error("card.bad-value",
                                 "field " + std::to_string(gap) + " of a " + std::string(reader.cardName()) +
                                     " is not used and must be blank",
                                 "model");
                    break;
                }
            }
            card.terms.push_back(term);
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ViscoElasticMatFormat::check(const ViscoElasticMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!std::isfinite(card.solidThermalExpansion) || !std::isfinite(card.liquidThermalExpansion))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("liquidThermalExpansion") + " must be finite numbers",
                           "solidThermalExpansion"};
    if (card.model == ViscoElasticMatModel::IsoLin)
    {
        if (!card.orthoTerms.empty())
            return CardProblem{"card.bad-value", "the orthotropic terms belong to the ORTHO model", "youngsModulusX"};
        if (card.solidThermalExpansion != 0.0 || card.liquidThermalExpansion != 0.0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " and " +
                                   CardProblem::fieldNamed("liquidThermalExpansion") +
                                   " are not used by the ISOLIN model",
                               "solidThermalExpansion"};
        if (card.terms.size() != 1)
            return CardProblem{"card.bad-value",
                               "an ISOLIN card has one term (" + std::string(CardProblem::FIELD) + ", " +
                                   CardProblem::fieldNamed("longTimeShearModulus") + ", " +
                                   CardProblem::fieldNamed("decayConstant") + ", " +
                                   CardProblem::fieldNamed("viscosity") + ")",
                               "shortTimeShearModulus"};
        for (std::size_t index = 0; index < 4; ++index)
        {
            if (!std::isfinite(card.terms[0][index]) || card.terms[0][index] < 0.0)
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                                   ISOLIN_NAMES[index]};
        }
        return std::nullopt;
    }
    if (card.model == ViscoElasticMatModel::Ortho)
    {
        if (!card.terms.empty())
            return CardProblem{"card.bad-value", "the terms of an ORTHO card are its orthotropic terms",
                               "shearModulus"};
        if (card.solidThermalExpansion != 0.0 || card.liquidThermalExpansion != 0.0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " and " +
                                   CardProblem::fieldNamed("liquidThermalExpansion") +
                                   " are not used by the ORTHO model",
                               "solidThermalExpansion"};
        for (const std::array<double, 10>& term : card.orthoTerms)
        {
            for (std::size_t index = 0; index < 10; ++index)
            {
                if (!std::isfinite(term[index]))
                    return CardProblem{"card.bad-value", "the values of a term must be finite numbers",
                                       ORTHO_NAMES[index]};
                // The Poisson ratios (VXY, VYZ, VZX) are the only values that may be negative.
                if ((index < 4 || index > 6) && term[index] < 0.0)
                    return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                                       ORTHO_NAMES[index]};
            }
        }
        return std::nullopt;
    }
    if (!card.orthoTerms.empty())
        return CardProblem{"card.bad-value", "the orthotropic terms belong to the ORTHO model", "youngsModulusX"};
    if (isFixedLayout(card.model) && card.terms.size() > static_cast<std::size_t>(FIXED_TERM_COUNT))
        return CardProblem{"card.bad-value",
                           "this model has the fixed layout of at most five terms (use ISO1, MOONEY1, "
                           "OGDEN1 or FOAM1 for more)",
                           "shearModulus"};
    for (const std::array<double, 4>& term : card.terms)
    {
        if (!isFinite(term))
            return CardProblem{"card.bad-value", "the values of a term must be finite numbers", "shearModulus"};
        for (std::size_t index = 0; index < 4; ++index)
        {
            if (term[index] < 0.0)
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                                   TERM_NAMES[index]};
        }
    }
    return std::nullopt;
}

void ViscoElasticMatFormat::write(const ViscoElasticMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, modelWord(card.model), "IsotropicForm1");
    if (card.model == ViscoElasticMatModel::IsoLin)
    {
        if (!card.terms.empty())
        {
            for (std::size_t index = 0; index < 4; ++index)
                writer.real(3 + static_cast<int>(index), card.terms[0][index]);
        }
        return;
    }
    if (card.model == ViscoElasticMatModel::Ortho)
    {
        int first = FIRST_TERM_FIELD;
        for (const std::array<double, 10>& term : card.orthoTerms)
        {
            int fields[10];
            orthoTermFields(first, fields);
            for (std::size_t index = 0; index < 10; ++index)
                writer.real(fields[index], term[index]);
            first += 2 * LINE;
        }
        return;
    }
    writer.realUnlessDefault(3, card.solidThermalExpansion, 0.0);
    writer.realUnlessDefault(4, card.liquidThermalExpansion, 0.0);
    if (isFixedLayout(card.model))
    {
        for (std::size_t term = 0; term < card.terms.size(); ++term)
        {
            int fields[4];
            fixedTermFields(term, fields);
            for (std::size_t index = 0; index < 4; ++index)
                writer.real(fields[index], card.terms[term][index]);
        }
        return;
    }
    int first = FIRST_TERM_FIELD;
    for (const std::array<double, 4>& term : card.terms)
    {
        for (std::size_t index = 0; index < 4; ++index)
            writer.real(first + static_cast<int>(index), term[index]);
        first += LINE;
    }
}

namespace
{

// The terms of a card as the flat list of the column, and back.
template <std::size_t N>
std::vector<double> flatten(const std::vector<std::array<double, N>>& terms)
{
    std::vector<double> flat;
    flat.reserve(terms.size() * N);
    for (const std::array<double, N>& term : terms)
        flat.insert(flat.end(), term.begin(), term.end());
    return flat;
}

template <std::size_t N>
std::vector<std::array<double, N>> gather(std::span<const double> flat)
{
    std::vector<std::array<double, N>> terms(flat.size() / N);
    for (std::size_t term = 0; term < terms.size(); ++term)
    {
        for (std::size_t index = 0; index < N; ++index)
            terms[term][index] = flat[term * N + index];
    }
    return terms;
}

} // namespace

std::uint32_t ViscoElasticMatTable::append(const ViscoElasticMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_model.push(static_cast<std::uint8_t>(card.model));
    m_solidThermalExpansion.push(card.solidThermalExpansion);
    m_liquidThermalExpansion.push(card.liquidThermalExpansion);
    m_terms.push(flatten(card.terms));
    m_orthoTerms.push(flatten(card.orthoTerms));
    return row;
}

ViscoElasticMatCard ViscoElasticMatTable::card(std::uint32_t row) const
{
    ViscoElasticMatCard card;
    card.id = m_id.get(row);
    card.model = static_cast<ViscoElasticMatModel>(m_model.get(row));
    card.solidThermalExpansion = m_solidThermalExpansion.get(row);
    card.liquidThermalExpansion = m_liquidThermalExpansion.get(row);
    card.terms = gather<4>(m_terms.get(row));
    card.orthoTerms = gather<10>(m_orthoTerms.get(row));
    return card;
}

void ViscoElasticMatTable::store(std::uint32_t row, const ViscoElasticMatCard& card)
{
    m_id.set(row, card.id);
    m_model.set(row, static_cast<std::uint8_t>(card.model));
    m_solidThermalExpansion.set(row, card.solidThermalExpansion);
    m_liquidThermalExpansion.set(row, card.liquidThermalExpansion);
    m_terms.set(row, flatten(card.terms));
    m_orthoTerms.set(row, flatten(card.orthoTerms));
}

void ViscoElasticMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_model.keepRows(rows);
    m_solidThermalExpansion.keepRows(rows);
    m_liquidThermalExpansion.keepRows(rows);
    m_terms.keepRows(rows);
    m_orthoTerms.keepRows(rows);
}

void ViscoElasticMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the material that the card extends.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
}

} // namespace oor::model
