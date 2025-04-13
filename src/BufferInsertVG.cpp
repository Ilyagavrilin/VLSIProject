#include "BufferInsertVG.h"

namespace VG {

void BufferInsertVG::buildRoutingTree(std::vector<Edge> &Edges,
                                   std::vector<Node> &Sinks) {
  CountSinks = Sinks.size();
  buildRecursive(Root, Edges, Sinks);
}

Params BufferInsertVG::getOptimParams() {
  Root->CapsRATs = recursiveVanGin(Root);
  pruneSolutions(Root->CapsRATs);

  return Root->CapsRATs.back();
}

Trace BufferInsertVG::getTrace(const Params &P) const { return Root->SolutionsTrace[P]; }

static bool isAllVisited(std::vector<Edge> &Edges) {
  return std::all_of(Edges.begin(), Edges.end(),
                     [](const auto Eg) { return Eg.IsVisited; });
}

void BufferInsertVG::buildRecursive(Node *node, std::vector<Edge> &Edges,
                                 std::vector<Node> &Sinks) const {
  if (node->ID > 0 && node->ID < CountSinks + 1) {
    node->CapsRATs = Sinks[node->ID - 1].CapsRATs;
  }
  if (!isAllVisited(Edges)) {
    for (auto &Eg : Edges) {
      if (node->ID == Eg.Start) {
        Eg.IsVisited = true;
        if (node->Left && !node->Right) {
          node->Right = new Node;
          node->LenRight = Eg.Len;
          node->Right->ID = Eg.End;
          buildRecursive(node->Right, Edges, Sinks);
        } else if (!node->Left) {
          node->Left = new Node;
          node->LenLeft = Eg.Len;
          node->Left->ID = Eg.End;
          buildRecursive(node->Left, Edges, Sinks);
        }
      }
    }
  }
  return;
}

void BufferInsertVG::addWire(std::list<Params> &List, Node *Parent, Node *Child,
                          int Len) {
  assert(!List.empty());
  for (auto &Point : List) {
    auto &TraceBefore = Child->SolutionsTrace[Point];
    TraceBefore.push_back({Parent->ID, /* IsBuffer */ false});
    Point.RAT =
        Point.RAT - Len * Len * UnitWire.R * UnitWire.C / 2 - Len * UnitWire.R * Point.C;
    Point.C = Point.C + Len * UnitWire.C;
    Parent->SolutionsTrace[Point] = TraceBefore;
  }
  return;
}

void BufferInsertVG::insertBuffer(std::list<Params> &List, Node *Parent) {
  if (!List.empty()) {
    std::list<Params> tmpList = List;
    for (auto &Point : tmpList) {
      Trace addtmp = Parent->SolutionsTrace[Point];
      addtmp.push_back({Parent->ID, /* IsBuffer */ true});
      Point.RAT = Point.RAT - Buffer.R * Point.C - Buffer.IntrinsicDel;
      Point.C = Buffer.C;
      List.push_back(Point);
      Parent->SolutionsTrace[Point] = addtmp;
    }
  }
  return;
}

void BufferInsertVG::pruneSolutions(std::list<Params> &Solutions) {
  // Sort according to cap values
  Solutions.sort([](const auto &A, const auto &B) { return A.C < B.C; });

  auto FirstBr = Solutions.begin();
  auto SecondBr = Solutions.begin();
  SecondBr++;
  // We don't need to check for c1 > c2 case as we already sorted the Solutions
  while (SecondBr != Solutions.end()) {
    if (FirstBr->C < SecondBr->C) {
      // Delete element with smaller q
      if ((FirstBr->RAT) >= (SecondBr->RAT))
        SecondBr = Solutions.erase(SecondBr);
      else
        ++FirstBr, ++SecondBr;
    } else if (FirstBr->C == SecondBr->C) {
      // Check for q if c is equal
      // Delete element with smaller q
      if (FirstBr->RAT >= SecondBr->RAT)
        SecondBr = Solutions.erase(SecondBr);
      else {
        // Delete one of two equal elements
        FirstBr = Solutions.erase(FirstBr);
        ++SecondBr;
      }
    }
  }
}

std::list<Params> BufferInsertVG::mergeBranch(std::list<Params> &First,
                                           std::list<Params> &Second,
                                           Node *Parent) {
  std::list<Params> Result;
  auto FirstBr = First.begin();
  auto SecondBr = Second.begin();
  while (FirstBr != First.end() && SecondBr != Second.end()) {
    auto FirstTr = Parent->SolutionsTrace[*FirstBr];
    auto SecondTr = Parent->SolutionsTrace[*SecondBr];

    for (const auto Point : SecondTr)
      FirstTr.push_back(Point);
    auto C = FirstBr->C + SecondBr->C;
    // It is works because both branches already sorted by caps
    auto MinRAT = std::min(FirstBr->RAT, SecondBr->RAT);
    Result.push_back({C, MinRAT});
    Parent->SolutionsTrace[{C, MinRAT}] = FirstTr;
    if (FirstBr->RAT == MinRAT)
      ++FirstBr;
    else
      ++SecondBr;
  }
  return Result;
}

// Recursive adding wires, buffers and prunning inferior solutions
std::list<Params> BufferInsertVG::recursiveVanGin(Node *node) {
  std::list<Params> Left, Right, Middle, Result;
  if ((node->ID > 0) && (node->ID < CountSinks + 1)) {
    Result = node->CapsRATs;
    assert(Result.size() == 1);
    assert((node->SolutionsTrace).empty());
    node->SolutionsTrace[Result.front()] = {{node->ID, /* IsBuffer */ false}};
    return Result;
  }
  assert(node->SolutionsTrace.size() == 0);

  if (node->Left) {
    Left = recursiveVanGin(node->Left);
    addWire(Left, node, node->Left, node->LenLeft);
    insertBuffer(Left, node);
    pruneSolutions(Left);
  }
  if (node->Right) {
    Right = recursiveVanGin(node->Right);
    addWire(Right, node, node->Right, node->LenRight);
    insertBuffer(Right, node);
    pruneSolutions(Right);
  }
  if (node->Left && node->Right) {
    Middle = mergeBranch(Left, Right, node);
    pruneSolutions(Middle);
    return Middle;
  }

  if (node->Left && !node->Right)
    return Left;

  assert(0 && "All nodes have't Children or have Left Child");
  return Left;
}

} // namespace VG
