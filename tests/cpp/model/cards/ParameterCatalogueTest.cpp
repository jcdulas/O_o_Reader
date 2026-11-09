#include "oor/base/text/NumberParser.hpp"
#include "oor/model/cards/parameters/parameter/ParameterTable.hpp"
#include "oor/model/parameters/ParameterCatalogue.hpp"
#include "oor/model/parameters/ParameterValues.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;

namespace
{

ParameterCard textCard(const std::string& name, const std::string& text)
{
    ParameterCard card;
    card.name = name;
    card.kind = ParameterKind::Text;
    card.textValue = text;
    return card;
}

ParameterCard integerCard(const std::string& name, std::int32_t value)
{
    ParameterCard card;
    card.name = name;
    card.kind = ParameterKind::Integer;
    card.integerValue = value;
    return card;
}

ParameterCard realCard(const std::string& name, double value)
{
    ParameterCard card;
    card.name = name;
    card.kind = ParameterKind::Real;
    card.realValue = value;
    return card;
}

} // namespace

TEST_CASE("the catalogue knows the parameters of the format reference, by neutral name", "[model][parameters]")
{
    const ParameterCatalogue& catalogue = ParameterCatalogue::instance();
    CHECK(catalogue.entries().size() > 400);
    const ParamInfo* wtmass = catalogue.find("WeightToMass");
    REQUIRE(wtmass != nullptr);
    CHECK(wtmass->name == "WeightToMass");
    CHECK(catalogue.find("WTMASS") == nullptr);       // only neutral parameter names are accepted
    CHECK(catalogue.find("weightToMass") == nullptr); // a neutral name is exact
    CHECK(wtmass->type == ParamType::Real);
    CHECK(wtmass->defaultValue.kind == ParameterKind::Real);
    CHECK(wtmass->defaultValue.realValue == 1.0);
    const ParamInfo* autospc = catalogue.find("AutoSingularityConstr");
    REQUIRE(autospc != nullptr);
    CHECK(autospc->type == ParamType::Text);
    CHECK(autospc->defaultValue.textValue == "Yes");
    const ParamInfo* grdpnt = catalogue.find("WeightReferenceNode");
    REQUIRE(grdpnt != nullptr);
    CHECK(grdpnt->defaultValue.integerValue == -1);
    CHECK(catalogue.find("PostprocessingFile")->defaultValue.integerValue == 9999999);
    CHECK(catalogue.find("ShellDrillingStiffness")->defaultValue.realValue == 100.0);
    // a name the format reference gives with another: ALPHA1, ALPHA2 are two entries
    CHECK(catalogue.find("RayleighMassDamping") != catalogue.find("RayleighStiffnessDamping"));
    CHECK(catalogue.find("RayleighStiffnessDamping") != nullptr);
    // an exact name wins over the numbered one: StressSign and StressSignNode, DofSetPrintSets1 and DofSetPrintSets
    CHECK(catalogue.find("StressSignNode")->name == "StressSignNode");
    REQUIRE(catalogue.find("DofSetPrintSets3") != nullptr);
    CHECK(catalogue.find("DofSetPrintSets3")->name == "DofSetPrintSets");
    CHECK(catalogue.find("DofSetPrintSets3")->numbered);
    CHECK(catalogue.find("WeightToMass2") == nullptr); // WeightToMass is not a numbered name
    CHECK(catalogue.find("NOSUCHPARAMETER") == nullptr);
    CHECK(catalogue.find("") == nullptr);
    CHECK(catalogue.find("123") == nullptr);
}

