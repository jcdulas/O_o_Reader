#pragma once

#include <memory>
#include <utility>

namespace oor::model
{

// Something a higher layer keeps next to a table and that must follow the table when the model is copied: the cache of
// node positions in the basic system lives with the GRID table, for example. The model layer knows nothing about what
// it is; it only copies it with the table (clone) so that the copy of a model has its own cache, in the state of the
// original at the time of the copy.
class TableAttachment
{
public:
    virtual ~TableAttachment() = default;
    [[nodiscard]] virtual std::unique_ptr<TableAttachment> clone() const = 0;

protected:
    TableAttachment() = default;
    TableAttachment(const TableAttachment&) = default;
    TableAttachment& operator=(const TableAttachment&) = default;
};

// The place of one attachment in a table. Copying a table copies the attachment (the clone shares the big buffers by
// copy on write, see the attachment's own clone). The slot is mutable on purpose: an attachment is a cache, and reading
// the geometry of a model that is const only fills it.
class AttachmentSlot
{
public:
    AttachmentSlot() = default;
    AttachmentSlot(const AttachmentSlot& other)
        : m_attachment(other.m_attachment ? other.m_attachment->clone() : nullptr)
    {
    }
    AttachmentSlot& operator=(const AttachmentSlot& other)
    {
        if (this != &other)
            m_attachment = other.m_attachment ? other.m_attachment->clone() : nullptr;
        return *this;
    }
    AttachmentSlot(AttachmentSlot&&) noexcept = default;
    AttachmentSlot& operator=(AttachmentSlot&&) noexcept = default;
    ~AttachmentSlot() = default;

    [[nodiscard]] TableAttachment* get() const noexcept { return m_attachment.get(); }
    void reset(std::unique_ptr<TableAttachment> attachment) { m_attachment = std::move(attachment); }

private:
    std::unique_ptr<TableAttachment> m_attachment;
};

} // namespace oor::model
