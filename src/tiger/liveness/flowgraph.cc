#include "tiger/liveness/flowgraph.h"
#include <list>
namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */

  auto jmp_tars = std::list<std::pair<FNodePtr, assem::Targets *>>();
  
  FNodePtr last_node = nullptr;
  for (auto &instr : instr_list_->GetList()) {
    auto new_node = flowgraph_->NewNode(instr);
    if (last_node) {
      flowgraph_->AddEdge(last_node, new_node);
    }
    if (typeid(*instr) == typeid(assem::LabelInstr)) {
      auto label = static_cast<assem::LabelInstr *>(instr)->label_;
      label_map_->Enter(label, new_node);
    } else if (typeid(*instr) == typeid(assem::OperInstr)) {
      auto jmps = static_cast<assem::OperInstr *>(instr)->jumps_;
      if (jmps) {
        last_node = jmps->labels_->size() == 1 ? nullptr : new_node;
        jmp_tars.emplace_back(new_node, jmps);
        continue;
      }
    }
    last_node = new_node;
  }

  for (auto &jmp : jmp_tars) {
    for (auto &label : *jmp.second->labels_) {
      auto tar = label_map_->Look(label);
      flowgraph_->AddEdge(jmp.first, tar);
    }
  }
}
} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}
} // namespace assem
