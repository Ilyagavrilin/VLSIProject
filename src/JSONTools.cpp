#include "JSONTools.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace JSONHelper {

static int calculateDistance(const std::pair<int, int> &p1,
                             const std::pair<int, int> &p2) {
  return std::abs(p1.first - p2.first) + std::abs(p1.second - p2.second);
}

bool parseTechFile(const std::string &filename, float &bufC, float &bufR,
                   float &bufDel, float &unitR, float &unitC) {
  try {
    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open technology file: " << filename
                << std::endl;
      return false;
    }

    json techJson;
    file >> techJson;

    unitR = techJson["technology"]["unit_wire_resistance"];
    unitC = techJson["technology"]["unit_wire_capacitance"];

    const auto &module = techJson["module"][0];
    const auto &input = module["input"][0];

    bufC = input["C"];
    bufR = input["R"];
    bufDel = input["intrinsic_delay"];

    return true;
  } catch (const json::exception &e) {
    std::cerr << "JSON parsing error: " << e.what() << std::endl;
    return false;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return false;
  }
}

bool parseTestFile(const std::string &filename, std::vector<VG::Edge> &edges,
                   std::vector<VG::Params> &capsRATs,
                   std::map<int, std::pair<int, int>> &nodeCoords) {
  try {
    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open test file: " << filename << std::endl;
      return false;
    }

    json testJson;
    file >> testJson;

    std::map<int, std::string> nodeTypes;

    for (const auto &node : testJson["node"]) {
      int id = node["id"];
      int x = node["x"];
      int y = node["y"];
      std::string type = node["type"];

      nodeCoords[id] = {x, y};
      nodeTypes[id] = type;

      // terminal node
      if (type == "t") {
        float capacitance = node["capacitance"];
        float rat = node["rat"];

        capsRATs.push_back({capacitance, rat});
      }
    }

    // basic node, type "b"
    // TODO(Ilyagavrilin): Maybe push node driverID first
    int driverId = -1;
    for (const auto &[id, type] : nodeTypes) {
      if (type == "b") {
        driverId = id;
        break;
      }
    }

    if (driverId == -1) {
      std::cerr << "Error: Failed to find driver node" << std::endl;
      return false;
    }

    for (const auto &edge : testJson["edge"]) {
      int startId = edge["vertices"][0];
      int endId = edge["vertices"][1];

      std::vector<std::pair<int, int>> points;
      for (const auto &segment : edge["segments"]) {
        points.push_back({segment[0], segment[1]});
      }

      int totalLength = 0;
      for (size_t i = 0; i < points.size() - 1; i++) {
        totalLength += calculateDistance(points[i], points[i + 1]);
      }

      edges.push_back({startId, endId, totalLength});
    }

    return true;
  } catch (const json::exception &e) {
    std::cerr << "JSON parsing error: " << e.what() << std::endl;
    return false;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return false;
  }
}

bool generateOutputFile(const std::string &originalFilename,
                        const std::map<int, std::pair<int, int>> &nodeCoords,
                        const VG::Trace &bufferPlaces) {
  try {
    std::ifstream inFile(originalFilename);
    if (!inFile.is_open()) {
      std::cerr << "Error: Could not open original file for reading: "
                << originalFilename << std::endl;
      return false;
    }

    json originalJson;
    inFile >> originalJson;

    std::filesystem::path path(originalFilename);
    std::string outputFilename = path.stem().string() + "_out.json";

    json bufferTemplate;
    std::string bufferName;
    for (const auto &node : originalJson["node"]) {
      if (node["type"] == "b") {
        bufferTemplate = node;
        bufferName = node["name"];
        break;
      }
    }

    int maxNodeId = 0;
    for (const auto &node : originalJson["node"]) {
      maxNodeId = std::max(maxNodeId, static_cast<int>(node["id"]));
    }

    for (const auto &place : bufferPlaces) {
      json newBuffer = bufferTemplate;
      newBuffer["id"] = ++maxNodeId;

      auto it = nodeCoords.find(place.ID);
      if (it != nodeCoords.end()) {
        newBuffer["x"] = it->second.first;
        newBuffer["y"] = it->second.second;
      }

      originalJson["node"].push_back(newBuffer);
    }

    std::ofstream outFile(outputFilename);
    if (!outFile.is_open()) {
      std::cerr << "Error: Could not create output file: " << outputFilename
                << std::endl;
      return false;
    }

    outFile << std::setw(4) << originalJson << std::endl;
    std::cout << "Output written to " << outputFilename << std::endl;
    return true;
  } catch (const json::exception &e) {
    std::cerr << "JSON error: " << e.what() << std::endl;
    return false;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return false;
  }
}

} // namespace JSONHelper