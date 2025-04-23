#include <iostream>
#include <string>
#include "JSONTools.h"
#include "BufferInsertVG.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <technology_file>.json <test_file>.json" << std::endl;
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
        JSONTools::convertToVGStructures(inputData, edges, nodes);
#ifdef DEBUG
        for (const auto& elem: edges) 
            std::cout << elem.Start << " | " << elem.End << " | " << elem.Len << std::endl;
        std::cout << std::endl;
        for (const auto& elem: nodes) 
            std::cout << elem.ID << " | " << elem.CapsRATs.begin()->C << " | " << elem.CapsRATs.begin()->RAT << std::endl;
#endif
        VG::BufferInsertVG bufferInserter(wireParams, bufferParams);
        bufferInserter.buildRoutingTree(edges, nodes);
        
        VG::Params optimalParams = bufferInserter.getOptimParams();
        VG::Trace optimalTrace = bufferInserter.getTrace(optimalParams);
        
        JSONTools::writeOutputFile(testFilename, inputData, optimalTrace);
        
        std::cout << "Optimization complete. Optimal RAT: " << optimalParams.RAT << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
