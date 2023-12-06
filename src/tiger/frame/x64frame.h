//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:

  X64RegManager();
  [[nodiscard]] temp::TempList *Registers() override;
  [[nodiscard]] temp::TempList *ArgRegs() override;
  [[nodiscard]] temp::TempList *CallerSaves() override;
  [[nodiscard]] temp::TempList *CalleeSaves() override;
  [[nodiscard]] temp::TempList *ReturnSink() override;
  [[nodiscard]] int WordSize() override { return 8; };
  [[nodiscard]] temp::Temp *FramePointer() override;
  [[nodiscard]] temp::Temp *StackPointer() override;
  [[nodiscard]] temp::Temp *ReturnValue() override;
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame(temp::Label *name, std::list<bool> &formals);
  Access *AlloLocal(bool in_f) override;
  temp::Temp *FramePointer() const override;
};

/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset), Access(true) {}
  tree::Exp *ToExp(tree:: Exp *f_ptr) const override {
    return new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, f_ptr, new tree::ConstExp(this->offset)));
  }
  /* TODO: Put your lab5 code here */
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg), Access(false) {}
  tree::Exp *ToExp(tree::Exp *f_ptr) const override {
    return new tree::TempExp(reg);
  }
  /* TODO: Put your lab5 code here */
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
