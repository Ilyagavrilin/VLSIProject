#include "BufferInsertVG.h"

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
    printNTree(*i, flag, depth + 1, it == (x->Children.size()) - 1);
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
  pruneSolutions(Root->CapsRATs);
#ifdef DEBUG
  std::cout << "ALL PARAMS:\n";
  for (auto Node : Root->CapsRATs)
    std::cout << "    C: " << Node.C << ", RAT: " << Node.RAT << "\n";

  std::cout << "Optim C=" << Root->CapsRATs.back().C
            << ", RAT: " << Root->CapsRATs.back().RAT << "\n";

  const auto &Tr = Root->SolutionsTrace[Root->CapsRATs.back()];
  std::cout << "Trace size: " << Tr.size() << "\n";
  for (auto TrEl : Tr)
    std::cout << "    Tr.ID= " << TrEl.ID << ", Tr.isbuf=" << TrEl.IsBuffer
              << "\n";
#endif
  return Root->CapsRATs.back();
}

Trace BufferInsertVG::getTrace(const Params &P) const { return Root->SolutionsTrace[P]; }

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

std::list<Params> BufferInsertVG::mergeBranch(const std::list<Params> &First,
                                              const std::list<Params> &Second,
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

std::list<Params>
BufferInsertVG::mergeBranches(const std::vector<std::list<Params>> &CldParams,
                              Node *Parent) {
  for ([[maybe_unused]] auto &One : CldParams)
    assert(!One.empty());

  switch (CldParams.size()) {
  case 1:
    return CldParams.front();
  case 2:
    return mergeBranch(CldParams.front(), CldParams.back(), Parent);
  }
  std::list<Params> Result;
  for (auto Cld = std::prev(CldParams.end()); Cld != CldParams.begin(); --Cld) {
    std::list<Params> FirstResult = *Cld;

    std::list<Params> SecondResult;
    std::for_each(CldParams.begin(), Cld, [&SecondResult](const auto &Result) {
      std::copy(Result.begin(), Result.end(), std::back_inserter(SecondResult));
    });

    auto Solution = mergeBranch(FirstResult, SecondResult, Parent);
    std::move(Solution.begin(), Solution.end(), std::back_inserter(Result));
  }
  return Result;
}

// Recursive adding wires, buffers and prunning inferior solutions
std::list<Params> BufferInsertVG::recursiveVanGin(Node *N) {
  std::list<Params> Result;
  if ((N->ID > 0) && (N->ID < CountSinks + 1)) {
    // sink - tree leaf
    Result = N->CapsRATs;
    assert(Result.size() == 1);
    assert((N->SolutionsTrace).empty());
    N->SolutionsTrace[Result.front()] = {{N->ID, /* IsBuffer */ false}};
    return Result;
  }
  assert(N->SolutionsTrace.size() == 0);

  std::vector<std::list<Params>> ChildParams;
  for (auto i = 0; i < int(N->Children.size()); ++i) {
    Node *Cld = N->Children[i];
    auto LenCld = N->Lens[i];
    auto CldParams = recursiveVanGin(Cld);
    addWire(CldParams, N, Cld, LenCld);
    insertBuffer(CldParams, N);
    pruneSolutions(CldParams);
    ChildParams.push_back(CldParams);
  }

  auto Middle = mergeBranches(ChildParams, N);
  pruneSolutions(Middle);
  return Middle;
}

} // namespace VG
