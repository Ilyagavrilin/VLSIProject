#include "VanGinneken.h"

void VanGinneken::buildRoutingTree(std::vector<Edge> &Edges,
                                   std::vector<Node> &Sinks) {
  CountSinks = Sinks.size();
  buildRecursive(root, Edges, Sinks);
}

Params VanGinneken::getOptimParams() {
  root->cap_n_RAT = recursiveVanGin(root);
  pruneSolutions(root->cap_n_RAT);

  return root->cap_n_RAT.back();
}

Trace VanGinneken::getTrace(const Params &P) const { return root->mapList[P]; }

static bool isAllVisited(std::vector<Edge> &Edges) {
  return std::all_of(Edges.begin(), Edges.end(),
                     [](const auto Eg) { return Eg.IsVisited; });
}

void VanGinneken::buildRecursive(Node *node, std::vector<Edge> &Edges,
                                 std::vector<Node> &Sinks) const {
  if (node->num > 0 && node->num < CountSinks + 1) {
    node->left = nullptr;
    node->right = nullptr;
    node->cap_n_RAT = Sinks[node->num - 1].cap_n_RAT;
  }
  if (!isAllVisited(Edges)) {
    for (auto &Eg : Edges) {
      if (node->num == Eg.Start) {
        Eg.IsVisited = true;
        if (node->left && !node->right) {
          node->right = new Node;
          node->right->left = nullptr;
          node->right->right = nullptr;
          node->Len_right = Eg.Len;
          node->right->num = Eg.End;
          buildRecursive(node->right, Edges, Sinks);
        } else if (!node->left) {
          node->left = new Node;
          node->left->right = nullptr;
          node->left->left = nullptr;
          node->Len_left = Eg.Len;
          node->left->num = Eg.End;
          buildRecursive(node->left, Edges, Sinks);
        }
      }
    }
  }
  return;
}

void VanGinneken::addWire(std::list<Params> &List, Node *Parent, Node *Child,
                          int Len) {
  assert(!List.empty());
  for (auto &Point : List) {
    auto &TraceBefore = Child->mapList[Point];
    TraceBefore.push_back({Parent->num, /* IsBuffer */ false});
    Point.RAT =
        Point.RAT - Len * Len * unitR * unitC / 2 - Len * unitR * Point.C;
    Point.C = Point.C + Len * unitC;
    Parent->mapList[Point] = TraceBefore;
  }
  return;
}

void VanGinneken::insertBuffer(std::list<Params> &List, Node *Parent) {
  if (!List.empty()) {
    std::list<Params> tmpList = List;
    for (auto &Point : tmpList) {
      Trace addtmp = Parent->mapList[Point];
      addtmp.push_back({Parent->num, /* IsBuffer */ true});
      Point.RAT = Point.RAT - BufR * Point.C - BufDel;
      Point.C = BufC;
      List.push_back(Point);
      Parent->mapList[Point] = addtmp;
    }
  }
  return;
}

void VanGinneken::pruneSolutions(std::list<Params> &Solutions) {
  // Sort according to cap values
  Solutions.sort([](const auto &A, const auto &B) { return A.C < B.C; });

  auto FirstBr = Solutions.begin();
  auto SecondBr = Solutions.begin();
  SecondBr++;
  // we don't need to check for c1 > c2 case as we already sorted the Solutions
  while (SecondBr != Solutions.end()) {
    if (FirstBr->C < SecondBr->C) {
      // delete element with smaller q
      if ((FirstBr->RAT) >= (SecondBr->RAT))
        SecondBr = Solutions.erase(SecondBr);
      else
        ++FirstBr, ++SecondBr;
    } else if (FirstBr->C == SecondBr->C) {
      // Check for q if c is equal
      // delete element with smaller q
      if (FirstBr->RAT >= SecondBr->RAT)
        SecondBr = Solutions.erase(SecondBr);
      else {
        // delete one of two equal elements
        FirstBr = Solutions.erase(FirstBr);
        ++SecondBr;
      }
    }
  }
}

std::list<Params> VanGinneken::mergeBranch(std::list<Params> &First,
                                           std::list<Params> &Second,
                                           Node *Parent) {
  std::list<Params> Result;
  auto FirstBr = First.begin();
  auto SecondBr = Second.begin();
  while (FirstBr != First.end() && SecondBr != Second.end()) {
    auto FirstTr = Parent->mapList[*FirstBr];
    auto SecondTr = Parent->mapList[*SecondBr];

    for (const auto Point : SecondTr)
      FirstTr.push_back(Point);
    auto C = FirstBr->C + SecondBr->C;
    // It is works because both branches already sorted by caps
    auto MinRAT = std::min(FirstBr->RAT, SecondBr->RAT);
    Result.push_back({C, MinRAT}); // pick min
    Parent->mapList[{C, MinRAT}] = FirstTr;
    if (FirstBr->RAT == MinRAT)
      ++FirstBr;
    else
      ++SecondBr;
  }
  return Result;
}

// Recursive adding wires, buffers and prunning inferior solutions
std::list<Params> VanGinneken::recursiveVanGin(Node *node) {
  std::list<Params> Left, Right, Middle, Result;
  if ((node->num > 0) && (node->num < CountSinks + 1)) {
    Result = node->cap_n_RAT;
    assert(Result.size() == 1);
    assert((node->mapList).empty());
    node->mapList[Result.front()] = {{node->num, /* IsBuffer */ false}};
    return Result;
  }
  assert(node->mapList.size() == 0);

  if (node->left) {
    Left = recursiveVanGin(node->left);
    addWire(Left, node, node->left, node->Len_left);
    insertBuffer(Left, node);
    pruneSolutions(Left);
  }
  if (node->right) {
    Right = recursiveVanGin(node->right);
    addWire(Right, node, node->right, node->Len_right);
    insertBuffer(Right, node);
    pruneSolutions(Right);
  }
  if (node->left && node->right) {
    Middle = mergeBranch(Left, Right, node);
    pruneSolutions(Middle);
    return Middle;
  }

  if (node->left && !node->right)
    return Left;
  assert(0 && "All nodes have't Children or have left Child");
}

//  VanGinneken VG;
//  VG.buildRoutingTree(edgeList, sinkList);
//  const auto &OptimParams = VG.getOptimParams();
//  const auto &ResultTrace = VG.getTrace(OptimParams);
//
//  Trace BufPlaces;
//  std::copy_if(ResultTrace.begin(), ResultTrace.end(),
//               std::back_inserter(BufPlaces),
//               [](const auto &TrPoint) { return TrPoint.IsBuffer; });
//
//  std::cout << "\nCount buffers: " << BufPlaces.size() << std::endl;
//  std::cout << "Buffer places: ";
//  for (const auto &Place : BufPlaces)
//    std::cout << Place.Num << " ";
//  std::cout << "\nRAT: " << OptimParams.RAT
//            << ", C: " << OptimParams.C << std::endl;
