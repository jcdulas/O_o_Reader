#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"
#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadTable.hpp"
#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The line direction word of a FacePressureLoad (Norm, the normal, by default).
const char* lineDirectionWord(model::LineLoadDirection direction) noexcept
{
    switch (direction)
    {
    case model::LineLoadDirection::X:
        return "XDirection";
    case model::LineLoadDirection::Y:
        return "YDirection";
    case model::LineLoadDirection::Z:
        return "ZDirection";
    case model::LineLoadDirection::Tangent:
        return "Tangent";
    case model::LineLoadDirection::Normal:
        break;
    }
    return "Norm";
}

// What every row of one pressure card shares in a set: the set, the card, its pressures at the corners (as the card
// gives them; a row holds them times `factor`), its system and vector (a PLOAD4; NaN: none), the factor of the
// expansion and the SORL and LDIR words (a PLOAD4; null for the other cards). The names outlive the table.
struct PressureCard
{
    base::EntityId set = 0;
    const char* name = "ShellPressureLoad";
    std::array<double, 4> values{};
    std::int32_t system = 0;
    bool systemKnown = false;
    std::array<double, 3> vector{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
    double factor = 1.0;
    const char* surfaceOrLine = nullptr;
    const char* lineDirection = nullptr;
};

// The rows of the pressure cards of the sets listed, in the order of the table: for each set, its cards in their
// order, for each card its elements in increasing IDs (a PLOAD one row for the face of its grid points). `sink` gets
// begin(card), then row(element, elementKnown, face) for each row (face: the corners of the face of a solid, the grid
// points of a PLOAD; empty for a shell), then end().
template <typename Sink>
void walkPressures(const model::Model& model, const LoadSetListing& listing, std::span<const std::uint16_t> types,
                   const std::optional<std::vector<std::int64_t>>& onlyElements, const CardList& elementList,
                   Sink& sink)
{
    std::vector<std::int32_t> face;
    for (const base::EntityId set : listing.sets())
    {
        for (const analysis::ElementaryLoad& load : listing.loadsOf(set, types))
        {
            if (load.kind != analysis::LoadKind::Pressure)
                continue;
            const std::uint16_t type = load.card.table;
            PressureCard card;
            card.set = set;
            card.factor = load.scale;
            if (type == model::typeIndexOf<model::PointsPressureLoadTable>())
            {
                if (onlyElements)
                    continue;
                const model::PointsPressureLoadCard data =
                    static_cast<const model::PointsPressureLoadTable&>(*model.findTable(type)).card(load.card.row);
                face.clear();
                for (const base::EntityId node : data.nodes)
                {
                    if (node != 0)
                        face.push_back(node);
                }
                card.name = "PointsPressureLoad";
                card.values = {data.pressure, data.pressure, data.pressure, data.pressure};
                sink.begin(card);
                sink.row(0, false, face);
                sink.end();
                continue;
            }
            base::EntityId node1 = 0;
            base::EntityId node3Or4 = 0;
            if (type == model::typeIndexOf<model::ShellPressureLoadTable>())
            {
                const double pressure =
                    static_cast<const model::ShellPressureLoadTable&>(*model.findTable(type)).pressureAt(load.card.row);
                card.values = {pressure, pressure, pressure, pressure};
            }
            else
            {
                const model::FacePressureLoadCard data =
                    static_cast<const model::FacePressureLoadTable&>(*model.findTable(type)).card(load.card.row);
                card.name = "FacePressureLoad";
                card.values = data.pressures;
                card.system = data.frameId;
                card.systemKnown = true;
                if (data.direction[0] != 0.0 || data.direction[1] != 0.0 || data.direction[2] != 0.0)
                    card.vector = data.direction;
                node1 = data.node1;
                node3Or4 = data.node3Or4;
                card.surfaceOrLine = data.kind == model::PressureKind::Line ? "Line" : "Surface";
                card.lineDirection = lineDirectionWord(data.lineDirection);
            }
            sink.begin(card);
            for (const analysis::IdRun& range : load.elements)
            {
                const std::span<const std::int32_t> inRun = idsInRun(elementList.ids, range.first, range.last);
                const auto first = static_cast<std::size_t>(inRun.data() - elementList.ids.data());
                for (std::size_t at = first; at < first + inRun.size(); ++at)
                {
                    const std::int32_t element = elementList.ids[at];
                    if (onlyElements && !std::binary_search(onlyElements->begin(), onlyElements->end(),
                                                            static_cast<std::int64_t>(element)))
                        continue;
                    face.clear();
                    // the card of the element from the list (no look-up of its ID: a row of a million pressures)
                    if (const model::CardLocation location = elementList.location(at);
                        analysis::dimensionOf(analysis::ElementTopology::shapeOf(location.table)) == 3)
                    {
                        if (const auto found = pressureFace(model, location, node1, node3Or4))
                        {
                            for (const base::EntityId node : *found)
                            {
                                if (node != 0)
                                    face.push_back(node);
                            }
                        }
                        else
                        {
                            if (node1 != 0)
                                face.push_back(node1);
                            if (node3Or4 != 0)
                                face.push_back(node3Or4);
                        }
                    }
                    sink.row(element, true, face);
                }
            }
            sink.end();
        }
    }
}

// The rows of pressureTable, a row an element and card (the default form).
class PressureRows
{
public:
    explicit PressureRows(std::size_t rows)
    {
        if (rows == 0)
            return;
        // unexpanded: a row a card at least (millions of PLOAD2 of one element)
        for (auto* column : {&m_sid, &m_eid})
            column->reserve(rows);
        for (auto* column : {&m_eidValid, &m_faceValid, &m_cidValid, &m_wordsValid})
            column->reserve(rows);
        for (auto* column : {&m_card, &m_sorl, &m_ldir})
            column->reserve(rows);
        for (auto* column : {&m_pressures[0], &m_pressures[1], &m_pressures[2], &m_pressures[3], &m_direction[0],
                             &m_direction[1], &m_direction[2], &m_scale})
            column->reserve(rows);
        m_cid.reserve(rows);
        m_faceOffsets.reserve(rows + 1);
    }

