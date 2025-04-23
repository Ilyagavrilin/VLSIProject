#include "JSONTools.h"
#include "BufferInsertVG.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>

using json = nlohmann::json;

namespace JSONTools {
int calculateManhattanDistance(const std::vector<int> &p1,
                               const std::vector<int> &p2) {
  return std::abs(p2[0] - p1[0]) + std::abs(p2[1] - p1[1]);
}

int calculateSegmentLength(const std::vector<std::vector<int>> &segments) {
  int totalLength = 0;
  for (size_t i = 0; i < segments.size() - 1; ++i) {
    totalLength += calculateManhattanDistance(segments[i], segments[i + 1]);
  }
  return totalLength;
}

VG::TechParams parseTechFile(const std::string &filename) {
  VG::TechParams wireParams;

  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open tech file: " + filename);
  }

  json techData;
  file >> techData;

  // Parse unit wire parameters
  wireParams.R = techData["technology"]["unit_wire_resistance"];
  wireParams.C = techData["technology"]["unit_wire_capacitance"];
  wireParams.IntrinsicDel = 0.0f; // Wire has no intrinsic delay

  return wireParams;
}

VG::TechParams parseBufferParams(const std::string &filename) {
  VG::TechParams bufferParams;

  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open tech file: " + filename);
  }

  json techData;
  file >> techData;

  // Parse buffer parameters
  if (techData["module"].size() > 0) {
    auto &module = techData["module"][0];
    if (module["input"].size() > 0) {
      auto &input = module["input"][0];
      bufferParams.C = input["C"];
      bufferParams.R = input["R"];
      bufferParams.IntrinsicDel = input["intrinsic_delay"];
    }
  }

  return bufferParams;
}

InputData parseTestFile(const std::string &filename) {
  InputData data;

  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open test file: " + filename);
  }

  json testData;
  file >> testData;

  // Parse nodes
  for (const auto &node : testData["node"]) {
    InputNode inputNode;
    inputNode.id = node["id"];
    inputNode.x = node["x"];
    inputNode.y = node["y"];
    inputNode.type = node["type"];
    inputNode.name = node["name"];

    if (node.contains("capacitance")) {
      inputNode.capacitance = node["capacitance"];
    }

    if (node.contains("rat")) {
      inputNode.rat = node["rat"];
    }

    data.nodes.push_back(inputNode);
  }

  for (const auto &edge : testData["edge"]) {
    InputEdge inputEdge;
    inputEdge.id = edge["id"];

    for (const auto &vertex : edge["vertices"]) {
      inputEdge.vertices.push_back(vertex);
    }

    for (const auto &segment : edge["segments"]) {
      std::vector<int> point;
      for (const auto &coord : segment) {
        point.push_back(coord);
      }
      inputEdge.segments.push_back(point);
    }

    data.edges.push_back(inputEdge);
  }

  return data;
}

