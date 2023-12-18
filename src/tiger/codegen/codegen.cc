#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>
#include <iostream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  fs_ = frame_->labels_->Name() + "_framesize";
  auto instrs = new assem::InstrList();
  for (auto stm : traces_->GetStmList()->GetList()) {
    stm->Munch(*instrs, fs_);
  }
  assem_instr_ = std::make_unique<AssemInstr>(frame::ProcEntryExit2(instrs));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(label_->Name(), label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::OperInstr("jmp `j0", new temp::TempList{}, new temp::TempList{}, new assem::Targets(jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::string jxx;
  switch (op_)
  {
  case tree::RelOp::EQ_OP:
    jxx = "je";
    break;
  case tree::RelOp::NE_OP:
    jxx = "jne";
    break;
  case tree::RelOp::LE_OP:
    jxx = "jle";
    break;
  case tree::RelOp::LT_OP:
    jxx = "jl";
    break;
  case tree::RelOp::GE_OP:
    jxx = "jge";
    break;
  case tree::RelOp::GT_OP:
    jxx = "jg";
    break;
  default:
    break;
  }
  auto left = left_->Munch(instr_list, fs);
  auto right = right_->Munch(instr_list, fs);
  std::stringstream assem_stream;
  assem_stream << jxx << " `j0";
  instr_list.Append(new assem::OperInstr(
    "cmpq `s0, `s1", new temp::TempList{}, new temp::TempList{right, left}, nullptr
  ));
  instr_list.Append(new assem::OperInstr(
    assem_stream.str(), new temp::TempList{}, new temp::TempList{}, 
      new assem::Targets(new std::vector<temp::Label *>{true_label_, false_label_})
  ));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if (typeid(*dst_) == typeid(MemExp)) {
    auto dst_mem = static_cast<MemExp *>(dst_);
    if (typeid(*dst_mem->exp_) == typeid(BinopExp)) {
      auto dst_bin = static_cast<BinopExp *>(dst_mem->exp_);
      if (dst_bin->op_ == PLUS_OP) {
        if (typeid(*dst_bin->left_) == typeid(ConstExp)) {
          auto e1 = dst_bin->right_;
          auto e2 = src_;
          auto dst_reg = e1->Munch(instr_list, fs);
          auto src_reg = e2->Munch(instr_list, fs);
          auto idx = static_cast<ConstExp *>(dst_bin->left_)->consti_;
          if (dst_reg == reg_manager->FramePointer()) {
            std::string mov = "movq `s0, (" + std::string(fs.data()) + " + " + std::to_string(idx) + ")" + "(`s1)";
            instr_list.Append(new assem::OperInstr(
              mov, nullptr, new temp::TempList{src_reg, reg_manager->StackPointer()}, nullptr
            ));
          } else {
            std::string mov = "movq `s0, " + std::to_string(idx) + "(`s1)";
            instr_list.Append(new assem::OperInstr(
              mov, nullptr, new temp::TempList{src_reg, dst_reg}, nullptr
            ));
          }
          return;
        }
        if (typeid(*dst_bin->right_) == typeid(ConstExp)) {
          auto e1 = dst_bin->left_;
          auto e2 = src_;
          auto idx = static_cast<ConstExp *>(dst_bin->right_)->consti_;
          auto dst_reg = e1->Munch(instr_list, fs);
          auto src_reg = e2->Munch(instr_list, fs);
          if (dst_reg == reg_manager->FramePointer()) {
            std::string mov = "movq `s0, (" + std::string(fs.data()) + " + " + std::to_string(idx) + ")(`s1)";
            instr_list.Append(new assem::OperInstr(
              mov, nullptr, new temp::TempList{src_reg, reg_manager->StackPointer()}, nullptr
            ));
          } else {
            std::string mov = "movq `s0, " + std::to_string(idx) + "(`s1)";
            instr_list.Append(new assem::OperInstr(
              mov, nullptr, new temp::TempList{src_reg, dst_reg}, nullptr
            ));
          } 
          return;
        }
      }
    }
  
    auto src = src_->Munch(instr_list, fs);
    auto dst = dst_mem->exp_->Munch(instr_list, fs);
    auto mov = "movq `s0, (`s1)";
    instr_list.Append(new assem::OperInstr(
      mov, nullptr, new temp::TempList{src, dst}, nullptr
    ));
    return;
  }
  if (typeid(*src_) == typeid(MemExp)) {
    auto src_mem = static_cast<MemExp *>(src_);
    if (typeid(*src_mem->exp_) == typeid(BinopExp)) {
      auto src_bin = static_cast<BinopExp *>(src_mem->exp_);
      if (src_bin->op_ == PLUS_OP) {
        if (typeid(*src_bin->left_) == typeid(ConstExp)) {
          auto e1 = src_bin->right_;
          auto e2 = dst_;
          auto idx = static_cast<ConstExp *>(src_bin->left_)->consti_;
          auto src_reg = e1->Munch(instr_list, fs);
          auto dst_reg = e2->Munch(instr_list, fs);
          if (src_reg == reg_manager->FramePointer()) {
            auto mov = "movq (" + std::string(fs.data()) + " + " + std::to_string(idx) + ")(`s0), `d0";
            instr_list.Append(new assem::OperInstr(
              mov, new temp::TempList{dst_reg}, new temp::TempList{reg_manager->StackPointer()}, nullptr
            ));
          } else {
            std::string mov = "movq " + std::to_string(idx) + "(`s0), `d0";
            instr_list.Append(new assem::OperInstr(
              mov, new temp::TempList{dst_reg}, new temp::TempList{src_reg}, nullptr
            ));
          } 
          return;
        }
        if (typeid(*src_bin->right_) == typeid(ConstExp)) {
          auto e1 = src_bin->left_;
          auto e2 = dst_;
          auto idx = static_cast<ConstExp *>(src_bin->right_)->consti_;
          auto src_reg = e1->Munch(instr_list, fs);
          auto dst_reg = e2->Munch(instr_list, fs);
          if (src_reg == reg_manager->FramePointer()) {
            auto mov = "movq (" + std::string(fs.data()) + " + " + std::to_string(idx) + ")(`s0), `d0";
            instr_list.Append(new assem::OperInstr(
              mov, new temp::TempList{dst_reg}, new temp::TempList{reg_manager->StackPointer()}, nullptr
            ));
          } else {
            auto mov = "movq " + std::to_string(idx) + "(`s0), `d0";
            instr_list.Append(new assem::OperInstr(
              mov, new temp::TempList{dst_reg}, new temp::TempList{src_reg}, nullptr
            ));
          }
          return;
        }
        auto left_temp = src_bin->left_->Munch(instr_list, fs);
        auto right_temp = src_bin->right_->Munch(instr_list, fs);
        auto dst_reg = dst_->Munch(instr_list, fs);
        instr_list.Append(
          new assem::OperInstr(
            "movq (`s0,`s1),`d0", new temp::TempList(dst_reg), new temp::TempList{left_temp,right_temp}, nullptr
          )
        );
        return;
      }
    }
  
    auto dst_reg = dst_->Munch(instr_list, fs);
    auto src_reg = src_mem->exp_->Munch(instr_list, fs);
    auto mov = "movq (`s0), `d0";
    instr_list.Append(new assem::OperInstr(
      mov, new temp::TempList{dst_reg}, new temp::TempList{src_reg}, nullptr
    ));
    return;
  }

  if(typeid(*src_) == typeid(ConstExp)){
    auto src = static_cast<ConstExp *>(src_);
    auto dst = dst_->Munch(instr_list, fs);
    auto mov = "movq $" + std::to_string(src->consti_) + ", `d0";
    instr_list.Append(new assem::OperInstr(
        mov, new temp::TempList{dst}, new temp::TempList{}, nullptr
    ));
    return;
  }


  //fp -> sp
  if(typeid(*src_) == typeid(BinopExp)){
    auto dst_temp = dst_->Munch(instr_list,fs);
    auto src = static_cast<BinopExp*>(src_);
    if(src->op_==PLUS_OP){
      std::stringstream assem;
      assem<<"leaq ";
      if (typeid(*src->left_) == typeid(ConstExp)) {
          auto left = static_cast<ConstExp *>(src->left_);
          auto right_temp = src->right_->Munch(instr_list, fs);
          if (right_temp == reg_manager->FramePointer()) {
            assem<<"("<<fs;
            if(left->consti_>=0){
              assem<<"+";
            }
            assem<<left->consti_<<")(`s0)";
            right_temp = reg_manager->StackPointer();
          } else {
            assem<<left->consti_<<"(`s0)";
          }
          assem<<",`d0";
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(dst_temp),
              new temp::TempList{right_temp},
              nullptr
            )
          );
          return;
        }
      if (typeid(*src->right_) == typeid(ConstExp)) {
          auto right = static_cast<ConstExp *>(src->right_);
          auto left_temp = src->left_->Munch(instr_list, fs);
          if (left_temp == reg_manager->FramePointer()) {
            assem<<"("<<fs;
            if(right->consti_>=0){
              assem<<"+";
            }
            assem<<right->consti_<<")(`s0)";
            left_temp = reg_manager->StackPointer();
          } else {
            assem<<right->consti_<<"(`s0)";
          }
          assem<<",`d0";
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(dst_temp),
              new temp::TempList{left_temp},
              nullptr
            )
          );
          return;
        }

      assem<<"(`s0,`s1),`d0";
      auto left_temp = src->left_->Munch(instr_list,fs);
      auto right_temp = src->right_->Munch(instr_list,fs);
      instr_list.Append(
        new assem::OperInstr(
          assem.str(),
          new temp::TempList(dst_temp),
          new temp::TempList{left_temp,right_temp},
          nullptr
        )
      );
      return;
    }

  }
  
  auto src_reg = src_->Munch(instr_list,fs);
  auto dst_reg = dst_->Munch(instr_list,fs);
  auto mov = "movq `s0, `d0";
  instr_list.Append(new assem::MoveInstr(
    mov, new temp::TempList{dst_reg}, new temp::TempList{src_reg}
  ));
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list,fs);

}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto left = left_->Munch(instr_list, fs);
  auto right = right_->Munch(instr_list, fs);
  auto res = temp::TempFactory::NewTemp();
  std::string mov = "movq `s0, `d0";
  instr_list.Append(new assem::MoveInstr(
    mov, new temp::TempList{res}, new temp::TempList{left}
  ));
  switch (op_)
  { 
  case PLUS_OP:
    instr_list.Append(new assem::OperInstr(
      "addq `s0, `d0", new temp::TempList{res}, new temp::TempList{right}, nullptr
    ));
    break;
  case MINUS_OP:
    instr_list.Append(new assem::OperInstr(
      "subq `s0, `d0", new temp::TempList{res}, new temp::TempList{right}, nullptr
    ));
    break;
  case MUL_OP:{
    auto mov1 = "movq `s0, %rax";
    instr_list.Append(new assem::MoveInstr(
      mov1, new temp::TempList{reg_manager->GetRegister(0)}, new temp::TempList{res}
    ));
    auto mov2 = "imulq `s0";
    instr_list.Append(new assem::OperInstr(
      mov2, new temp::TempList{reg_manager->GetRegister(0), reg_manager->GetRegister(3)}, new temp::TempList{right, reg_manager->GetRegister(0)}, nullptr
    ));
    auto mov3 = "movq `s0, `d0";
    instr_list.Append(new assem::MoveInstr(
      mov3, new temp::TempList{res}, new temp::TempList{reg_manager->GetRegister(0)}
    ));
  }
  break;
  case DIV_OP:
  {
    auto mov1 = "movq `s0, %rax";
    instr_list.Append(new assem::MoveInstr(
    mov1, new temp::TempList{reg_manager->GetRegister(0)}, new temp::TempList{res}
    ));
    auto cq = "cqto";
    instr_list.Append(new assem::OperInstr(
      cq, new temp::TempList{reg_manager->GetRegister(3), reg_manager->GetRegister(0)}, new temp::TempList{reg_manager->GetRegister(0)}, nullptr
    ));
    auto div = "idivq `s0";
    instr_list.Append(new assem::OperInstr(
      div, new temp::TempList{reg_manager->GetRegister(3), reg_manager->GetRegister(0)}, new temp::TempList{right, reg_manager->GetRegister(0), reg_manager->GetRegister(3)}, nullptr
    ));
    auto mov2 = "movq `s0, `d0";
    instr_list.Append(new assem::MoveInstr(
      mov2, new temp::TempList(res), new temp::TempList(reg_manager->GetRegister(0))
    ));
  }
  default:
    break;
  }
  return res;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto temp = temp::TempFactory::NewTemp();
  if (typeid(*exp_) == typeid(BinopExp)) {
    auto exp_bin = static_cast<BinopExp *>(exp_);
    if (exp_bin->op_ == PLUS_OP) {
      if (typeid(*exp_bin->left_) == typeid(ConstExp)) {
        auto lef = static_cast<ConstExp *>(exp_bin->left_);
        auto src_reg = exp_bin->right_->Munch(instr_list, fs);
        int idx = lef->consti_;
        if (src_reg == reg_manager->FramePointer()) {
          auto mov = "movq (" + std::string(fs.data()) + " + " + std::to_string(idx) + ")(`s0), `d0";
          instr_list.Append(new assem::OperInstr(
            mov, new temp::TempList{temp}, new temp::TempList{reg_manager->StackPointer()}, nullptr
          ));
        } else {
          auto mov = "movq " + std::to_string(idx) + "(`s0), `d0";
          instr_list.Append(new assem::OperInstr(
            mov, new temp::TempList{temp}, new temp::TempList{src_reg}, nullptr
          ));
        }
      } else if (typeid(*exp_bin->right_) == typeid(ConstExp)) {
        auto rig = static_cast<ConstExp *>(exp_bin->right_);
        auto src_reg = exp_bin->left_->Munch(instr_list, fs);
        int idx = rig->consti_;
        if (src_reg == reg_manager->FramePointer()) {
          auto mov = "movq (" + std::string(fs.data()) + " + " + std::to_string(idx) + ")(`s0), `d0";
          instr_list.Append(new assem::OperInstr(
            mov, new temp::TempList{temp}, new temp::TempList{reg_manager->StackPointer()}, nullptr
          ));
        } else {
          auto mov = "movq " + std::to_string(idx) + "(`s0), `d0";
          instr_list.Append(new assem::OperInstr(
            mov, new temp::TempList{temp}, new temp::TempList{src_reg}, nullptr
          ));
        }
      } else {
        auto lef = exp_bin->left_->Munch(instr_list, fs);
        auto rig = exp_bin->right_->Munch(instr_list, fs);
        instr_list.Append(new assem::OperInstr(
          "movq (`s0,`s1), `d0", new temp::TempList{temp}, new temp::TempList{lef, rig}, nullptr
        ));
      }
    }
    return temp;
  }
  auto mv = "movq (`s0), `d0";
  instr_list.Append(new assem::OperInstr(
    mv, new temp::TempList{temp}, new temp::TempList(exp_->Munch(instr_list, fs)), nullptr
  ));
  return temp;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list,fs);
  return exp_->Munch(instr_list,fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto tmp = temp::TempFactory::NewTemp();
  auto mov = "leaq " + temp::LabelFactory::LabelString(name_) + "(%rip), `d0";
  instr_list.Append(new assem::OperInstr(
    mov, new temp::TempList{tmp}, new temp::TempList{}, nullptr
  ));
  return tmp;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto tmp = temp::TempFactory::NewTemp();
  auto mov = "movq $" + std::to_string(consti_) + ", `d0";
  instr_list.Append(new assem::OperInstr(
    mov, new temp::TempList{tmp}, new temp::TempList{}, nullptr
  ));
  return tmp;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto args = args_->MunchArgs(instr_list, fs);
  auto arg_num = args_->GetList().size();
  auto arg_size = reg_manager->ArgRegs()->GetList().size();
  auto rsp_sub = (arg_num - arg_size) * reg_manager->WordSize();
  if (arg_num > arg_size) {
    auto sub = "subq $" + std::to_string(rsp_sub) + ", `d0";
    instr_list.Append(new assem::OperInstr(
      sub, new temp::TempList{reg_manager->StackPointer()}, new temp::TempList(), nullptr
    ));
  }
  auto calldefs = reg_manager->CallerSaves();
  calldefs->Append(reg_manager->ReturnValue());

  auto cal = "callq " + temp::LabelFactory::LabelString(static_cast<NameExp *>(fun_)->name_);
  instr_list.Append(new assem::OperInstr(
    cal, calldefs, args, nullptr
  ));

  if (arg_num > arg_size) {
    auto add = "addq $" + std::to_string(rsp_sub) + ", `d0";
    instr_list.Append(new assem::OperInstr(
      add, new temp::TempList(reg_manager->StackPointer()), new temp::TempList{}, nullptr
    ));
  }
  return reg_manager->ReturnValue();
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto args = reg_manager->ArgRegs();
  auto arg_size = args->GetList().size();
  int arg_num = exp_list_.size();
  auto iter = args->GetList().begin();
  auto word_sz = reg_manager->WordSize();
  auto tmps = new temp::TempList();
  auto i = 0;
  for (auto &exp : exp_list_) {
    auto tmp = exp->Munch(instr_list, fs);
    if (i < arg_size) {
      tmps->Append(*iter);
      if (i == 0 && tmp == reg_manager->FramePointer()) {
        std::string mov = "leaq " + std::string(fs.data()) + "(`s0), `d0"; 
        instr_list.Append(new assem::OperInstr(
          mov, new temp::TempList{*iter}, new temp::TempList{reg_manager->StackPointer()}, nullptr
        ));
      } else {
        auto mov = "movq `s0, `d0";
        instr_list.Append(new assem::MoveInstr(
          mov, new temp::TempList{*iter}, new temp::TempList{tmp}
        ));
      }
      iter++;
    } else {
      auto mov = "movq `s0, " + std::to_string((word_sz * (i - arg_num))) + "(`s1)";
      instr_list.Append(new assem::OperInstr(
        mov, new temp::TempList(), new temp::TempList{tmp, reg_manager->StackPointer()}, nullptr
      ));
    }
    tmps->Append(tmp);
    i++;
  }
  if (i > arg_size) {
    tmps->Append(reg_manager->StackPointer());
  }
  return tmps;
}

} // namespace tree