    void begin(const PressureCard& card) { m_card0 = &card; }
    void end() {}
    void row(std::int32_t element, bool elementKnown, const std::vector<std::int32_t>& face)
    {
        const PressureCard& card = *m_card0;
        m_sorl.emplace_back(card.surfaceOrLine != nullptr ? std::string_view(card.surfaceOrLine) : std::string_view());
        m_ldir.emplace_back(card.lineDirection != nullptr ? std::string_view(card.lineDirection) : std::string_view());
        m_wordsValid.push_back(card.surfaceOrLine != nullptr ? 1 : 0);
        m_sid.push_back(card.set);
        m_card.emplace_back(card.name);
        m_eid.push_back(element);
        m_eidValid.push_back(elementKnown ? 1 : 0);
        m_faceNodes.insert(m_faceNodes.end(), face.begin(), face.end());
        m_faceOffsets.push_back(static_cast<std::int64_t>(m_faceNodes.size()));
        m_faceValid.push_back(face.empty() ? 0 : 1);
        for (std::size_t item = 0; item < 4; ++item)
            m_pressures[item].push_back(card.values[item] * card.factor);
        m_cid.push_back(card.system);
        m_cidValid.push_back(card.systemKnown ? 1 : 0);
        for (std::size_t item = 0; item < 3; ++item)
            m_direction[item].push_back(card.vector[item]);
        m_scale.push_back(card.factor);
    }

