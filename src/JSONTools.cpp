#include "JSONTools.h"
#include "BufferInsertVG.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>

// #define DEBUG
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

void convertToVGStructures(InputData &inputData, std::vector<VG::Edge> &edges,
                           std::vector<VG::Node> &nodes,
                           std::map<int, int> &originalToNewId,
                           std::map<int, int> &newToOriginalId) {
  edges.clear();
  nodes.clear();
  originalToNewId.clear();
  newToOriginalId.clear();

  int newId = 0;
  int driverId = -1;
  for (auto &inputNode : inputData.nodes) {
    if (inputNode.type == "b") {
      originalToNewId[inputNode.id] = newId;
      newToOriginalId[newId] = inputNode.id;
      driverId = inputNode.id;
      newId++;
      break;
    }
  }

  if (driverId == -1) {
    throw std::runtime_error(
        "No driver (buffer) node found in the input file!");
  }

  for (const auto &inputNode : inputData.nodes) {
    if (inputNode.type == "t") {
      originalToNewId[inputNode.id] = newId;
      newToOriginalId[newId] = inputNode.id;
      newId++;
    }
  }

  for (const auto &inputNode : inputData.nodes) {
    if (inputNode.type == "s") {
      originalToNewId[inputNode.id] = newId;
      newToOriginalId[newId] = inputNode.id;
      newId++;
    }
  }
#ifdef DEBUG
  std::cout << "Node ID Mapping (Original -> New):" << std::endl;
  for (const auto &mapping : originalToNewId) {
    std::cout << "  Original ID: " << mapping.first
              << " -> New ID: " << mapping.second << std::endl;
  }
#endif
  for (const auto &inputNode : inputData.nodes) {
    if (inputNode.type == "t") {
      int newNodeId = originalToNewId[inputNode.id];
      VG::Params params;
      params.C = inputNode.capacitance;
      params.RAT = inputNode.rat;
      nodes.emplace_back(newNodeId, params);
#ifdef DEBUG
      std::cout << "Node: " << std::endl;
      std::cout << newNodeId << " | " << inputNode.type << " | ";
      for (const auto &parm : nodes.back().CapsRATs) {
        std::cout << parm.C << " | " << parm.RAT << std::endl;
      }
#endif
    }
  }

  for (const auto &inputEdge : inputData.edges) {
    if (inputEdge.vertices.size() < 2) {
#ifdef DEBUG
      std::cerr << "Warning: Edge " << inputEdge.id
                << " has fewer than 2 vertices, skipping." << std::endl;
#endif
      continue;
    }

    VG::Edge edge;

    int originalStart = inputEdge.vertices[0];
    int originalEnd = inputEdge.vertices[1];

    if (originalToNewId.find(originalStart) == originalToNewId.end() ||
        originalToNewId.find(originalEnd) == originalToNewId.end()) {
#ifdef DEBUG
      std::cerr << "Warning: Edge " << inputEdge.id
                << " references unknown node IDs, skipping." << std::endl;
#endif
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

// Helper function to find point coordinates at a specific distance from the
// child
std::vector<int>
findPointAtDistance(const std::vector<std::vector<int>> &segments,
                    int distanceFromChild) {
  int remainingDistance = distanceFromChild;

  for (int i = segments.size() - 2; i >= 0; --i) {
    const auto &start = segments[i];
    const auto &end = segments[i + 1];

    int segmentLength = calculateManhattanDistance(start, end);

    if (remainingDistance <= segmentLength) {
      double ratio = static_cast<double>(remainingDistance) / segmentLength;
      if (start[0] == end[0]) {
        int y = end[1] + (start[1] - end[1]) * ratio;
        return {start[0], y};
      } else {
        int x = end[0] + (start[0] - end[0]) * ratio;
        return {x, end[1]};
      }
    }

    remainingDistance -= segmentLength;
  }

  return segments[0];
}

void writeOutputFile(const std::string &originalFilename,
                     const InputData &originalData,
                     const std::vector<VG::BufPlace> &bufferLocations,
                     const std::map<int, int> &newToOriginalId) {
  std::filesystem::path inputPath(originalFilename);
  std::string outputFilename = inputPath.stem().string() + "_out.json";

#ifdef DEBUG
  for (const auto &buf : bufferLocations) {
    std::cout << "ParentID: " << buf.ParentID << ", ChildID: " << buf.ChildID
              << ", Len (from Child): " << buf.Len << "\n";
  }
#endif

  int maxNodeId = 0;
  int maxEdgeId = 0;
  for (const auto &node : originalData.nodes) {
    maxNodeId = std::max(maxNodeId, node.id);
  }
  for (const auto &edge : originalData.edges) {
    maxEdgeId = std::max(maxEdgeId, edge.id);
  }

  std::vector<InputNode> newNodes = originalData.nodes;

  InputNode bufferTemplate;
  bool foundTemplate = false;
  for (const auto &node : originalData.nodes) {
    if (node.type == "b") {
      bufferTemplate = node;
      foundTemplate = true;
      break;
    }
  }
  // Add more checks
  if (!foundTemplate) {
    throw std::runtime_error(
        "Could not find buffer template in original nodes!");
  }

  std::map<std::pair<int, int>, std::vector<VG::BufPlace>> bufferGroups;
  for (const auto &bufLoc : bufferLocations) {
    int originalParentId = newToOriginalId.at(bufLoc.ParentID);
    int originalChildId = newToOriginalId.at(bufLoc.ChildID);
    bufferGroups[{originalParentId, originalChildId}].push_back(bufLoc);
  }

  std::vector<InputEdge> newEdges;
  std::set<int> edgesToKeep;

  for (const auto &edge : originalData.edges) {
    edgesToKeep.insert(edge.id);
  }

  for (auto &[edgePair, buffers] : bufferGroups) {
    auto [originalParentId, originalChildId] = edgePair;

    // Corner case: buffer on node (self-loop)
    if (originalParentId == originalChildId) {
      for (const auto &bufLoc : buffers) {
        if (bufLoc.Len != 0) {
          std::cerr << "Warning: Non-zero length buffer on self-loop, skipping."
                    << std::endl;
          continue;
        }

        std::vector<int> nodePosition;
        for (const auto &node : originalData.nodes) {
          if (node.id == originalParentId) {
            nodePosition = {node.x, node.y};
            break;
          }
        }

        int newBufferId = ++maxNodeId;
        InputNode newBuffer = bufferTemplate;
        newBuffer.id = newBufferId;
        newBuffer.x = nodePosition[0];
        newBuffer.y = nodePosition[1];
        newNodes.push_back(newBuffer);

        InputEdge loopEdge;
        loopEdge.id = ++maxEdgeId;
        loopEdge.vertices = {newBufferId, newBufferId};
        loopEdge.segments = {nodePosition, nodePosition};
        newEdges.push_back(loopEdge);
      }
      continue;
    }

    InputEdge *targetEdge = nullptr;
    for (auto &edge : originalData.edges) {
      if ((edge.vertices[0] == originalParentId &&
           edge.vertices[1] == originalChildId) ||
          (edge.vertices[0] == originalChildId &&
           edge.vertices[1] == originalParentId)) {
        targetEdge = const_cast<InputEdge *>(&edge);
        edgesToKeep.erase(edge.id);
        break;
      }
    }

    if (!targetEdge) {
      std::cerr << "Warning: Could not find edge between nodes "
                << originalParentId << " and " << originalChildId << std::endl;
      continue;
    }

    std::vector<std::vector<int>> segments = targetEdge->segments;
    if (targetEdge->vertices[0] == originalChildId &&
        targetEdge->vertices[1] == originalParentId) {
      std::reverse(segments.begin(), segments.end());
    }

    int totalLength = calculateSegmentLength(segments);

    std::sort(buffers.begin(), buffers.end(),
              [](const VG::BufPlace &a, const VG::BufPlace &b) {
                return a.Len < b.Len;
              });

    std::vector<int> childPosition;
    for (const auto &node : originalData.nodes) {
      if (node.id == originalChildId) {
        childPosition = {node.x, node.y};
        break;
      }
    }

    struct BufferInfo {
      int id;
      std::vector<int> position;
      int distanceFromChild;
    };

    std::vector<BufferInfo> bufferInfos;

    for (const auto &bufLoc : buffers) {
      BufferInfo info;
      info.id = ++maxNodeId;
      info.distanceFromChild = bufLoc.Len;

      if (bufLoc.Len == 0) {
        info.position = childPosition;
      } else {
        info.position = findPointAtDistance(segments, bufLoc.Len);
      }

      InputNode newBuffer = bufferTemplate;
      newBuffer.id = info.id;
      newBuffer.x = info.position[0];
      newBuffer.y = info.position[1];
      newNodes.push_back(newBuffer);

      bufferInfos.push_back(info);
    }

    if (bufferInfos.empty()) {
      continue;
    }

    BufferInfo parentInfo;
    parentInfo.id = originalParentId;
    parentInfo.position = segments.front();
    parentInfo.distanceFromChild = totalLength;

    BufferInfo childInfo;
    childInfo.id = originalChildId;
    childInfo.position = segments.back();
    childInfo.distanceFromChild = 0;

    std::vector<BufferInfo> allPoints;
    allPoints.push_back(parentInfo);
    allPoints.insert(allPoints.end(), bufferInfos.begin(), bufferInfos.end());
    allPoints.push_back(childInfo);

    std::sort(allPoints.begin(), allPoints.end(),
              [](const BufferInfo &a, const BufferInfo &b) {
                return a.distanceFromChild > b.distanceFromChild;
              });

    for (size_t i = 0; i < allPoints.size() - 1; ++i) {
      const auto &startInfo = allPoints[i];
      const auto &endInfo = allPoints[i + 1];

      InputEdge newEdge;
      newEdge.id = ++maxEdgeId;
      newEdge.vertices = {startInfo.id, endInfo.id};

      if (startInfo.distanceFromChild == endInfo.distanceFromChild) {
        newEdge.segments = {startInfo.position, endInfo.position};
      } else {
        newEdge.segments = extractSegmentsBetween(
            segments, startInfo.distanceFromChild, endInfo.distanceFromChild);

        if (!newEdge.segments.empty()) {
          newEdge.segments.front() = startInfo.position;
          newEdge.segments.back() = endInfo.position;
        } else {
          newEdge.segments = {startInfo.position, endInfo.position};
        }
      }

      newEdges.push_back(newEdge);
    }
  }

  for (const auto &edge : originalData.edges) {
    if (edgesToKeep.find(edge.id) != edgesToKeep.end()) {
      newEdges.push_back(edge);
    }
  }

  json outputJson;

  json nodeArray = json::array();
  for (const auto &node : newNodes) {
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
  for (const auto &edge : newEdges) {
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

std::vector<std::vector<int>>
extractSegmentsBetween(const std::vector<std::vector<int>> &segments,
                       int startDistanceFromChild, int endDistanceFromChild) {

  if (segments.size() < 2) {
    return {};
  }

  std::vector<std::vector<int>> result;

  std::vector<int> segmentLengths;
  for (size_t i = 0; i < segments.size() - 1; ++i) {
    int length = calculateManhattanDistance(segments[i + 1], segments[i]);
    segmentLengths.push_back(length);
  }

  std::vector<int> distancesFromChild;
  int cumulativeDistance = 0;
  distancesFromChild.push_back(cumulativeDistance);
  for (int i = segmentLengths.size() - 1; i >= 0; --i) {
    cumulativeDistance += segmentLengths[i];
    distancesFromChild.insert(distancesFromChild.begin(), cumulativeDistance);
  }

  int startSegmentIdx = -1;
  int endSegmentIdx = -1;
  std::vector<int> startPoint, endPoint;

  for (size_t i = 0; i < distancesFromChild.size() - 1; ++i) {
    if (startSegmentIdx == -1 &&
        distancesFromChild[i] >= startDistanceFromChild &&
        distancesFromChild[i + 1] <= startDistanceFromChild) {
      startSegmentIdx = i;

      double ratio =
          (double)(startDistanceFromChild - distancesFromChild[i + 1]) /
          (distancesFromChild[i] - distancesFromChild[i + 1]);

      if (segments[i][0] == segments[i + 1][0]) {
        int y =
            segments[i + 1][1] + ratio * (segments[i][1] - segments[i + 1][1]);
        startPoint = {segments[i][0], y};
      } else {
        int x =
            segments[i + 1][0] + ratio * (segments[i][0] - segments[i + 1][0]);
        startPoint = {x, segments[i][1]};
      }
    }

    if (endSegmentIdx == -1 && distancesFromChild[i] >= endDistanceFromChild &&
        distancesFromChild[i + 1] <= endDistanceFromChild) {
      endSegmentIdx = i;

      double ratio =
          (double)(endDistanceFromChild - distancesFromChild[i + 1]) /
          (distancesFromChild[i] - distancesFromChild[i + 1]);

      if (segments[i][0] == segments[i + 1][0]) {
        int y =
            segments[i + 1][1] + ratio * (segments[i][1] - segments[i + 1][1]);
        endPoint = {segments[i][0], y};
      } else {
        int x =
            segments[i + 1][0] + ratio * (segments[i][0] - segments[i + 1][0]);
        endPoint = {x, segments[i][1]};
      }
    }
  }

  if (startSegmentIdx != -1 && endSegmentIdx != -1) {
    result.push_back(startPoint);

    if (startSegmentIdx > endSegmentIdx) {
      for (int i = startSegmentIdx; i > endSegmentIdx; --i) {
        result.push_back(segments[i]);
      }
    } else if (endSegmentIdx > startSegmentIdx) {
      for (int i = startSegmentIdx + 1; i <= endSegmentIdx; ++i) {
        result.push_back(segments[i]);
      }
    }

    if (startPoint != endPoint) {
      result.push_back(endPoint);
    }
  }

  return result;
}

} // namespace JSONTools