TEST_CASE("every entry of the catalogue is consistent with itself", "[model][parameters]")
{
    std::set<std::string> names;
    for (const ParamInfo& info : ParameterCatalogue::instance().entries())
    {
        INFO(info.name);
        CHECK(names.insert(std::string(info.name)).second); // each name once
        CHECK(ParameterCatalogue::instance().find(info.name) == &info);
        const ParameterKind kind = info.defaultValue.kind;
        switch (info.type)
        {
        case ParamType::Integer:
            CHECK((kind == ParameterKind::Blank || kind == ParameterKind::Integer));
            break;
        case ParamType::Real:
            CHECK((kind == ParameterKind::Blank || kind == ParameterKind::Real));
            break;
        case ParamType::Number:
            CHECK((kind == ParameterKind::Blank || kind == ParameterKind::Integer || kind == ParameterKind::Real));
            break;
        case ParamType::Text:
            CHECK((kind == ParameterKind::Blank || kind == ParameterKind::Text));
            break;
        case ParamType::Complex:
            CHECK((kind == ParameterKind::Blank || kind == ParameterKind::Complex));
            break;
        case ParamType::Any:
            break;
        }
        // the options are values of the kind, and the default is one of them
        const std::vector<std::string_view> allowed = info.allowedValues();
        for (const std::string_view option : allowed)
        {
            if (info.type == ParamType::Integer || info.type == ParamType::Number)
                CHECK(base::NumberParser::parseInteger(option).has_value());
            else
            {
                CHECK(info.type == ParamType::Text);
                CHECK(!option.empty());
            }
        }
        if (!allowed.empty() && info.defaultValue.exists())
        {
            const ParameterCard card = ParameterCatalogue::defaultCard(info);
            CHECK(ParameterCatalogue::fit(info, card) == ParameterFit::Fits);
        }
    }
}

TEST_CASE("a parameter of the wrong kind or outside the listed options is found", "[model][parameters]")
{
    const ParameterCatalogue& catalogue = ParameterCatalogue::instance();
    CHECK_FALSE(catalogue.problemWith(realCard("WeightToMass", 0.0042)).has_value());
    // an integer is a real that is whole
    CHECK_FALSE(catalogue.problemWith(integerCard("WeightToMass", 1)).has_value());
    const auto word = catalogue.problemWith(textCard("WeightToMass", "YES"));
    REQUIRE(word.has_value());
    CHECK(word->fit == ParameterFit::WrongKind);
    CHECK(word->message ==
          "WeightToMass takes a real number, found the word YES"); // diagnostics use the neutral parameter name
    CHECK(catalogue.problemWith(integerCard("AutoSingularityConstr", 1))->fit == ParameterFit::WrongKind);
    CHECK(catalogue.problemWith(realCard("PostprocessingFile", -1.0))->fit == ParameterFit::WrongKind);
    CHECK_FALSE(catalogue.problemWith(integerCard("PostprocessingFile", -1)).has_value());
    const auto options = catalogue.problemWith(textCard("AcousticOutputKind", "LOUD"));
    REQUIRE(options.has_value());
    CHECK(options->fit == ParameterFit::NotAllowed);
    CHECK(options->message == "AcousticOutputKind takes one of Peak, RootMeanSquare, found the word LOUD");
    // Values use the neutral names of the catalogue.
    CHECK_FALSE(catalogue.problemWith(textCard("AcousticOutputKind", "RootMeanSquare")).has_value());
    CHECK(catalogue.problemWith(integerCard("AdjointMethod", 7))->fit == ParameterFit::NotAllowed);
    // a complex parameter takes a pair, or a lone real
    ParameterCard pair;
    pair.name = "StiffnessScale1";
    pair.kind = ParameterKind::Complex;
    CHECK_FALSE(catalogue.problemWith(pair).has_value());
    CHECK_FALSE(catalogue.problemWith(realCard("StiffnessScale1", 1.0)).has_value());
    CHECK(catalogue.problemWith(textCard("StiffnessScale1", "YES"))->fit == ParameterFit::WrongKind);
    // a parameter with no value, a name the format reference does not list, and a parameter of any kind are not judged
    ParameterCard blank;
    blank.name = "WeightToMass";
    CHECK_FALSE(catalogue.problemWith(blank).has_value());
    CHECK_FALSE(catalogue.problemWith(textCard("MYPARAM", "WHATEVER")).has_value());
    CHECK_FALSE(catalogue.problemWith(textCard("ConditionCheck", "NO")).has_value());
}

