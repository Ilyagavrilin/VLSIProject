#pragma once

#include "BufferInsertVG.h"
#include <cmath>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace JSONHelper {
bool parseTechFile(const std::string &filename, float &bufC, float &bufR,
                   float &bufDel, float &unitR, float &unitC);

bool parseTestFile(const std::string &filename, std::vector<VG::Edge> &edges,
                   std::vector<VG::Params> &capsRATs,
                   std::map<int, std::pair<int, int>> &nodeCoords);

bool generateOutputFile(const std::string &originalFilename,
                        const std::map<int, std::pair<int, int>> &nodeCoords,
                        const VG::Trace &bufferPlaces);
} // namespace JSONHelper
