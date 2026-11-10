#include "ModelEditingBindings.hpp"

#include "BindingSupport.hpp"
#include "oor/api/ControlEdits.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/control/case_control/CommandCatalogue.hpp"
#include "oor/model/control/executive_control/SolutionSequences.hpp"
#include "oor/model/format/CardLayouts.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace oor::wasm
{
namespace
{

model::FieldValue scalar(const val& value)
{
    using Kind = model::FieldValue::Kind;
    if (value.isNull() || value.isUndefined())
        return {};
    const std::string type = value.typeOf().as<std::string>();
    if (type == "string")
        return {Kind::Text, 0, 0, value.as<std::string>()};
    if (type == "number")
    {
        const double number = value.as<double>();
        if (!std::isfinite(number))
            throw base::UsageError("a numeric card field must be finite");
        if (std::trunc(number) == number && (number != 0 || !std::signbit(number)))
        {
            if (std::abs(number) > 9007199254740991.0)
                throw base::UsageError("an integer card field must be a safe JavaScript integer");
            return {Kind::Integer, static_cast<std::int64_t>(number), 0, {}};
        }
        return {Kind::Real, 0, number, {}};
    }
    // A real whose value is integral still has real semantics in an alternative field.
    if (type == "object" && value["real"].typeOf().as<std::string>() == "number")
    {
        const double number = value["real"].as<double>();
        if (std::isfinite(number))
            return {Kind::Real, 0, number, {}};
    }
    throw base::UsageError("a card field is a number, string, null or {real: number}");
}

api::CardInput cardInput(const val& object)
{
    api::CardInput input;
    input.type = object["type"].as<std::string>();
    input.form = option<std::string>(object, "form", {});
    if (!object["comment"].isUndefined() && !object["comment"].isNull())
        input.comment = object["comment"].as<std::string>();
    const val fields = object["fields"];
    const auto count = fields["length"].as<unsigned>();
    input.fields.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        input.fields.push_back(scalar(fields[i]));
    return input;
}

std::vector<api::CardInput> cardInputs(const val& array)
{
    std::vector<api::CardInput> inputs;
    const auto count = array["length"].as<unsigned>();
    inputs.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        inputs.push_back(cardInput(array[i]));
    return inputs;
}

val addedCards(const std::vector<api::AddedCard>& added)
{
    val array = val::array();
    for (const auto& card : added)
    {
        val object = val::object();
        object.set("type", card.type);
        object.set("id", card.id);
        object.set("key", card.key);
        array.call<void>("push", object);
    }
    return array;
}

val insertCards(api::Deck& deck, const val& array)
{
    return guarded([&] { return addedCards(api::insertCards(deck, cardInputs(array))); });
}

val appendInclude(api::Deck& deck, const std::string& path, const val& array)
{
    return guarded([&] { return addedCards(api::appendInclude(deck, path, cardInputs(array))); });
}

struct InputColumn
{
    unsigned field = 0;
    std::vector<model::FieldValue> values;
    std::vector<double> numbers;
    bool numeric = false;
    bool real = false;
    bool broadcast = false;
};

val insertBatch(api::Deck& deck, const std::string& type, unsigned count, const val& columns)
{
    return guarded(
        [&]
        {
            std::vector<InputColumn> input;
            unsigned fieldCount = 0;
            const auto columnCount = columns["length"].as<unsigned>();
            input.reserve(columnCount);
            for (unsigned i = 0; i < columnCount; ++i)
            {
                const val column = columns[i];
                InputColumn item;
                item.field = column["field"].as<unsigned>();
                if (item.field == 0 || item.field > 1000000)
                    throw base::UsageError("a batch field index must be between 1 and 1000000");
                fieldCount = std::max(fieldCount, item.field);
                item.broadcast = column["broadcast"].as<bool>();
                item.real = column["real"].as<bool>();
                const val data = column["data"];
                if (item.broadcast)
                    item.values.push_back(scalar(data));
                else if (val::global("ArrayBuffer").call<bool>("isView", data))
                {
                    // One transfer per numeric column, then the row loop stays entirely in C++.
                    item.numeric = true;
                    item.numbers = emscripten::convertJSArrayToNumberVector<double>(data);
                    if (item.numbers.size() != count)
                        throw base::UsageError("batch column lengths differ");
                    for (const double number : item.numbers)
                        if (!std::isfinite(number) ||
                            (!item.real && std::trunc(number) == number && std::abs(number) > 9007199254740991.0))
                            throw base::UsageError("a batch contains a nonfinite or unsafe numeric value");
                }
                else
                {
                    if (data["length"].as<unsigned>() != count)
                        throw base::UsageError("batch column lengths differ");
                    item.values.reserve(count);
                    for (unsigned row = 0; row < count; ++row)
                        item.values.push_back(scalar(data[row]));
                }
                input.push_back(std::move(item));
            }
            const auto added = api::insertCardBatch(
                deck, type, count,
                [&](std::size_t row)
                {
                    api::CardInput card;
                    card.type = type;
                    card.fields.resize(fieldCount);
                    for (const auto& column : input)
                    {
                        auto& value = card.fields[column.field - 1];
                        if (!column.numeric)
                            value = column.values[column.broadcast ? 0 : row];
                        else
                        {
                            const double number = column.numbers[row];
                            if (!column.real && std::trunc(number) == number && (number != 0 || !std::signbit(number)))
                                value = {model::FieldValue::Kind::Integer, static_cast<std::int64_t>(number), 0, {}};
                            else
                                value = {model::FieldValue::Kind::Real, 0, number, {}};
                        }
                    }
                    return card;
                });
            std::vector<std::int32_t> ids;
            std::vector<std::uint32_t> keys;
            ids.reserve(added.size());
            keys.reserve(added.size());
            for (const auto& card : added)
            {
                ids.push_back(card.id);
                keys.push_back(card.key);
            }
            val result = val::object();
            result.set("type", type);
            result.set("ids", int32Array(ids));
            result.set("keys", typedArray("Uint32Array", keys.data(), keys.size()));
            return result;
        });
}

std::vector<api::ControlRecord> controlRecords(const val& array)
{
    std::vector<api::ControlRecord> result;
    const auto count = array["length"].as<unsigned>();
    result.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        result.push_back({array[i]["name"].as<std::string>(), vectorOf<std::string>(array[i]["fields"]),
                          option<std::string>(array[i], "comment", {})});
    return result;
}

val caseValue(const model::CaseValue& value)
{
    using Kind = model::CaseValueKind;
    switch (value.kind)
    {
    case Kind::Integer:
        return val(static_cast<double>(value.integer));
    case Kind::Real:
        return val(value.real);
    case Kind::Keyword:
    case Kind::Text:
        return val(value.text);
    case Kind::IntegerList:
    {
        val array = val::array();
        for (auto number : value.integers)
            array.call<void>("push", static_cast<double>(number));
        return array;
    }
    case Kind::RealList:
        return float64Array(value.reals);
    case Kind::None:
        return val::null();
    }
    return val::null();
}

val caseEntry(const model::CaseEntry& entry)
{
    val result = val::object();
    result.set("key", entry.key);
    result.set("name", entry.name);
    result.set("kind", entry.kind == model::EntryKind::Output    ? "output"
                       : entry.kind == model::EntryKind::Set     ? "set"
                       : entry.kind == model::EntryKind::Param   ? "parameter"
                       : entry.kind == model::EntryKind::Command ? "command"
                                                                 : "metadata");
    result.set("value", caseValue(entry.value));
    result.set("loadSequence", entry.value.kind == model::CaseValueKind::Integer && !entry.value.integers.empty()
                                   ? val(static_cast<double>(entry.value.integers.front()))
                                   : val::null());
    result.set("variant", entry.variant);
    result.set("options", textArray(entry.options));
    result.set("number", static_cast<double>(entry.number));
    result.set("members", textArray(entry.set.items()));
    result.set("comment", entry.leadingComments);
    return result;
}

val caseSectionObject(const model::Subcase& section)
{
    val result = val::object();
    result.set("key", section.key);
    result.set("kind", std::string(model::subcaseKindName(section.kind)));
    result.set("id", static_cast<double>(section.id));
    val entries = val::array();
    for (const auto& entry : section.entries)
        entries.call<void>("push", caseEntry(entry));
    result.set("entries", entries);
    val children = val::array();
    for (const auto& child : section.children)
        children.call<void>("push", caseSectionObject(child));
    result.set("children", children);
    return result;
}

std::int64_t positiveId(double value, bool zero = false)
{
    if (!std::isfinite(value) || std::trunc(value) != value || value < (zero ? 0 : 1) || value > 9007199254740991.0)
        throw base::UsageError("a section ID must be a positive safe integer (zero for an unnumbered section)");
    return static_cast<std::int64_t>(value);
}

} // namespace

void bindModelEditing(emscripten::class_<api::Deck>& binding)
{
    emscripten::function("cardLayouts", +[] { return std::string(model::cardLayoutsText()); });
    emscripten::function(
        "solutionCatalogue",
        +[]
        {
            return guarded(
                []
                {
                    val result = val::array();
                    for (const auto& solution : model::SolutionSequences::all())
                    {
                        val item = val::object();
                        item.set("name", std::string(solution.name));
                        item.set("number", static_cast<double>(solution.number));
                        result.call<void>("push", item);
                    }
                    return result;
                });
        });
    emscripten::function(
        "caseCommandCatalogue",
        +[]
        {
            return guarded(
                []
                {
                    constexpr const char* shapes[] = {"integer",    "integerOrKeyword", "real",          "keyword",
                                                      "text",       "integerList",      "realList",      "output",
                                                      "flag",       "subcaseHeader",    "setDefinition", "parameter",
                                                      "blockHeader"};
                    val result = val::array();
                    for (const auto& definition : model::CommandCatalogue::instance().definitions())
                    {
                        val item = val::object();
                        item.set("name", definition.name);
                        item.set("shape", std::string(shapes[static_cast<unsigned>(definition.shape)]));
                        item.set("variants", textArray(definition.variants));
                        item.set("keywords", textArray(definition.keywords));
                        item.set("sectionKind", definition.shape == model::CommandShape::SubcaseHeader
                                                    ? std::string(model::subcaseKindName(definition.subcaseKind))
                                                    : std::string{});
                        item.set("blockKind", definition.blockKind);
                        item.set("inBlock", definition.isBlockCommand);
                        result.call<void>("push", item);
                    }
                    return result;
                });
        });
    binding.class_function(
               "createModel", +[] { return guarded([] { return api::Deck::createModel(); }); })
        .function(
            "copy", +[](api::Deck& deck) { return guarded([&] { return deck.copy(); }); })
        .function(
            "_insertCard",
            +[](api::Deck& deck, const val& input)
            {
                return guarded(
                    [&]
                    {
                        const auto card = api::insertCard(deck, cardInput(input));
                        val result = val::object();
                        result.set("type", card.type);
                        result.set("id", card.id);
                        result.set("key", card.key);
                        return result;
                    });
            })
        .function("_insertCards", &insertCards)
        .function("_insertCardBatch", &insertBatch)
        .function("_appendInclude", &appendInclude)
        .function(
            "_cardByKey",
            +[](api::Deck& deck, const std::string& type, std::uint32_t key, const val& computed)
            {
                return guarded(
                    [&]
                    {
                        const auto found = api::findCardByKey(deck, type, key, vectorOf<std::string>(computed));
                        return found ? cardObject(*found) : val::null();
                    });
            })
        .function(
            "_updateCard", +[](api::Deck& deck, const std::string& type, std::uint32_t key, const val& input)
                           { guarded([&] { api::updateCardByKey(deck, type, key, cardInput(input)); }); })
        .function(
            "_removeCard", +[](api::Deck& deck, const std::string& type, std::uint32_t key)
                           { return guarded([&] { return api::removeCardByKey(deck, type, key); }); })
        .function(
            "_setCardValues",
            +[](api::Deck& deck, const std::string& type, std::uint32_t key, const val& values)
            {
                guarded(
                    [&]
                    {
                        std::vector<std::pair<std::string, model::FieldValue>> input;
                        const auto names = vectorOf<std::string>(val::global("Object").call<val>("keys", values));
                        for (const auto& name : names)
                            input.emplace_back(name, scalar(values[name]));
                        api::setCardValuesByKey(deck, type, key, input);
                    });
            })
        .function(
            "_setCardValueByKey",
            +[](api::Deck& deck, const std::string& type, std::uint32_t key, const std::string& name,
                const std::string& value) { guarded([&] { api::setCardValueByKey(deck, type, key, name, value); }); })
        .function(
            "_setCardComment",
            +[](api::Deck& deck, const std::string& type, std::uint32_t key, const std::string& comment)
            { guarded([&] { api::setCardCommentByKey(deck, type, key, comment); }); })
        .function(
            "applyDefaults", +[](api::Deck& deck) { guarded([&] { deck.applyDefaults(); }); })
        .function(
            "checkReferences",
            +[](api::Deck& deck) { return guarded([&] { return diagnosticsArray(deck.checkReferences()); }); })
        .function(
            "compact", +[](api::Deck& deck) { guarded([&] { deck.model()->compact(); }); })
        .function(
            "_setSolution",
            +[](api::Deck& deck, const std::string& solution) { guarded([&] { api::setSolution(deck, solution); }); })
        .function(
            "_setExecutive", +[](api::Deck& deck, const val& records)
                             { guarded([&] { api::setExecutive(deck, controlRecords(records)); }); })
        .function(
            "_setCaseControl", +[](api::Deck& deck, const val& records)
                               { guarded([&] { api::setCaseControl(deck, controlRecords(records)); }); })
        .function(
            "caseRecords",
            +[](api::Deck& deck)
            {
                return guarded(
                    [&]
                    {
                        val result = val::array();
                        for (const auto& record : api::caseRecords(deck))
                        {
                            val item = val::object();
                            item.set("name", record.name);
                            item.set("fields", textArray(record.fields));
                            item.set("comment", record.comment);
                            result.call<void>("push", item);
                        }
                        return result;
                    });
            })
        .function(
            "executiveRecords",
            +[](api::Deck& deck)
            {
                return guarded(
                    [&]
                    {
                        val result = val::array();
                        for (const auto& record : api::executiveRecords(deck))
                        {
                            val item = val::object();
                            item.set("name", record.name);
                            item.set("fields", textArray(record.fields));
                            item.set("comment", record.comment);
                            result.call<void>("push", item);
                        }
                        return result;
                    });
            })
        .function(
            "_caseSections",
            +[](api::Deck& deck)
            {
                return guarded(
                    [&]
                    {
                        val result = val::array();
                        for (const auto& section : std::as_const(*deck.model()).control().caseControl.segments())
                            result.call<void>("push", caseSectionObject(section));
                        return result;
                    });
            })
        .function(
            "_caseGlobalKey",
            +[](api::Deck& deck) { return std::as_const(*deck.model()).control().caseControl.global().key; })
        .function(
            "_caseSection", +[](api::Deck& deck, std::uint32_t key)
                            { return guarded([&] { return caseSectionObject(api::caseSection(deck, key)); }); })
        .function(
            "_addCaseSection",
            +[](api::Deck& deck, const std::string& kind, double id, std::uint32_t parent, const val& records)
            {
                return guarded(
                    [&]
                    {
                        return api::addCaseSection(deck, kind, positiveId(id, kind == "auxiliaryCase"), parent,
                                                   controlRecords(records));
                    });
            })
        .function(
            "_setCaseCommands",
            +[](api::Deck& deck, std::uint32_t key, const val& records)
            {
                return guarded(
                    [&]
                    {
                        const auto keys = api::setCaseCommands(deck, key, controlRecords(records));
                        return typedArray("Uint32Array", keys.data(), keys.size());
                    });
            })
        .function(
            "_removeCaseEntry", +[](api::Deck& deck, std::uint32_t key)
                                { return guarded([&] { return api::removeCaseEntry(deck, key); }); })
        .function(
            "_removeCaseSection", +[](api::Deck& deck, std::uint32_t key)
                                  { return guarded([&] { return api::removeCaseSection(deck, key); }); })
        .function(
            "_renumberCaseSection", +[](api::Deck& deck, std::uint32_t key, double id)
                                    { guarded([&] { api::renumberCaseSection(deck, key, positiveId(id)); }); })
        .function(
            "_effectiveCaseCommand",
            +[](api::Deck& deck, std::uint32_t key, const std::string& name, const std::string& variant)
            {
                return guarded(
                    [&]
                    {
                        (void)api::caseSection(deck, key);
                        const auto* entry =
                            std::as_const(*deck.model()).control().caseControl.effectiveCommand(key, name, variant);
                        return entry ? caseEntry(*entry) : val::null();
                    });
            })
        .function(
            "_effectiveCaseOutputs",
            +[](api::Deck& deck, std::uint32_t key, const std::string& name)
            {
                return guarded(
                    [&]
                    {
                        (void)api::caseSection(deck, key);
                        val result = val::array();
                        for (const auto* entry :
                             std::as_const(*deck.model()).control().caseControl.effectiveOutputs(key, name))
                            result.call<void>("push", caseEntry(*entry));
                        return result;
                    });
            })
        .function(
            "_addPart",
            +[](api::Deck& deck, const std::string& kind, double id, double massId, bool appends)
            {
                return guarded(
                    [&]
                    {
                        return deck.addPart({kind, positiveId(id, kind == "ActivePart" || kind == "PassivePart"),
                                             positiveId(massId, true), appends});
                    });
            })
        .function(
            "_part",
            +[](api::Deck& deck, const std::string& kind, double id, double massId)
            {
                return guarded(
                    [&]
                    {
                        return deck.part(kind, positiveId(id, kind == "ActivePart" || kind == "PassivePart"),
                                         positiveId(massId, true));
                    });
            })
        .function(
            "parts",
            +[](api::Deck& deck)
            {
                return guarded(
                    [&]
                    {
                        val result = val::array();
                        for (const auto& part : deck.parts())
                        {
                            val item = val::object();
                            item.set("kind", part.kind);
                            item.set("id", static_cast<double>(part.id));
                            item.set("massId", static_cast<double>(part.massId));
                            item.set("appends", part.appends);
                            result.call<void>("push", item);
                        }
                        return result;
                    });
            });
}

} // namespace oor::wasm
