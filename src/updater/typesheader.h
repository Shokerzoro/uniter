#ifndef TYPESHEADER_H
#define TYPESHEADER_H

#include <filesystem>
#include <map>
#include <vector>

using Path = std::filesystem::path;
using PathMapper = std::map<std::filesystem::path, std::filesystem::path>;
using PathVector = std::vector<std::filesystem::path>;

#endif // TYPESHEADER_H
