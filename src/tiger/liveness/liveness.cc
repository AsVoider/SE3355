#include "tiger/liveness/liveness.h"
#include <iostream>
#include <set>
extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

temp::TempList *Union(temp::TempList *a, temp::TempList *b) {
  auto ret = new temp::TempList{};
  auto ret_set = std::set<temp::Temp *>();
  if (a) {
    for (auto &a_temp : a->GetList()) {
      ret_set.insert(a_temp);
    }
  }
  if(b) {
    for (auto &b_temp : b->GetList()) {
      ret_set.insert(b_temp);
    }
  }
  for (auto &ret_temp : ret_set) {
    ret->Append(ret_temp);
  }
  return ret;
}

temp::TempList *Diff(temp::TempList *a, temp::TempList *b) {
  if (!a) {
    return new temp::TempList{};
  }
  if (!b) {
    return a;
  }
  auto ret_set = std::set<temp::Temp *>(a->GetList().begin(), a->GetList().end());
  for (auto &b_temp : b->GetList()) {
    if (ret_set.find(b_temp) != ret_set.end()) {
      ret_set.erase(b_temp);
    }
  }
  auto ret = new temp::TempList{};
  for (auto &ret_temp : ret_set) {
    ret->Append(ret_temp);
  }
  return ret;
}

bool Equal(temp::TempList *a, temp::TempList *b) {
  if (!a && !b)
    return true;
  auto a_list = std::list<temp::Temp *>();
  auto b_list = std::list<temp::Temp *>();
  if (a) {
    a_list = a->GetList();
  }
  if (b) {
    b_list = b->GetList();
  }
  auto a_set = std::set<temp::Temp *>(a_list.begin(), a_list.end());
  auto b_set = std::set<temp::Temp *>(b_list.begin(), b_list.end());
  return a_set == b_set;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  for (auto &node : flowgraph_->Nodes()->GetList()) {
    in_->Enter(node, new temp::TempList{});
    out_->Enter(node, new temp::TempList{});
  }
  bool done = false;
  while (!done) {
    done = true;
    for (auto &nodeptr : flowgraph_->Nodes()->GetList()) {
      auto instr = nodeptr->NodeInfo();
      auto new_in = Union(instr->Use(), Diff(out_->Look(nodeptr), instr->Def()));
      if (!Equal(new_in, in_->Look(nodeptr))) {
        done = false;
        in_->Enter(nodeptr, new_in);
      }

      auto succ_list = nodeptr->Succ()->GetList();
      temp::TempList *out = nullptr;
      for (auto &succ : succ_list) {
        out = Union(out, in_->Look(succ));
      }
      if (!Equal(out, out_->Look(nodeptr))) {
        done = false;
        out_->Enter(nodeptr, out);
      }
    }
  }

}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  auto precolor = reg_manager->ExceptRsp()->GetList();
  for (auto &temp : precolor) {
    auto new_node = live_graph_.interf_graph->NewNode(temp);
    temp_node_map_->Enter(temp, new_node);
  }

  for (auto &temp1 : precolor) {
    for (auto &temp2 : precolor) {
      auto node1 = temp_node_map_->Look(temp1);
      auto node2 = temp_node_map_->Look(temp2);
      if (node1 != node2) {
        live_graph_.interf_graph->AddEdge(node1, node2);
        live_graph_.interf_graph->AddEdge(node1, node2);
      }
    }
  }

  for (auto &flow_node : flowgraph_->Nodes()->GetList()) {
    auto out_list = Union(flow_node->NodeInfo()->Def(), flow_node->NodeInfo()->Use());
    for (auto &out : out_list->GetList()) {
      auto new_node = temp_node_map_->Look(out);
      if (out == reg_manager->StackPointer()) {
        continue;
      }
      if (temp_node_map_->Look(out) == nullptr) {
        auto new_node = live_graph_.interf_graph->NewNode(out);
        temp_node_map_->Enter(out, new_node);
      }
    }
  }

  for(auto node : flowgraph_->Nodes()->GetList()) {
    temp::TempList *out_live = out_->Look(node);

    if(typeid(*node->NodeInfo()) != typeid(assem::MoveInstr)) {
      auto node_def = node->NodeInfo()->Def();
      if (node_def == nullptr)
        continue;
      for (auto &def : node_def->GetList()) {
        if (def == reg_manager->StackPointer())
          continue;
        for (auto &live_out : out_live->GetList()) {
          if (live_out == reg_manager->StackPointer() || live_out == def)
            continue;
          auto def_node = temp_node_map_->Look(def);
          auto out_node = temp_node_map_->Look(live_out);
          live_graph_.interf_graph->AddEdge(def_node, out_node);
          live_graph_.interf_graph->AddEdge(out_node, def_node);
        }
      }
    } else {
      auto node_def = node->NodeInfo()->Def();
      auto node_use = node->NodeInfo()->Use();
      for (auto &def : node_def->GetList()) {
        auto def_node = temp_node_map_->Look(def);
        for (auto &use : node_use->GetList()) {
          auto use_node = temp_node_map_->Look(use);
          if (live_graph_.moves->Contain(use_node, def_node))
            continue;
          if (def == reg_manager->StackPointer() || use == reg_manager->StackPointer() || def_node == use_node)
            continue;
          live_graph_.moves->Append(use_node, def_node);
        }
        auto live_without_use = Diff(out_live, node_use);
        for (auto &live_reg : live_without_use->GetList()) {
          if (live_reg == reg_manager->StackPointer() || def == reg_manager->StackPointer() || def == live_reg)
            continue;
          auto live_node = temp_node_map_->Look(live_reg);
          live_graph_.interf_graph->AddEdge(def_node, live_node);
          live_graph_.interf_graph->AddEdge(live_node, def_node);
        }
      }
    }
  }
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
