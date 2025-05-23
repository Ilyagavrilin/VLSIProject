#include "BufferInsertVG.h"
#include "JSONTools.h"
#include <chrono>
#include <iostream>
#include <map>
#include <string>

// #define DEBUG

int main(int argc, char* argv[]) {
  using namespace std::chrono;
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " <technology_file>.json <test_file>.json" << std::endl;
    return 1;
  }

    std::string techFilename = argv[1];
    std::string testFilename = argv[2];
    
    try {
        VG::TechParams wireParams = JSONTools::parseTechFile(techFilename);
        VG::TechParams bufferParams = JSONTools::parseBufferParams(techFilename);
        JSONTools::InputData inputData = JSONTools::parseTestFile(testFilename);
        
        std::vector<VG::Edge> edges;
        std::vector<VG::Node> nodes;
        std::map<int, int> originalToNewId, newToOriginalId;
        JSONTools::convertToVGStructures(inputData, edges, nodes, originalToNewId, newToOriginalId);
#ifdef DEBUG
        std::cout << "Edges\n";
        for (const auto& elem: edges) 
            std::cout << elem.Start << " | " << elem.End << " | " << elem.Len << std::endl;
        std::cout << std::endl << "Sinks:\n";
        for (const auto& elem: nodes)
          std::cout << elem.ParentID << " | " << elem.CapsRATs.begin()->C
                    << " | " << elem.CapsRATs.begin()->RAT << std::endl;
#endif
        VG::BufferInsertVG bufferInserter(wireParams, bufferParams);
        bufferInserter.buildRoutingTree(edges, nodes);

        auto Start = high_resolution_clock::now();
        const auto &optimalParams = bufferInserter.getOptimParams();
        auto End = high_resolution_clock::now();

        const auto &bufferLocations = optimalParams.Buffers;

        JSONTools::writeOutputFile(testFilename, inputData, bufferLocations, newToOriginalId);

        std::cout << "Optimization complete. Optimal RAT: "
                  << std::round(optimalParams.RAT * 100) / 100 << std::endl;
#ifdef DEBUG
        std::cout << "Time: "
                  << duration_cast<milliseconds>(End - Start).count()
                  << std::endl;
#endif

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
