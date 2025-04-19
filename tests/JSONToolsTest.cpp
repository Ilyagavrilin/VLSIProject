#include "JSONTools.h"
#include "BufferInsertVG.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdio>

namespace {

class JSONToolsTest : public ::testing::Test {
protected:
  void SetUp() override {
    createTempTechFile();
    createTempTestFile();
  }

  void TearDown() override {
    if (std::filesystem::exists(tempTechFile)) {
      std::filesystem::remove(tempTechFile);
    }
    
    if (std::filesystem::exists(tempTestFile)) {
      std::filesystem::remove(tempTestFile);
    }

    // Name of ouput file builds like <inut_name>_out.json
    std::string outputFile = "test_temp_out.json";
    if (std::filesystem::exists(outputFile)) {
      std::filesystem::remove(outputFile);
    }
  }

  void createTempTechFile() {
    tempTechFile = "tech_temp.json";
    std::ofstream file(tempTechFile);
    file << R"({
      "module": [{
        "name": "buf1x",
        "output": [{ "name": "z", "inverting": "no"}],
        "input": [{ "name": "a", "C": 0.5, "R": 2.0, "intrinsic_delay": 4.0}]
      }],
      "technology": {
        "unit_wire_resistance": 0.05,
        "unit_wire_resistance_comment0": "KOhm/um",
        "unit_wire_capacitance": 0.3,
        "unit_wire_capacitance_comment0": "fF/um"
      }
    })";
    file.close();
  }

  void createTempTestFile() {
    tempTestFile = "test_temp.json";
    std::ofstream file(tempTestFile);
    file << R"({
    "node": [
        {
            "id": 0,
            "x": 0,
            "y": 0,
            "type": "b",
            "name": "buf1x"
        },
        {
            "id": 1,
            "x": 90,
            "y": 10,
            "type": "t",
            "name": "z0",
            "capacitance": 0.5,
            "rat": 200.0
        }
    ],
    "edge": [
        {
            "id": 0,
            "vertices": [ 0, 1 ],
            "segments": [
                [ 0, 0 ],
                [90, 0 ],
                [90, 10]
            ]
        }
    ]
})";
    file.close();
  }

  std::string tempTechFile;
  std::string tempTestFile;
};

// Test parsing technology file
TEST_F(JSONToolsTest, ParseTechFile) {
  float bufC, bufR, bufDel, unitR, unitC;
  
  bool result = JSONHelper::parseTechFile(tempTechFile, bufC, bufR, bufDel, unitR, unitC);
  
  EXPECT_TRUE(result);
  EXPECT_FLOAT_EQ(bufC, 0.5f);
  EXPECT_FLOAT_EQ(bufR, 2.0f);
  EXPECT_FLOAT_EQ(bufDel, 4.0f);
  EXPECT_FLOAT_EQ(unitR, 0.05f);
  EXPECT_FLOAT_EQ(unitC, 0.3f);
}

// Test parsing invalid tech file
TEST_F(JSONToolsTest, ParseInvalidTechFile) {
  float bufC, bufR, bufDel, unitR, unitC;
  
  bool result = JSONHelper::parseTechFile("no_file.json", bufC, bufR, bufDel, unitR, unitC);
  
  EXPECT_FALSE(result);
}

// Test parsing test file
TEST_F(JSONToolsTest, ParseTestFile) {
  std::vector<VG::Edge> edges;
  std::vector<VG::Params> capsRATs;
  std::map<int, std::pair<int, int>> nodeCoords;
  
  bool result = JSONHelper::parseTestFile(tempTestFile, edges, capsRATs, nodeCoords);
  
  EXPECT_TRUE(result);
  
  // Check sizes
  EXPECT_EQ(edges.size(), 1);
  EXPECT_EQ(capsRATs.size(), 1);
  EXPECT_EQ(nodeCoords.size(), 2);
  
  if (capsRATs.size() >= 1) {
    EXPECT_FLOAT_EQ(capsRATs[0].C, 0.5f);
    EXPECT_FLOAT_EQ(capsRATs[0].RAT, 200.0f);
  }
  
  // Check coords
  if (nodeCoords.size() >= 2) {
    EXPECT_EQ(nodeCoords[0].first, 0);
    EXPECT_EQ(nodeCoords[0].second, 0);
    
    EXPECT_EQ(nodeCoords[1].first, 90);
    EXPECT_EQ(nodeCoords[1].second, 10);
    
    EXPECT_EQ(nodeCoords[3].first, 0);
    EXPECT_EQ(nodeCoords[3].second, 0);
  }
}
} // namespace