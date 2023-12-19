#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"
#include <unordered_set>
#include <set>

namespace col {
struct Result {
  Result() : coloring(nullptr), spills(nullptr) {}
  Result(temp::Map *coloring, live::INodeListPtr spills)
      : coloring(coloring), spills(spills) {}
  temp::Map *coloring;
  live::INodeListPtr spills;
};

class Color {
  /* TODO: Put your lab6 code here */
public:
  live::LiveGraph livegraph;
  std::unordered_set<temp::Temp *> precolored;
  live::INodeListPtr simplify_work;
  live::INodeListPtr freeze_work;
  live::INodeListPtr spill_work;
  live::INodeListPtr spill_node;
  live::INodeListPtr select_stack;
  live::INodeListPtr coalesced_node;
  live::INodeListPtr colored_node;
  live::INodeListPtr ini;

  live::MoveList *work_moves;
  live::MoveList *active_move;
  live::MoveList *colaesce_move;
  live::MoveList *constrained_move;
  live::MoveList *frozen_move;
  std::unordered_map<live::INodePtr, live::MoveList *> move_list;
  std::unordered_map<live::INodePtr, int> degrees;
  std::unordered_map<live::INodePtr, live::INodeListPtr> adj_list;
  std::unordered_map<live::INodePtr, live::INodePtr> alias;
  std::unordered_map<temp::Temp *, int> color;
  std::set<std::pair<live::INodePtr, live::INodePtr>> adj_set;

  bool MoveRelated(live::INodePtr);
  bool OK(live::INodePtr, live::INodePtr);
  bool AllOK(std::list<live::INodePtr> &, live::INodePtr);
  bool Conservertive(live::INodeListPtr);
  void Decrement(live::INodePtr);
  void EnableMoves(live::INodeListPtr);
  void AddWorkList(live::INodePtr);
  void Combine(live::INodePtr, live::INodePtr);
  void FreezeMove(live::INodePtr);
  void Init(std::set<int> &);
  live::INodePtr GetAlias(live::INodePtr);
  live::MoveList *NodeMoves(live::INodePtr);
  live::INodeListPtr Adjacent(live::INodePtr);

  Color(live::LiveGraph);
  ~Color();
  void Build();
  void AddEdge(live::INodePtr, live::INodePtr);
  void MakeWorkList();
  void Simplify();
  void Coalesce();
  void Freeze();
  void SelectSpill();
  void Assign();
  Result Exe();

  Result res;

};
} // namespace col

#endif // TIGER_COMPILER_COLOR_H