TEST_CASE("the reader keeps a PARAM of the wrong kind and warns", "[model][parameters]")
{
    const Model wrong = readModelText(
        "Parameter;WeightToMass;Yes\nParameter;WeightReferenceNode;0\nParameter;MYPARAM;Yes\nParameter;"
        "PostprocessingFile;-1.5\n");
    CHECK_FALSE(wrong.diagnostics().hasErrors());
    std::size_t warnings = 0;
    for (const base::Diagnostic& diagnostic : wrong.diagnostics().diagnostics())
    {
        if (diagnostic.code == "card.parameter-kind")
        {
            ++warnings;
            CHECK(diagnostic.severity == base::Severity::Warning);
        }
    }
    CHECK(warnings == 2); // WTMASS and POST
    CHECK(wrong.findTable<ParameterTable>()->rows().aliveCount() == 4);
    const Model fine = readModelText(
        "Parameter;WeightToMass;0.0042\nParameter;WeightToMass;1\nParameter;AutoSingularityConstr;No\nParameter;"
        "StiffnessScale1;1.;2.\n");
    CHECK(fine.diagnostics().diagnostics().empty());
}

TEST_CASE("a parameter has the value of the last PARAM card, else the default of the format reference",
          "[model][parameters]")
{
    const Model model = readModelText(
        "Parameter;WeightToMass;0.5\nParameter;WeightToMass;0.25\nParameter;AutoSingularityConstr;No\nParameter;"
        "ShellDrillingStiffness;Yes\nParameter;MYPARAM;7\n");
    const auto wtmass = parameterValue(model, "WeightToMass");
    REQUIRE(wtmass.has_value());
    CHECK(wtmass->fromDeck);
    CHECK(wtmass->card.realValue == 0.25);
    CHECK(realParameter(model, "WeightToMass") == 0.25);
    CHECK(parameterValue(model, "AutoSingularityConstr")->card.textValue == "No");
    // not in the deck: the default
    const auto grdpnt = parameterValue(model, "WeightReferenceNode");
    REQUIRE(grdpnt.has_value());
    CHECK_FALSE(grdpnt->fromDeck);
    CHECK(grdpnt->card.integerValue == -1);
    CHECK(integerParameter(model, "WeightReferenceNode") == -1);
    CHECK(integerParameter(model, "InertiaRelief") == 0);
    // a word where a real is wanted is not a number: the default stands
    CHECK(realParameter(model, "ShellDrillingStiffness") == 100.0);
    // a name the catalogue does not know has the value of its card, or nothing
    CHECK(parameterValue(model, "MYPARAM")->card.integerValue == 7);
    CHECK_FALSE(parameterValue(model, "OTHER").has_value());
    // a known parameter with no default in the format reference has nothing either
    CHECK_FALSE(parameterValue(model, "OutputMaxRatio").has_value());
    // no PARAM at all
    const Model empty = readModelText("Node;1;;0.;0.;0.\n");
    CHECK(realParameter(empty, "WeightToMass") == 1.0);
    CHECK(parameterValue(empty, "HeatStaticAnalysis")->card.textValue == "No");
}

TEST_CASE("a parameter preserves its neutral name through serialization", "[model][parameters]")
{
    const Model model = readModelText(
        "Parameter;WeightToMass;0.5\nParameter;DofSetPrintSets3;R\nParameter;MYPARAM;7\nParameter;ShellQuad4Formulation;"
        "MITC4\n");
    const ParameterTable& table = *model.findTable<ParameterTable>();
    CHECK(table.card(0).name == "WeightToMass");
    CHECK(table.card(1).name == "DofSetPrintSets3");     // a numbered name keeps its number
    CHECK(table.card(2).name == "MYPARAM");              // an arbitrary name is kept as written
    CHECK(table.card(3).name == "ShellQuad4Formulation"); // this name is outside the parameter catalogue
    const std::string written = oor::testing::writeModelText(model);
    CHECK(written.find("WeightToMass") != std::string::npos);
    CHECK(written.find("DofSetPrintSets3") != std::string::npos);
    CHECK(written.find("MYPARAM") != std::string::npos);
    CHECK(written.find("ShellQuad4Formulation") != std::string::npos);
    oor::testing::requireModelFileRoundTrip(model);
}
