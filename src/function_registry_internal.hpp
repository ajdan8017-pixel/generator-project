#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

using ModuleFunc = std::function<std::string(const std::vector<std::string>&)>;