void convertToVGStructures(const InputData &inputData,
                           std::vector<VG::Edge> &edges,
                           std::vector<VG::Node> &nodes) {
  edges.clear();
  nodes.clear();

  std::map<int, int> originalToNewId;

  int newId = 0;
  for (const auto &inputNode : inputData.nodes) {
    if (inputNode.type == "b") {
      originalToNewId[inputNode.id] = newId++;
      break;
    }
  }

  if (originalToNewId.empty()) {
    throw std::runtime_error(
        "No driver (buffer) node found in the input file!");
  }
  int count = 0;
  for (const auto &inputNode : inputData.nodes) {
    if (inputNode.type == "t") {
      originalToNewId[inputNode.id] = newId++;
      count++;
    }
  }

  for (const auto &inputNode : inputData.nodes) {
    if (inputNode.type == "s") {
      originalToNewId[inputNode.id] = newId++;
    }
  }

  std::cout << "Node ID Mapping (Original -> New):" << std::endl;
  for (const auto &mapping : originalToNewId) {
    std::cout << "  Original ID: " << mapping.first
              << " -> New ID: " << mapping.second << std::endl;
  }

  for (const auto &inputNode : inputData.nodes) {
    int newNodeId = originalToNewId[inputNode.id];
    if (newNodeId >= 0 && newNodeId < newId && inputNode.type == "t") {
      VG::Params params;
      params.C = inputNode.capacitance;
      params.RAT = inputNode.rat;
      nodes.emplace_back(newNodeId, params);
#ifdef DEBUG
      std::cout << "Node: " << std::endl;
      std::cout << newNodeId << " | " << inputNode.type << " | ";
      for (const auto &parm: nodes.back().CapsRATs) {
          std::cout << parm.C << " | " << parm.RAT << std::endl;
      }
#endif
    }
  }

  for (const auto &inputEdge : inputData.edges) {
    if (inputEdge.vertices.size() < 2) {
      std::cerr << "Warning: Edge " << inputEdge.id
                << " has fewer than 2 vertices, skipping." << std::endl;
      continue;
    }

    VG::Edge edge;

    int originalStart = inputEdge.vertices[0];
    int originalEnd = inputEdge.vertices[1];

    if (originalToNewId.find(originalStart) == originalToNewId.end() ||
        originalToNewId.find(originalEnd) == originalToNewId.end()) {
      std::cerr << "Warning: Edge " << inputEdge.id
                << " references unknown node IDs, skipping." << std::endl;
      continue;
    }

    edge.Start = originalToNewId[originalStart];
    edge.End = originalToNewId[originalEnd];

    int length = 0;
    for (size_t i = 0; i < inputEdge.segments.size() - 1; ++i) {
      const auto &p1 = inputEdge.segments[i];
      const auto &p2 = inputEdge.segments[i + 1];

      if (p1.size() >= 2 && p2.size() >= 2) {
        length += std::abs(p2[0] - p1[0]) + std::abs(p2[1] - p1[1]);
      }
    }
    edge.Len = length;
    edge.IsVisited = false;

    edges.push_back(edge);
  }
#ifdef DEBUG
  std::cout << "Edges (Start -> End, Length):" << std::endl;
  for (const auto &edge : edges) {
    std::cout << "  " << edge.Start << " -> " << edge.End
              << ", Length: " << edge.Len << std::endl;
  }
#endif
}

void writeOutputFile(const std::string &originalFilename,
                     const InputData &originalData, const VG::Trace &trace) {
  std::string outputFilename =
      originalFilename.substr(0, originalFilename.find_last_of('.')) +
      "_out.json";

  json outputJson;
#ifdef DEBUG
  for (const auto &trace_elem : trace) {
    std::cout << "ID: " << trace_elem.ID << " Is buff: " << trace_elem.IsBuffer
              << std::endl;
  }
#endif
  json nodeArray = json::array();
  for (const auto &node : originalData.nodes) {
    json nodeJson;
    nodeJson["id"] = node.id;
    nodeJson["x"] = node.x;
    nodeJson["y"] = node.y;
    nodeJson["type"] = node.type;
    nodeJson["name"] = node.name;

    if (node.type == "t") {
      nodeJson["capacitance"] = node.capacitance;
      nodeJson["rat"] = node.rat;
    }

    nodeArray.push_back(nodeJson);
  }

  json edgeArray = json::array();
  for (const auto &edge : originalData.edges) {
    json edgeJson;
    edgeJson["id"] = edge.id;
    edgeJson["vertices"] = edge.vertices;
    edgeJson["segments"] = edge.segments;

    edgeArray.push_back(edgeJson);
  }

  outputJson["node"] = nodeArray;
  outputJson["edge"] = edgeArray;

  std::ofstream outFile(outputFilename);
  if (!outFile.is_open()) {
    throw std::runtime_error("Could not open output file: " + outputFilename);
  }

  outFile << outputJson.dump(4); // Pretty print with 4-space indent
  std::cout << "Output written to " << outputFilename << std::endl;
}

} // namespace JSONTools