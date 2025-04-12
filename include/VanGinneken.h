#ifndef REPEATER_INSERTION_H
#define REPEATER_INSERTION_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

struct Params {
  int C;
  int RAT;

  bool operator<(const Params &A) const {
    if (C < A.C)
      return true;
    return RAT < A.RAT;
  }
};

struct TraceElem {
  int Num;
  bool IsBuffer;
};

typedef std::list<TraceElem> Trace;

int BufC = 5, BufR = 1, BufDel = 10;
int unitR = 5, unitC = 10;

struct Edge {
  int Len;
  int Start;
  int End;
  bool IsVisited;
};

struct Node {
  Node *left;
  Node *right;
  int num;
  int Len_left;
  int Len_right;
  std::list<Params> cap_n_RAT;
  std::map<Params, Trace> mapList;
};

class VanGinneken {
  Node *root;
  int CountSinks;

  void buildRecursive(Node *node, std::vector<Edge> &Edges,
                      std::vector<Node> &Sinks) const;
  std::list<Params> recursiveVanGin(Node *node);
  void addWire(std::list<Params> &List, Node *Parent, Node *Child, int Len);
  void insertBuffer(std::list<Params> &List, Node *Parent);
  std::list<Params> mergeBranch(std::list<Params> &First,
                                std::list<Params> &Second, Node *Parent);
  void pruneSolutions(std::list<Params> &Solutions);

public:
  VanGinneken() {
    root = new Node;
    root->num = 0;
  };

  void buildRoutingTree(std::vector<Edge> &Edges, std::vector<Node> &Sinks);
  Params getOptimParams();
  Trace getTrace(const Params &P) const;
};

#endif // REPEATER_INSERTION_H
