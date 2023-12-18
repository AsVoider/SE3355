#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"
#include <list>

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result() { };
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
public:
  frame::Frame *frame_;
  cg::AssemInstr *instr_list_;
  std::list<temp::Temp *> new_temps;
  temp::Map *Colors;
  live::INodeListPtr spills;
  RegAllocator(frame::Frame *fr, std::unique_ptr<cg::AssemInstr> instrlist) :frame_(fr), instr_list_(
    new cg::AssemInstr(instrlist->GetInstrList())) {
      spills = new live::INodeList();
      Colors = nullptr;
    }
  void RegAlloc();
  // void Prepare()
  assem::InstrList *ReWrite(std::list<temp::Temp *> &);
  std::unique_ptr<ra::Result> TransferResult();
};

} // namespace ra

#endif