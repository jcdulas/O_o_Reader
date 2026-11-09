#pragma once

#include "oor/deck/records/RecordText.hpp"
#include "oor/model/control/ControlRecords.hpp"

namespace oor::testing
{

class SectionText final : public model::RecordOutput
{
public:
    std::string text;
    void comments(const std::string& lines) override
    {
        std::size_t start = 0;
        while (start <= lines.size())
        {
            const auto end = lines.find('\n', start);
            text += "#" + lines.substr(start, end == std::string::npos ? end : end - start) + "\n";
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
    void record(const std::vector<std::string>& values) override { text += deck::RecordText::format(values) + "\n"; }
    void includeBegin(std::uint32_t) override {}
    void includeEnd() override {}
    void dropped(const std::string&) override {}
};

inline std::string caseText(const model::CaseControl& control)
{
    SectionText out;
    model::ControlRecords::writeCase(control, out);
    return out.text;
}

inline std::string executiveText(const model::ExecutiveControl& control)
{
    SectionText out;
    model::ControlRecords::writeExecutive(control, out);
    return out.text;
}

} // namespace oor::testing
