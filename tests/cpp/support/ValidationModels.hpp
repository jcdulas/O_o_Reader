#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace oor::testing
{

// The main model files under `folder` (validation/models): every .oos file that no Include record of another file
// names, sorted. An included file (a mesh shared by several models) is read with the models that include it.
inline std::vector<std::filesystem::path> validationModels(const std::filesystem::path& folder)
{
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder))
        if (entry.is_regular_file() && entry.path().extension() == ".oos")
            files.push_back(entry.path());
    std::set<std::filesystem::path> included;
    const std::string includeRecord = "Include;";
    for (const std::filesystem::path& file : files)
    {
        std::ifstream in(file, std::ios::binary);
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.starts_with(includeRecord))
                continue;
            std::string target = line.substr(includeRecord.size());
            while (!target.empty() && (target.back() == '\r' || target.back() == ' '))
                target.pop_back();
            included.insert((file.parent_path() / target).lexically_normal());
        }
    }
    std::vector<std::filesystem::path> models;
    for (const std::filesystem::path& file : files)
        if (!included.contains(file.lexically_normal()))
            models.push_back(file);
    std::sort(models.begin(), models.end());
    return models;
}

} // namespace oor::testing
