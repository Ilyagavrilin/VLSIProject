#include "BufferInsertVG.h"

// #define DEBUG

namespace VG {

#ifdef DEBUG
// Function to print the
// N-ary tree graphically
static void printNTree(Node *x, std::vector<bool> flag, int depth = 0,
                       bool isLast = false) {
  if (x == NULL)
    return;
  for (int i = 1; i < depth; ++i) {
    if (flag[i] == true) {
      std::cout << "| " << " " << " " << " ";
    } else {
      std::cout << " " << " " << " " << " ";
    }
  }
  if (depth == 0)
    std::cout << x->ID << '\n';
  else if (isLast) {
    std::cout << "+--- " << x->ID << '\n';
    flag[depth] = false;
  } else
    std::cout << "+--- " << x->ID << '\n';
  int it = 0;
  for (auto i = x->Children.begin(); i != x->Children.end(); ++i, ++it)
    printNTree(*i, flag, depth + 1, it == int(x->Children.size()) - 1);
  flag[depth] = true;
}

static void printNTree(Node *N, int nv) {
  std::vector<bool> flag(nv, true);
  printNTree(N, flag);
}
#endif

void BufferInsertVG::buildRoutingTree(std::vector<Edge> &Edges,
                                   std::vector<Node> &Sinks) {
  CountSinks = Sinks.size();
  buildRecursive(Root, Edges, Sinks);

#ifdef DEBUG
  std::cout << "Result tree:\n";
  int MaxNode = 0;
  for (auto E : Edges) {
    auto Max = std::max(E.Start, E.End);
    MaxNode = std::max(MaxNode, Max);
  }
  printNTree(Root, MaxNode);
#endif
}

Params BufferInsertVG::getOptimParams() {
  Root->CapsRATs = recursiveVanGin(Root);
  insertBuffer(Root->CapsRATs, Root, Root, 0);
  auto NoContainMainBuf = [](const auto &CR) {
    if (CR.Buffers.empty())
      return true;
    return CR.Buffers.back() != BufPlace{0, 0, 0};
  };
  Root->CapsRATs.remove_if(NoContainMainBuf);
  pruneSolutions(Root->CapsRATs);
#ifdef DEBUG

  std::cout << "Optim RAT: " << Root->CapsRATs.back().RAT
            << ", Count buffers: " << Root->CapsRATs.back().Buffers.size()
            << "\n";
  for (auto B : Root->CapsRATs.back().Buffers)
    std::cout << "    BUFFER Parent: " << B.ParentID
              << ", Child : " << B.ChildID << ", Len: " << B.Len << "\n";

#endif
  assert(Root->CapsRATs.size() == 1);
  return Root->CapsRATs.back();
}

static bool isAllVisited(std::vector<Edge> &Edges) {
  return std::all_of(Edges.begin(), Edges.end(),
                     [](const auto Eg) { return Eg.IsVisited; });
}

void BufferInsertVG::buildRecursive(Node *N, std::vector<Edge> &Edges,
                                    std::vector<Node> &Sinks) const {
  if (N->ID > 0 && N->ID < CountSinks + 1) {
    // sink
    N->CapsRATs = Sinks[N->ID - 1].CapsRATs;
  }
  if (!isAllVisited(Edges)) {
    for (auto &Eg : Edges) {
      if (N->ID == Eg.Start) {
        Eg.IsVisited = true;
        Node *New = new Node;
        New->ID = Eg.End;
        N->Children.push_back(New);
        N->Lens.push_back(Eg.Len);
        buildRecursive(N->Children.back(), Edges, Sinks);
      }
    }
  }
  return;
}

void BufferInsertVG::addWire(std::list<Params> &List, Node *Parent, Node *Child,
                          int Len) {
  assert(!List.empty());
  constexpr auto UnitLen = 1;
  std::list<Params> tmpList = List;
  for (auto &Point : tmpList) {
    List.remove(Point);
    Point.RAT = Point.RAT - UnitLen * UnitLen * UnitWire.R * UnitWire.C / 2 -
                UnitLen * UnitWire.R * Point.C;
    Point.C = Point.C + UnitLen * UnitWire.C;
    List.push_back(Point);
  }
  return;
}

void BufferInsertVG::insertBuffer(std::list<Params> &List, Node *Parent,
                                  Node *Child, int Len) {
  assert(!List.empty());
  std::list<Params> tmpList = List;
  for (auto &Point : tmpList) {
    Point.RAT = Point.RAT - Buffer.R * Point.C - Buffer.IntrinsicDel;
    Point.C = Buffer.C;
    Point.Buffers.push_back({Parent->ID, Child->ID, Len});
    List.push_back(Point);
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
    auto C = FirstBr->C + SecondBr->C;
    // It is works because both branches already sorted by caps
    auto MinRAT = std::min(FirstBr->RAT, SecondBr->RAT);
    FirstBr->Buffers.insert(FirstBr->Buffers.end(), SecondBr->Buffers.begin(),
                            SecondBr->Buffers.end());
    Params New(C, MinRAT, FirstBr->Buffers);
    Result.push_back(New);

    if (FirstBr->RAT == MinRAT)
      ++FirstBr;
    else
      ++SecondBr;
  }
  return Result;
}

std::list<Params>
BufferInsertVG::mergeBranches(std::vector<std::list<Params>> &CldParams,
                              Node *Parent) {
  if (CldParams.size() == 1)
    return CldParams.back();

  std::list<Params> FirstBr = *CldParams.begin();
  for (auto SecondBr = std::next(CldParams.begin());
       SecondBr != CldParams.end(); ++SecondBr) {
    FirstBr = mergeBranch(FirstBr, *SecondBr, Parent);
    pruneSolutions(FirstBr);
  }
  return FirstBr;
}

// Recursive adding wires, buffers and prunning inferior solutions
std::list<Params> BufferInsertVG::recursiveVanGin(Node *N) {
  if ((N->ID > 0) && (N->ID < CountSinks + 1)) {
    // sink - tree leaf
    return N->CapsRATs;
  }

  std::vector<std::list<Params>> ChildParams;
  for (auto i = 0; i < int(N->Children.size()); ++i) {
    Node *Cld = N->Children[i];
    auto LenCld = N->Lens[i];
    auto CldParams = recursiveVanGin(Cld);
    if (LenCld == 0) {
      insertBuffer(CldParams, N, Cld, 0);
      pruneSolutions(CldParams);
    } else if (Cld->ID < CountSinks + 1) {
      for (auto j = 1; j <= LenCld; ++j) {
        addWire(CldParams, N, Cld, 1);
        insertBuffer(CldParams, N, Cld, j);
        pruneSolutions(CldParams);
      }
    } else {
      for (auto j = 0; j < LenCld; ++j) {
        addWire(CldParams, N, Cld, 1);
        insertBuffer(CldParams, N, Cld, j);
        pruneSolutions(CldParams);
      }
    }

    ChildParams.push_back(CldParams);
  }

  auto Middle = mergeBranches(ChildParams, N);
  pruneSolutions(Middle);
  return Middle;
}

} // namespace VG
