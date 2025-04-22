#pragma once

#include <string>
#include <vector>
#include "BufferInsertVG.h"
#include <nlohmann/json.hpp>

namespace JSONTools {

struct InputNode {
    int id;
    int x;
    int y;
    std::string type;
    std::string name;
    float capacitance = 0.0f;
    float rat = 0.0f;
};

struct InputEdge {
    int id;
    std::vector<int> vertices;
    std::vector<std::vector<int>> segments;
};

struct InputData {
    std::vector<InputNode> nodes;
    std::vector<InputEdge> edges;
};

VG::TechParams parseTechFile(const std::string& filename);

VG::TechParams parseBufferParams(const std::string& filename);

InputData parseTestFile(const std::string& filename);

void convertToVGStructures(const InputData& inputData, 
                          std::vector<VG::Edge>& edges,
                          std::vector<VG::Node>& nodes);

void writeOutputFile(const std::string& originalFilename, 
                    const InputData& originalData,
                    const VG::Trace& trace);

}  // namespace JSONTools