    [[nodiscard]] std::shared_ptr<const QueryTable> finish()
    {
        TableBuilder builder;
        builder.int32("setId", std::move(m_sid));
        builder.dictionary("card", m_card);
        builder.int32("elementId", std::move(m_eid), std::move(m_eidValid));
        builder.intList("faceNodes", std::move(m_faceOffsets), std::move(m_faceNodes), std::move(m_faceValid));
        static const char* const NAMES[4] = {"p1", "p2", "p3", "p4"};
        for (std::size_t item = 0; item < 4; ++item)
            builder.real(NAMES[item], std::move(m_pressures[item]));
        builder.int32("frameId", std::move(m_cid), std::move(m_cidValid));
        builder.real("directionX", std::move(m_direction[0]));
        builder.real("directionY", std::move(m_direction[1]));
        builder.real("directionZ", std::move(m_direction[2]));
        builder.real("scale", std::move(m_scale));
        builder.dictionary("surfaceOrLine", m_sorl, m_wordsValid);
        builder.dictionary("loadDirection", m_ldir, std::move(m_wordsValid));
        return builder.finish();
    }

private:
    const PressureCard* m_card0 = nullptr;
    std::vector<std::int32_t> m_sid;
    std::vector<std::string_view> m_card; // names of card types and words: they outlive the table
    std::vector<std::int32_t> m_eid;
    std::vector<std::uint8_t> m_eidValid;
    std::vector<std::int64_t> m_faceOffsets{0};
    std::vector<std::int32_t> m_faceNodes;
    std::vector<std::uint8_t> m_faceValid;
    std::array<std::vector<double>, 4> m_pressures;
    std::vector<std::int32_t> m_cid;
    std::vector<std::uint8_t> m_cidValid;
    std::array<std::vector<double>, 3> m_direction;
    std::vector<double> m_scale;
    std::vector<std::string_view> m_sorl;
    std::vector<std::string_view> m_ldir;
    std::vector<std::uint8_t> m_wordsValid;
};

// The bits of a double, so that two values compare equal exactly when they are the same value (NaN, -0.0).
std::uint64_t bitsOf(double value) noexcept
{
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

bool sameBits(const std::array<double, 4>& left, const std::array<double, 4>& right) noexcept
{
    for (std::size_t item = 0; item < 4; ++item)
    {
        if (bitsOf(left[item]) != bitsOf(right[item]))
            return false;
    }
    return true;
}

// The compact form of pressureTable (detail="fields", a companion package): the rows of consecutive cards that share
// all but their element and their pressures, a field each. The elements of a field are runs of consecutive IDs (first,
// last), given once for every field that loads the same elements in the same order (`elementList`); its pressures
// once for the field when every row has the same (`isUniform`), else one a row, and one a row and corner when the
// four corners differ (`cornerCount` 4, else 1). A row on the face of a solid, or a PLOAD (a face of grid points), is
// a field of its own with its face. Expanded in order, the fields are the rows of the default form to the bit.
class PressureFields
{
public:
    void begin(const PressureCard& card)
    {
        m_card = &card;
        for (std::size_t item = 0; item < 4; ++item)
            m_values[item] = card.values[item] * card.factor;
        m_pending.clear();
    }
    void row(std::int32_t element, bool elementKnown, const std::vector<std::int32_t>& face)
    {
        if (elementKnown && face.empty())
        {
            m_pending.push_back(element);
            return;
        }
        // a face of its own: the rows before it, then it alone
        flushPending();
        close();
        m_open = Field{*m_card, m_values, true, {}, {}};
        if (elementKnown)
            m_open->elements.push_back(element);
        m_open->face = face;
        m_open->faceRow = true;
        m_open->elementKnown = elementKnown;
        close();
    }
    void end() { flushPending(); }

    [[nodiscard]] std::shared_ptr<const QueryTable> finish()
    {
        close();
        // the table keeps these vectors: their slack would stay with it (up to as much again for the values)
        m_valueItems.shrink_to_fit();
        m_runs.shrink_to_fit();
        TableBuilder builder;
        builder.int32("setId", std::move(m_sid));
        builder.dictionary("card", m_cardName);
        builder.int64("elementCount", std::move(m_elementCount));
        builder.int32("elementList", std::move(m_elementList), std::move(m_elementListValid));
        builder.intList("eidRuns", std::move(m_runOffsets), std::move(m_runs));
        builder.intList("faceNodes", std::move(m_faceOffsets), std::move(m_faceNodes), std::move(m_faceValid));
        builder.flag("isUniform", m_uniform);
        builder.int32("cornerCount", std::move(m_cornerCount));
        builder.realList("values", std::move(m_valueOffsets), std::move(m_valueItems));
        builder.int32("frameId", std::move(m_cid), std::move(m_cidValid));
        builder.real("directionX", std::move(m_direction[0]));
        builder.real("directionY", std::move(m_direction[1]));
        builder.real("directionZ", std::move(m_direction[2]));
        builder.real("scale", std::move(m_scale));
        builder.dictionary("surfaceOrLine", m_sorl, m_wordsValid);
        builder.dictionary("loadDirection", m_ldir, std::move(m_wordsValid));
        return builder.finish();
    }

private:
    struct Field
    {
        PressureCard card;                           // what its rows share (`values` unused)
        std::array<double, 4> first{};               // the pressures of its first row, times the factor
        bool uniform = true;                         // every row has `first`
        std::vector<std::array<double, 4>> values{}; // one a row once it is not uniform
        std::vector<std::int32_t> elements{};        // its rows' elements, in order
        std::vector<std::int32_t> face{};            // a face row: its face
        bool faceRow = false;                        // a row on a face of its own
        bool elementKnown = true;                    // a face row with an element (not a PLOAD)
    };

    [[nodiscard]] static bool shareAll(const PressureCard& left, const PressureCard& right) noexcept
    {
        // the names and words are literals: the same text is the same pointer
        return left.set == right.set && left.name == right.name && left.system == right.system &&
               left.systemKnown == right.systemKnown && bitsOf(left.vector[0]) == bitsOf(right.vector[0]) &&
               bitsOf(left.vector[1]) == bitsOf(right.vector[1]) && bitsOf(left.vector[2]) == bitsOf(right.vector[2]) &&
               bitsOf(left.factor) == bitsOf(right.factor) && left.surfaceOrLine == right.surfaceOrLine &&
               left.lineDirection == right.lineDirection;
    }

    // The rows of the card on shells since the last face: they go into the open field when they share its pressures,
    // or, a pressure a row, when they are few and the field holds a pressure a row already or is short (a ramp written
    // a card an element: a field of their own would cost more than a value a row); else they open a field of their own.
    static constexpr std::size_t FEW_ROWS = 16; // a value a row costs 8 to 32 bytes, a field some 110
    void flushPending()
    {
        if (m_pending.empty())
            return;
        Field* open = m_open ? &*m_open : nullptr;
        const bool joins = open != nullptr && shareAll(open->card, *m_card);
        if (joins && open->uniform && sameBits(open->first, m_values))
        {
            open->elements.insert(open->elements.end(), m_pending.begin(), m_pending.end());
        }
        else if (joins && m_pending.size() <= FEW_ROWS && (!open->uniform || open->elements.size() <= FEW_ROWS))
        {
            if (open->uniform)
            {
                open->values.assign(open->elements.size(), open->first);
                open->uniform = false;
            }
            open->elements.insert(open->elements.end(), m_pending.begin(), m_pending.end());
            open->values.insert(open->values.end(), m_pending.size(), m_values);
        }
        else
        {
            close();
            m_open = Field{*m_card, m_values, true, {}, m_pending};
        }
        m_pending.clear();
    }

    // The runs of the elements of the open field, and the list they make (an earlier one when it is the same).
    void addElements(const std::vector<std::int32_t>& elements)
    {
        std::vector<std::int32_t> runs;
        for (std::size_t at = 0; at < elements.size();)
        {
            std::size_t end = at + 1;
            while (end < elements.size() &&
                   static_cast<std::int64_t>(elements[end]) == static_cast<std::int64_t>(elements[end - 1]) + 1)
                ++end;
            runs.push_back(elements[at]);
            runs.push_back(elements[end - 1]);
            at = end;
        }
        std::uint64_t hash = 14695981039346656037ULL;
        for (const std::int32_t value : runs)
            hash = (hash ^ static_cast<std::uint32_t>(value)) * 1099511628211ULL;
        auto& lists = m_listsOfHash[hash];
        for (const std::int32_t list : lists)
        {
            const auto [begin, end] = m_listRuns[static_cast<std::size_t>(list)];
            if (static_cast<std::size_t>(end - begin) == runs.size() &&
                std::equal(runs.begin(), runs.end(), m_runs.begin() + begin))
            {
                m_elementList.push_back(list);
                m_runOffsets.push_back(static_cast<std::int64_t>(m_runs.size()));
                return;
            }
        }
        const auto list = static_cast<std::int32_t>(m_listRuns.size());
        lists.push_back(list);
        const auto begin = static_cast<std::int64_t>(m_runs.size());
        m_runs.insert(m_runs.end(), runs.begin(), runs.end());
        m_listRuns.emplace_back(begin, static_cast<std::int64_t>(m_runs.size()));
        m_elementList.push_back(list);
        m_runOffsets.push_back(static_cast<std::int64_t>(m_runs.size()));
    }

    void close()
    {
        if (!m_open)
            return;
        const Field& field = *m_open;
        m_sid.push_back(field.card.set);
        m_cardName.emplace_back(field.card.name);
        m_elementCount.push_back(field.elementKnown ? static_cast<std::int64_t>(field.elements.size()) : 1);
        if (field.elementKnown)
        {
            addElements(field.elements);
            m_elementListValid.push_back(1);
        }
        else
        {
            m_elementList.push_back(0);
            m_elementListValid.push_back(0);
            m_runOffsets.push_back(static_cast<std::int64_t>(m_runs.size()));
        }
        m_faceNodes.insert(m_faceNodes.end(), field.face.begin(), field.face.end());
        m_faceOffsets.push_back(static_cast<std::int64_t>(m_faceNodes.size()));
        m_faceValid.push_back(field.faceRow && !field.face.empty() ? 1 : 0);
        m_uniform.push_back(field.uniform ? 1 : 0);
        const auto equalCorners = [](const std::array<double, 4>& values)
        {
            return bitsOf(values[1]) == bitsOf(values[0]) && bitsOf(values[2]) == bitsOf(values[0]) &&
                   bitsOf(values[3]) == bitsOf(values[0]);
        };
        bool oneCorner = equalCorners(field.first);
        for (const std::array<double, 4>& values : field.values)
            oneCorner = oneCorner && equalCorners(values);
        m_cornerCount.push_back(oneCorner ? 1 : 4);
        const auto put = [&](const std::array<double, 4>& values)
        {
            if (oneCorner)
                m_valueItems.push_back(values[0]);
            else
                m_valueItems.insert(m_valueItems.end(), values.begin(), values.end());
        };
        if (field.uniform)
            put(field.first);
        else
            for (const std::array<double, 4>& values : field.values)
                put(values);
        m_valueOffsets.push_back(static_cast<std::int64_t>(m_valueItems.size()));
        m_cid.push_back(field.card.system);
        m_cidValid.push_back(field.card.systemKnown ? 1 : 0);
        for (std::size_t item = 0; item < 3; ++item)
            m_direction[item].push_back(field.card.vector[item]);
        m_scale.push_back(field.card.factor);
        m_sorl.emplace_back(field.card.surfaceOrLine != nullptr ? std::string_view(field.card.surfaceOrLine)
                                                                : std::string_view());
        m_ldir.emplace_back(field.card.lineDirection != nullptr ? std::string_view(field.card.lineDirection)
                                                                : std::string_view());
        m_wordsValid.push_back(field.card.surfaceOrLine != nullptr ? 1 : 0);
        m_open.reset();
    }

    const PressureCard* m_card = nullptr;
    std::array<double, 4> m_values{}; // the pressures of the rows of the card, times its factor
    std::vector<std::int32_t> m_pending;
    std::optional<Field> m_open;
    std::unordered_map<std::uint64_t, std::vector<std::int32_t>> m_listsOfHash;
    std::vector<std::pair<std::int64_t, std::int64_t>> m_listRuns; // the runs of each list, in m_runs

    std::vector<std::int32_t> m_sid;
    std::vector<std::string_view> m_cardName;
    std::vector<std::int64_t> m_elementCount;
    std::vector<std::int32_t> m_elementList;
    std::vector<std::uint8_t> m_elementListValid;
    std::vector<std::int64_t> m_runOffsets{0};
    std::vector<std::int32_t> m_runs;
    std::vector<std::int64_t> m_faceOffsets{0};
    std::vector<std::int32_t> m_faceNodes;
    std::vector<std::uint8_t> m_faceValid;
    std::vector<std::uint8_t> m_uniform;
    std::vector<std::int32_t> m_cornerCount;
    std::vector<std::int64_t> m_valueOffsets{0};
    std::vector<double> m_valueItems;
    std::vector<std::int32_t> m_cid;
    std::vector<std::uint8_t> m_cidValid;
    std::array<std::vector<double>, 3> m_direction;
    std::vector<double> m_scale;
    std::vector<std::string_view> m_sorl;
    std::vector<std::string_view> m_ldir;
    std::vector<std::uint8_t> m_wordsValid;
};

// pressureTable: the pressures on elements and faces of the sets asked for.
class PressureTableQuery final : public QueryBase
{
public:
    PressureTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string detail = run.arguments.textOr("detail", "rows");
        if (detail != "rows" && detail != "fields")
            throw QueryError("pressureTable: detail is \"rows\" or \"fields\", not \"" + detail + "\"");
        const model::Model& model = *run.model;
        // the cards of every load set, read once for all the sets
        const analysis::LoadSetIndex loadSets(model);
        const LoadSetListing listing(model, loadSets, run.arguments,
                                     askedSets(model, run.arguments, "LOAD").value_or(allLoadSets(model)));
        const auto onlyElements = sortedIds(run.arguments.ids("elementIds"));
        const CardList elementList = listCards(model, typesOfCategory(model::CardCategory::Element));
        // the cards the table lists (the others of an unexpanded set are not read)
        const std::vector<std::uint16_t> types =
            sortedTypesOf<model::PointsPressureLoadTable, model::ShellPressureLoadTable,
                          model::FacePressureLoadTable>();
        if (detail == "fields")
        {
            PressureFields fields;
            walkPressures(model, listing, types, onlyElements, elementList, fields);
            return fields.finish();
        }
        PressureRows rows(listing.cardCount(types));
        walkPressures(model, listing, types, onlyElements, elementList, rows);
        return rows.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "pressureTable";
        info.family = "T8";
        info.description =
            "The pressures of the sets asked for (LOAD combinations expanded), card the card type (PointsPressureLoad, "
            "ShellPressureLoad, FacePressureLoad: a PLOAD, a PLOAD2, a PLOAD4): PLOAD2 and PLOAD4 give a row for each "
            "element "
            "they name (a THRU range gives the elements of the model in it), PLOAD one for the face of its grid points "
            "(elementId "
            "null). p1 to p4 are the pressures at the corners, already multiplied by `scale`, the product of the LOAD "
            "factors "
            "(PLOAD4 gives P1 to P4, PLOAD2 and PLOAD the one pressure four times); faceNodes is the face of a solid "
            "(null for a shell: the element is the face; for a solid the corners found from G1 and G3 or G4 of the "
            "card, "
            "else those two); frameId and directionX/y/z are the system and the vector of a PLOAD4 as given (null: the "
            "load is "
            "normal to the face; the vector is not converted to the basic system); sorl (Surface, Line) and ldir "
            "(Norm, "
            "XDirection, "
            "YDirection, ZDirection, Tangent) are the words of a PLOAD4, as given or by default (null for PLOAD and "
            "PLOAD2); "
            "a Line load acts on the edges of a CQUADR or CTRIAR only. elementIds keeps these elements. "
            "expandCombinations=False leaves the LOAD combinations unexpanded: each load set once, "
            "its own cards (setId the set of the card, scale 1), a combination asked for replaced by the sets it "
            "reaches; loadCombinationsTable gives its factors, and the rows of a set times its combinedFactor "
            "are the expanded rows of the combination that reaches it once, to the bit. detail=\"fields\" "
            "gives the same rows compact, a row a field: the rows of consecutive cards of a set that share all "
            "but their element and pressures (setId, card, frameId, direction, scale, sorl, ldir). elementCount is its "
            "rows; eidRuns the (first, last) runs of consecutive element IDs of its rows in their order, flattened, "
            "given on the first field of each elementList and empty on the others that load the same elements in the "
            "same order (elementList null for a PLOAD); values the pressures, once for the field when isUniform "
            "(every row the same), else one a row, cornerCount of them each (1 when the four corners are equal, "
            "else p1 to p4); a row on the face of a solid or a PLOAD is a field of its own with its faceNodes. The "
            "fields expanded in order are the rows of the default form to the bit.";
        info.detailParameter = "detail";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the load sets"),
                           parameter("elementIds", ParameterKind::IdList, "None", "keep these elements"),
                           expandCombinationsParameter(),
                           parameter("detail", ParameterKind::Text, "\"rows\"",
                                     "\"rows\" (a row an element and card) or \"fields\" (the same rows "
                                     "compact)")};
        info.defaultSort = "setId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("setId")
            .dictionary("card")
            .int32("elementId")
            .intList("faceNodes")
            .reals("p1 p2 p3 p4")
            .int32("frameId");
        columns.vector3("direction", "XYZ").real("scale").dictionary("surfaceOrLine").dictionary("loadDirection");
        info.columnSets.push_back(columns.build());
        ColumnListBuilder fields;
        fields.int32("setId").dictionary("card").int64("elementCount").int32("elementList").intList("eidRuns");
        fields.intList("faceNodes").flag("isUniform").int32("cornerCount").realList("values").int32("frameId");
        fields.vector3("direction", "XYZ").real("scale").dictionary("surfaceOrLine").dictionary("loadDirection");
        info.columnSets.push_back(fields.build("fields"));
        return info;
    }
};

} // namespace

void addPressureTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<PressureTableQuery>());
}

} // namespace oor::query
