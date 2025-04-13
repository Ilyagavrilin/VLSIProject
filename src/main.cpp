#include "BufferInsertVG.h"
#include "JSONTools.h"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0]
              << " <technology_file_name>.json <test_name>.json" << std::endl;
    return 1;
  }

  float BufC, BufR, BufDel;
  float UnitR, UnitC;

  if (!JSONHelper::parseTechFile(argv[1], BufC, BufR, BufDel, UnitR, UnitC)) {
    std::cerr << "Failed to parse technology file: " << argv[1] << std::endl;
    return 1;
  }

  std::vector<VG::Edge> Edges;
  std::vector<VG::Params> CapsRATs;
  std::map<int, std::pair<int, int>>
      NodeCoords; // TODO(Ilyagavrilin): wrap coords into structure

  if (!JSONHelper::parseTestFile(argv[2], Edges, CapsRATs, NodeCoords)) {
    std::cerr << "Failed to parse test file: " << argv[2] << std::endl;
    return 1;
  }

  std::vector<VG::Node> Sinks;
  auto SinkID = 1;
  for (const auto &Params : CapsRATs)
    Sinks.emplace_back(SinkID++, Params);

  VG::BufferInsertVG Net({UnitC, UnitR, 0}, {BufC, BufR, BufDel});
  Net.buildRoutingTree(Edges, Sinks);
  const auto &OptimParams = Net.getOptimParams();
  const auto &ResultTrace = Net.getTrace(OptimParams);

  VG::Trace BufPlaces;
  std::copy_if(ResultTrace.begin(), ResultTrace.end(),
               std::back_inserter(BufPlaces),
               [](const auto &TrPoint) { return TrPoint.IsBuffer; });

  // TODO(Ilyagavrilin): remove after testing
  std::cout << "\nCount buffers: " << BufPlaces.size() << std::endl;
  std::cout << "Buffer places: ";
  for (const auto &Place : BufPlaces)
    std::cout << Place.ID << " ";
  std::cout << "\nRAT: " << OptimParams.RAT << ", C: " << OptimParams.C
            << std::endl;

  if (!JSONHelper::generateOutputFile(argv[2], NodeCoords, BufPlaces)) {
    std::cerr << "Failed to generate output file" << std::endl;
    return 1;
  }

  return 0;
}