#ifndef REPEATER_INSERTION_H
#define REPEATER_INSERTION_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <vector>

namespace VG {
// Physical parameters of the elements. C-capacitance. R - resistance.
struct TechParams {
  float C;
  float R;
  float IntrinsicDel;
};

// For convenience in presenting the solution
struct BufPlace {
  int ParentID;
  int ChildID;
  int Len;
};

struct Params {
  float C;
  float RAT;
  std::vector<BufPlace> Buffers;

  bool operator<(const Params &Rhs) const {
    if (C < Rhs.C)
      return true;
    return RAT < Rhs.RAT;
  }

  bool operator==(const Params &Rhs) const {
    if (std::fabs(C - Rhs.C) < std::numeric_limits<float>::epsilon())
      return true;
    return std::fabs(RAT - Rhs.RAT) < std::numeric_limits<float>::epsilon();
  }
};

// Representing a net routing tree as edges connecting syns, steiner points and buffer.
struct Edge {
  int Start;
  int End;
  int Len;
  bool IsVisited = false;
};

// Sink, steiner point or buffer
struct Node {
  int ID;
  std::vector<Node *> Children;
  std::vector<int> Lens;
  std::list<Params> CapsRATs;

  Node() = default;
  Node(int ID, const Params &CRAT) : ID(ID) { CapsRATs = {CRAT}; }
};

class BufferInsertVG {
  Node *Root;
  int CountSinks;
  TechParams UnitWire;
  TechParams Buffer;

  void buildRecursive(Node *node, std::vector<Edge> &Edges,
                      std::vector<Node> &Sinks) const;
  std::list<Params> recursiveVanGin(Node *node);
  void addWire(std::list<Params> &List, Node *Parent, Node *Child, int Len);
  void insertBuffer(std::list<Params> &List, Node *Parent, Node *Child,
                    int Len);
  std::list<Params> mergeBranch(std::list<Params> &First,
                                std::list<Params> &Second, Node *Parent);
  std::list<Params> mergeBranches(std::vector<std::list<Params>> &CldParams,
                                  Node *Parent);
  void pruneSolutions(std::list<Params> &Solutions);

public:
  BufferInsertVG(const TechParams &UnitWire, const TechParams &Buffer) : UnitWire(UnitWire), Buffer(Buffer) {
    Root = new Node;
    Root->ID = 0;
  };

  void buildRoutingTree(std::vector<Edge> &Edges, std::vector<Node> &Sinks);
  Params getOptimParams();
};

} //namespace VG

#endif // REPEATER_INSERTION_H
