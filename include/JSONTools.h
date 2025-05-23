#pragma once

#include "BufferInsertVG.h"
#include <string>
#include <vector>

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

VG::TechParams parseTechFile(const std::string &filename);

VG::TechParams parseBufferParams(const std::string &filename);

InputData parseTestFile(const std::string &filename);

void convertToVGStructures(InputData &inputData, std::vector<VG::Edge> &edges,
                           std::vector<VG::Node> &nodes,
                           std::map<int, int> &originalToNewId,
                           std::map<int, int> &newToOriginalId);

void writeOutputFile(const std::string &originalFilename,
                     const InputData &originalData,
                     const std::vector<VG::BufPlace> &bufferLocations,
                     const std::map<int, int> &newToOriginalId);

std::vector<std::vector<int>>
extractSegmentsBetween(const std::vector<std::vector<int>> &segments,
                       int startDistanceFromChild, int endDistanceFromChild);

} // namespace JSONTools
