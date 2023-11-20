#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {

/* TODO: Put your lab5 code here */
X64RegManager::X64RegManager() : RegManager() {
  temp::Temp *rax = temp::TempFactory::NewTemp();
  temp::Temp *rbx = temp::TempFactory::NewTemp();
  temp::Temp *rcx = temp::TempFactory::NewTemp();
  temp::Temp *rdx = temp::TempFactory::NewTemp();
  temp::Temp *rdi = temp::TempFactory::NewTemp();
  temp::Temp *rsi = temp::TempFactory::NewTemp();
  temp::Temp *rbp = temp::TempFactory::NewTemp();
  temp::Temp *rsp = temp::TempFactory::NewTemp();
  temp::Temp *r8 = temp::TempFactory::NewTemp();
  temp::Temp *r9 = temp::TempFactory::NewTemp();
  temp::Temp *r10 = temp::TempFactory::NewTemp();
  temp::Temp *r11 = temp::TempFactory::NewTemp();
  temp::Temp *r12 = temp::TempFactory::NewTemp();
  temp::Temp *r13 = temp::TempFactory::NewTemp();
  temp::Temp *r14 = temp::TempFactory::NewTemp();
  temp::Temp *r15 = temp::TempFactory::NewTemp();

  std::string *rax_ = new std::string("rax");
  std::string *rbx_ = new std::string("rbx");
  std::string *rcx_ = new std::string("rcx");
  std::string *rdx_ = new std::string("rdx");
  std::string *rdi_ = new std::string("rdi");
  std::string *rsi_ = new std::string("rsi");
  std::string *rbp_ = new std::string("rbp");
  std::string *rsp_ = new std::string("rsp");
  std::string *r8_ = new std::string("r8");
  std::string *r9_ = new std::string("r9");
  std::string *r10_ = new std::string("r10");
  std::string *r11_ = new std::string("r11");
  std::string *r12_ = new std::string("r12");
  std::string *r13_ = new std::string("r13");
  std::string *r14_ = new std::string("r14");
  std::string *r15_ = new std::string("r15");

  this->regs_.emplace_back(rax);
  this->regs_.emplace_back(rbx);
  this->regs_.emplace_back(rcx);
  this->regs_.emplace_back(rdx);
  this->regs_.emplace_back(rdi);
  this->regs_.emplace_back(rsi);
  this->regs_.emplace_back(rbp);
  this->regs_.emplace_back(rsp);
  this->regs_.emplace_back(r8);
  this->regs_.emplace_back(r9);
  this->regs_.emplace_back(r10);
  this->regs_.emplace_back(r11);
  this->regs_.emplace_back(r12);
  this->regs_.emplace_back(r13);
  this->regs_.emplace_back(r14);
  this->regs_.emplace_back(r15);

  this->temp_map_->Enter(rax, rax_);
  this->temp_map_->Enter(rbx, rbx_);
  this->temp_map_->Enter(rcx, rcx_);
  this->temp_map_->Enter(rdx, rdx_);
  this->temp_map_->Enter(rdi, rdi_);
  this->temp_map_->Enter(rsi, rsi_);
  this->temp_map_->Enter(rbp, rbp_);
  this->temp_map_->Enter(rsp, rsp_);
  this->temp_map_->Enter(r8, r8_);
  this->temp_map_->Enter(r9, r9_);
  this->temp_map_->Enter(r10, r10_);
  this->temp_map_->Enter(r11, r11_);
  this->temp_map_->Enter(r12, r12_);
  this->temp_map_->Enter(r13, r13_);
  this->temp_map_->Enter(r14, r14_);
  this->temp_map_->Enter(r15, r15_);
  
}

temp::TempList *X64RegManager::Registers() {
  temp::TempList *reg = new temp::TempList();
  for (auto &r : this->regs_) {
    reg->Append(r);
  }
  return reg;
}

temp::TempList *X64RegManager::ArgRegs() {
  return new temp::TempList({this->regs_[4], this->regs_[5], this->regs_[3], this->regs_[2], this->regs_[8], this->regs_[9]});
}

temp::TempList *X64RegManager::CallerSaves() {
    return new temp::TempList({this->regs_[4], this->regs_[5], this->regs_[3], this->regs_[2], this->regs_[8], this->regs_[9], this->regs_[0], this->regs_[10], this->regs_[11]});
}

temp::TempList *X64RegManager::CalleeSaves() {
  return new temp::TempList({regs_[1], regs_[6], regs_[12], regs_[13], regs_[14], regs_[15]});
}

temp::TempList *X64RegManager::ReturnSink() {
  auto list = this->CallerSaves();
  list->Append(StackPointer());
  list->Append(ReturnValue());
  return list;
}

temp::Temp *X64RegManager::FramePointer() {
  return regs_[6];
}

temp::Temp *X64RegManager::StackPointer() {
  return regs_[7];
}

temp::Temp *X64RegManager::ReturnValue() {
  return regs_[0];
}

X64Frame::X64Frame(temp::Label *name, std::list<bool> formals) {
  labels_ = name;
  auto reg_list = reg_manager->ArgRegs()->GetList();
  auto frameptr = new tree::TempExp(reg_manager->FramePointer());
  this->offset_ = 0;
  for (auto f : formals) {
    formals_.emplace_back(AlloLocal(f));
  }

  auto iter = reg_list.begin();
  int index = 0;
  tree::MoveStm *move = nullptr;

  for (auto form : formals_) {
    if (index < 6) {
      move = new tree::MoveStm(form->ToExp(frameptr), new tree::TempExp(*iter));
    } else {
      move = new tree::MoveStm(form->ToExp(frameptr), new tree::MemExp(
        new tree::BinopExp(tree::PLUS_OP, frameptr, new tree::ConstExp((index - 5) * reg_manager->WordSize()))));
    }
    index++;
    iter++;
    view_shift_.emplace_back(move);
  }
}

frame::Access *X64Frame::AlloLocal(bool in_f) {
  if (in_f) {
    this->offset_ -= reg_manager->WordSize();
    return new InFrameAccess(offset_);
  } else {
    return new InRegAccess(temp::TempFactory::NewTemp());
  }
}

temp::Temp *X64Frame::FramePointer() const {
  return reg_manager->FramePointer();
}

tree::Exp *ExternalCall(std::string str, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(str)), args);
}

std::list<tree::Stm *> ProcEntryExit1(Frame *frame, tree::Stm *stm) {
  std::list<tree::Stm *> stmlist;
  auto callee = reg_manager->CalleeSaves()->GetList();
  std::vector<temp::Temp *> saved;

  for (auto &reg : callee) {
    auto temp = temp::TempFactory::NewTemp();
    saved.emplace_back(temp);
    stmlist.emplace_back(new tree::MoveStm(new tree::TempExp(temp), new tree::TempExp(reg)));
  }

  auto view = frame->view_shift_;
  stmlist.splice(stmlist.end(), view);
  stmlist.emplace_back(stm);

  auto callee_iter = callee.begin();
  auto save_iter = saved.begin();
  for (; callee_iter != callee.end() && save_iter != saved.end(); callee_iter++, save_iter++) {
    stmlist.emplace_back(new tree::MoveStm(new tree::TempExp(*callee_iter), new tree::TempExp(*save_iter)));
  }
  return stmlist;
}

} // namespace frame