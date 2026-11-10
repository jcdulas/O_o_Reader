#pragma once

#include "oor/base/ids/CardKey.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardTypeInfo.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"
#include "oor/model/storage/RowBook.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace oor::model
{

class Model;

// What a card table may need from the reader once the whole Bulk Data of a model is read (CardTable::finishRead).
struct ReadContext
{
    base::DiagnosticSink& sink; // the diagnostics of the read: a table reports what it finds wrong here
};

// Hands out card keys, one per card created (the model's counter).
class KeySource
{
public:
    virtual ~KeySource() = default;
    [[nodiscard]] virtual base::CardKey next() = 0;
};

// All the cards of one type, stored by column (structure of arrays). Every card type has its own table class, written
// by hand: its columns, how a row is read from text, written to text, copied, and which fields are references. The
// model works on tables only through this interface, so it handles every card type the same way.
class CardTable
{
public:
    virtual ~CardTable() = default;

    [[nodiscard]] virtual const CardTypeInfo& info() const noexcept = 0;

    [[nodiscard]] RowBook& rows() noexcept { return m_rows; }
    [[nodiscard]] const RowBook& rows() const noexcept { return m_rows; }

    // The ID of the card in `row` (the set ID of a grouped card, 0 for a card with no ID).
    [[nodiscard]] virtual base::EntityId idAt(std::uint32_t row) const = 0;
    virtual void setIdAt(std::uint32_t row, base::EntityId id) = 0;

    // Reads a card from its fields. Most cards give one row; a card that defines several things (CORD1R: two
    // coordinate systems) gives one row for each, so that every row is one object with one ID. Each row gets a key
    // from `keys`; their rows are put in `rows`. Returns false when the card cannot be read (the reader then holds
    // the error diagnostics and no row was added).
    [[nodiscard]] virtual bool readCards(FieldReader& reader, KeySource& keys, std::vector<std::uint32_t>& rows) = 0;

    // Writes the card in `row` as fields (the card name is the table's).
    virtual void writeCard(std::uint32_t row, FieldWriter& writer) const = 0;

    // A card type that gives one row per object of a list (SPOINT 1 THRU 1000) can write consecutive rows as one card.
    // It says so with writesRuns(); continuesRun(previous, row) tells whether `row` goes on from the row before it
    // (the IDs follow each other). When the model is written with `compress_thru`, the writer finds the runs of three
    // rows or more once (rows that are alive, have consecutive card keys, and carry no comment and no placement of
    // their own), writes each of them with writeRun(first row, number of rows) in place of the cards of the rows, and
    // writes the other rows with writeCard. Nothing by default.
    [[nodiscard]] virtual bool writesRuns() const noexcept { return false; }
    [[nodiscard]] virtual bool continuesRun(std::uint32_t /*previous*/, std::uint32_t /*row*/) const { return false; }
    virtual void writeRun(std::uint32_t row, std::uint32_t /*count*/, FieldWriter& writer) const
    {
        writeCard(row, writer);
    }

    // Calls the visitor for each reference field of the card in `row`, and stores the ID it returns.
    virtual void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) = 0;

    // True for a card type whose cards define excitation set IDs (LSEQ: its EXCITEID): an EXCITEID of a dynamic load
    // that names one of them resolves (format reference: with a LOADSET command the load of an EXCITEID is the one its
    // LSEQ picks). definedExcitationId() gives the ID the card in `row` defines (0 for none). False and 0 by default.
    [[nodiscard]] virtual bool definesExcitationIds() const noexcept { return false; }
    [[nodiscard]] virtual base::EntityId definedExcitationId(std::uint32_t /*row*/) const { return 0; }

    // Called once for every table of a model (and of each of its parts), after the whole Bulk Data of that model is
    // read, in registry order. A card type whose cards take defaults from another card that may come later in the deck
    // (GRID from GRDSET, CBAR from BAROR) applies them here to the fields that were blank. Not called for a model made
    // in memory: a card added afterwards does not get the defaults. Nothing by default.
    virtual void finishRead(Model& /*model*/, const ReadContext& /*context*/) {}

    // True when another card of `model` gives defaults to blank fields of the cards of this type when a deck is read
    // (the GRDSET of the GRIDs, the BAROR of the CBARs, the BEAMOR of the CBEAMs). The writer then writes the fields
    // those defaults govern even when they hold the default or zero (FieldWriter::governedFieldsExplicit), so that
    // reading what was written gives the same cards. False by default.
    [[nodiscard]] virtual bool fieldsGovernedByDefaults(const Model& /*model*/) const { return false; }

    // Drops the dead rows from every column.
    virtual void compact() = 0;

    // A copy that shares every column buffer with this table (copy on write), for Model::copy().
    [[nodiscard]] virtual std::unique_ptr<CardTable> clone() const = 0;

    // Puts the card of the last row in `row`, in place of the card there, and tombstones the last row: how an edited
    // card takes the place of the card it replaces (the reader appends the new card; this moves it back). The card
    // keeps the key of `row`; the appended row must have a fresh key, so keys stay in increasing order
    // (RowBook::findRow). Every other row keeps its number and its source precision, so the ID indexes of the model
    // stay valid. Edited values discard their source precision.
    virtual void moveLastRowTo(std::uint32_t row) = 0;

protected:
    CardTable() = default;
    CardTable(const CardTable&) = default;
    CardTable& operator=(const CardTable&) = default;

    RowBook m_rows;
};

// Creates an empty table of one card type.
using CardTableFactory = std::unique_ptr<CardTable> (*)();

} // namespace oor::model
