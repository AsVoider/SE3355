#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  return new Access(level, level->frame_->AlloLocal(escape));
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    auto cjmp = new tree::CjumpStm(tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
    PatchList t({&cjmp->true_label_});
    PatchList f({&cjmp->false_label_});
    return Cx(t, f, cjmp);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    return Cx({}, {}, stm_);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    auto r = temp::TempFactory::NewTemp();
    auto t = temp::LabelFactory::NewLabel();
    auto f = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    return new tree::EseqExp(
      new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
        new tree::EseqExp(
          cx_.stm_,
            new tree::EseqExp(
              new tree::LabelStm(f),
                new tree::EseqExp(
                  new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(0)),
                    new tree::EseqExp(new tree::LabelStm(t), new tree::TempExp(r))
                )
            )
        )
    );
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    return cx_;
  }
};

tree::Stm *list_seq(std::list<tree::Stm *> stms) {
  tree::Stm *seq = nullptr;
  for (auto iter = stms.rbegin(); iter != stms.rend(); iter++) {
    if (!(*iter))
      continue;
    if (seq) 
      seq = new tree::SeqStm(*iter, seq);
    else
      seq = *iter;
  }
  return seq;
}

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  FillBaseTEnv();
  FillBaseVEnv();
  auto res = absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(), main_level_->frame_->labels_, errormsg_.get());
  auto frag = new frame::ProcFrag(list_seq(ProcEntryExit1(main_level_->frame_, res->exp_->UnNx())), main_level_->frame_);
  frags->PushBack(frag);
}

} // namespace tr

namespace absyn {
tree::Exp *static_link(tr::Level *now, tr::Level *old) {
 tree::Exp *static_link = new tree::TempExp(reg_manager->FramePointer());
  while (now != old) {
    static_link = now->frame_->get_static()->ToExp(static_link);
    now = now->parent_;
  }
  return static_link;
}


tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_et = static_cast<env::VarEntry *>(venv->Look(sym_));
  tree::Exp* sta = nullptr;
  if (typeid(*var_et->access_->access_) == typeid(frame::InFrameAccess)) {
    sta = static_link(level, var_et->access_->level_);
  }
  sta = var_et->access_->access_->ToExp(sta);
  return new tr::ExpAndTy(new tr::ExExp(sta), var_et->ty_->ActualTy());

}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_i = var_->Translate(venv, tenv, level, label, errormsg);
  auto ty = var_i->ty_->ActualTy();
  if (typeid(*ty) != typeid(type::RecordTy)) {
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  auto re_ty = static_cast<type::RecordTy *>(ty);
  auto fields = re_ty->fields_->GetList();
  auto index = 0;
  for (auto &f : fields) {
    if (this->sym_ == f->name_) {
      auto exp = new tree::BinopExp(tree::PLUS_OP, var_i->exp_->UnEx(), new tree::ConstExp(index * reg_manager->WordSize()));
      auto mem = new tr::ExExp(new tree::MemExp(exp));
      return new tr::ExpAndTy(mem, f->ty_->ActualTy());
    }
    index++;
  }

  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var = var_->Translate(venv, tenv, level, label, errormsg);
  if (typeid(*var->ty_->ActualTy()) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "in subscript var, the var should be array type\n");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }

  auto sub = subscript_->Translate(venv, tenv, level, label, errormsg);
  if(typeid(*sub->ty_->ActualTy()) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "the array index should be integer\n");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::MemExp(
        new tree::BinopExp(
          tree::PLUS_OP, var->exp_->UnEx(), 
          new tree::BinopExp(tree::MUL_OP, sub->exp_->UnEx(), new tree::ConstExp(reg_manager->WordSize()))))), 
  static_cast<type::ArrayTy *>(var->ty_)->ty_->ActualTy());
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto exp_ty = var_->Translate(venv, tenv, level, label, errormsg);
  return new tr::ExpAndTy(new tr::ExExp(exp_ty->exp_->UnEx()), exp_ty->ty_);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)), type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto str_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label, str_));
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(str_label)), type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto func = static_cast<env::FunEntry *>(venv->Look(func_));
  auto fun_label = func->label_;
  // auto staticlink = static_link(level, func->level_->parent_);
  auto explist = new tree::ExpList();
  if (fun_label != nullptr) {
    explist->Append(static_link(level, func->level_->parent_));
  }
  auto args = args_->GetList();
  for (auto arg : args) {
    auto res = arg->Translate(venv, tenv, level, label, errormsg);
    explist->Append(res->exp_->UnEx());
  }
  
  tree::Exp *callExp = nullptr;
  if (fun_label != nullptr) {
    callExp = new tree::CallExp(new tree::NameExp(func->label_), explist);
  } else {
    callExp = frame::ExternalCall(func_->Name(), explist);
  }

  type::Ty *res_ty = type::VoidTy::Instance();
  if (func->result_) {
    res_ty = func->result_;
  }

  return new tr::ExpAndTy(new tr::ExExp(callExp), res_ty);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto lef = left_->Translate(venv, tenv, level, label, errormsg);
  auto rig = right_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *ret;
  tree::CjumpStm *stm;
  tr::PatchList trues, falses;
  // tr::Cx lf, ri;
  switch (oper_)
  {
  case absyn::PLUS_OP:
    ret = new tr::ExExp(new tree::BinopExp(tree::PLUS_OP, lef->exp_->UnEx(), rig->exp_->UnEx()));
    break;
  case absyn::MINUS_OP:
    ret = new tr::ExExp(new tree::BinopExp(tree::MINUS_OP, lef->exp_->UnEx(), rig->exp_->UnEx()));
    break;
  case absyn::TIMES_OP:
    ret = new tr::ExExp(new tree::BinopExp(tree::MUL_OP, lef->exp_->UnEx(), rig->exp_->UnEx()));
    break;
  case absyn::DIVIDE_OP:
    ret = new tr::ExExp(new tree::BinopExp(tree::DIV_OP, lef->exp_->UnEx(), rig->exp_->UnEx()));
    break;
  case LT_OP:
    stm = new tree::CjumpStm(tree::LT_OP, lef->exp_->UnEx(), rig->exp_->UnEx(), nullptr, nullptr);
    trues = tr::PatchList({&stm->true_label_});
    falses = tr::PatchList({&stm->false_label_});
    ret = new tr::CxExp(trues, falses, stm);
    break;
  case GT_OP:
    stm = new tree::CjumpStm(tree::GT_OP, lef->exp_->UnEx(), rig->exp_->UnEx(), nullptr, nullptr);
    trues = tr::PatchList({&stm->true_label_});
    falses = tr::PatchList({&stm->false_label_});
    ret = new tr::CxExp(trues, falses, stm);
    break;
  case LE_OP:
    stm = new tree::CjumpStm(tree::LE_OP, lef->exp_->UnEx(), rig->exp_->UnEx(), nullptr, nullptr);
    trues = tr::PatchList({&stm->true_label_});
    falses = tr::PatchList({&stm->false_label_});
    ret = new tr::CxExp(trues, falses, stm);
    break;
  case GE_OP:
    stm = new tree::CjumpStm(tree::GE_OP, lef->exp_->UnEx(), rig->exp_->UnEx(), nullptr, nullptr);
    trues = tr::PatchList({&stm->true_label_});
    falses = tr::PatchList({&stm->false_label_});
    ret = new tr::CxExp(trues, falses, stm);
    break;
  case EQ_OP:
  case NEQ_OP:
    if (lef->ty_->IsSameType(type::StringTy::Instance())) {
      auto str_cmp = frame::ExternalCall("string_equal", new tree::ExpList({lef->exp_->UnEx(), rig->exp_->UnEx()}));
      if (oper_ == absyn::EQ_OP)
        stm = new tree::CjumpStm(tree::EQ_OP, str_cmp, new tree::ConstExp(1), nullptr, nullptr);
      else 
        stm = new tree::CjumpStm(tree::NE_OP, str_cmp, new tree::ConstExp(1), nullptr, nullptr);
    } else {
      if (oper_ == absyn::EQ_OP)
        stm = new tree::CjumpStm(tree::EQ_OP, lef->exp_->UnEx(), rig->exp_->UnEx(), nullptr, nullptr);
      else 
        stm = new tree::CjumpStm(tree::NE_OP, lef->exp_->UnEx(), rig->exp_->UnEx(), nullptr, nullptr);
    }
    trues = tr::PatchList({&stm->true_label_});
    falses = tr::PatchList({&stm->false_label_});
    ret = new tr::CxExp(trues, falses, stm);
    break;
  case AND_OP: {
    auto lef_cx = lef->exp_->UnCx(errormsg);
    auto rig_cx = rig->exp_->UnCx(errormsg);
    auto rig_label = temp::LabelFactory::NewLabel();
    lef_cx.trues_.DoPatch(rig_label);
    trues = rig_cx.trues_;
    falses = tr::PatchList::JoinPatch(lef_cx.falses_, rig_cx.falses_);
    ret = new tr::CxExp(trues, falses, new tree::SeqStm(lef_cx.stm_, new tree::SeqStm(new tree::LabelStm(rig_label), rig_cx.stm_)));
    break;
  }
  case OR_OP: {
    auto lef_cx = lef->exp_->UnCx(errormsg);
    auto rig_cx = rig->exp_->UnCx(errormsg);
    auto rig_label = temp::LabelFactory::NewLabel();
    lef_cx.falses_.DoPatch(rig_label);
    trues = tr::PatchList::JoinPatch(lef_cx.trues_, rig_cx.trues_);
    falses = rig_cx.falses_;
    ret = new tr::CxExp(trues, falses, new tree::SeqStm(lef_cx.stm_, new tree::SeqStm(new tree::LabelStm(rig_label), rig_cx.stm_)));
    break;
  }
  default:
    break;
  }
  return new tr::ExpAndTy(ret, type::IntTy::Instance());
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto tmp = temp::TempFactory::NewTemp();
  tree::Stm *ret = nullptr;
  auto number = 0;
  for (auto efield : fields_->GetList()) {
    auto tr_exp = efield->exp_->Translate(venv, tenv, level, label, errormsg);
    if (ret == nullptr) {
      ret = new tree::MoveStm(new tree::MemExp(new tree::BinopExp(
        tree::PLUS_OP, new tree::TempExp(tmp), new tree::ConstExp(number * reg_manager->WordSize())
      )), tr_exp->exp_->UnEx());
    } else {
      ret = new tree::SeqStm(ret, new tree::MoveStm(new tree::MemExp(new tree::BinopExp(
        tree::PLUS_OP, new tree::TempExp(tmp), new tree::ConstExp(number * reg_manager->WordSize())
      )), tr_exp->exp_->UnEx()));
    }
    number++;
  }

  auto mem_alloc = new tree::MoveStm(new tree::TempExp(tmp), frame::ExternalCall(
    "alloc_record", new tree::ExpList({new tree::ConstExp(number * reg_manager->WordSize())})
  ));

  ret = new tree::SeqStm(mem_alloc, ret);
  auto retrn = new tr::ExExp(new tree::EseqExp(ret, new tree::TempExp(tmp)));
  return new tr::ExpAndTy(retrn, tenv->Look(typ_)->ActualTy());
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto exp_list = seq_->GetList();
  tree::Exp *ret = nullptr;
  tree::Stm *ret_stm = nullptr;
  type::Ty *ret_type = nullptr;
  for (auto ep : exp_list) {
    auto ep_ty = ep->Translate(venv, tenv, level, label, errormsg);
    if (ep == seq_->GetList().back()) {
      ret_type = ep_ty->ty_->ActualTy();
      if (ret_stm == nullptr) {
        ret = ep_ty->exp_->UnEx();
        } else {
        ret = new tree::EseqExp(ret_stm, ep_ty->exp_->UnEx());
        } 
      }else {
        if (ret_stm == nullptr) {
          ret_stm = ep_ty->exp_->UnNx();
        } else {
          ret_stm = new tree::SeqStm(ret_stm, ep_ty->exp_->UnNx());
        }
      }
    }
  return new tr::ExpAndTy(new tr::ExExp(ret), ret_type);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var = var_->Translate(venv, tenv, level, label, errormsg);
  auto exp = exp_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *ret = new tr::NxExp(new tree::MoveStm(var->exp_->UnEx(), exp->exp_->UnEx()));
  return new tr::ExpAndTy(ret, type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto test = test_->Translate(venv, tenv, level, label, errormsg);
  auto then = then_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *elsee;
  if (elsee_) elsee = elsee_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *ret;

  auto t = temp::LabelFactory::NewLabel();
  auto f = temp::LabelFactory::NewLabel();
  auto done = temp::LabelFactory::NewLabel();

  auto cx = test->exp_->UnCx(errormsg);
  cx.trues_.DoPatch(t);
  cx.falses_.DoPatch(f);

  if (!elsee_) {
    ret = new tr::NxExp(
      new tree::SeqStm(cx.stm_,
        new tree::SeqStm(new tree::LabelStm(t), 
          new tree::SeqStm(then->exp_->UnNx(), new tree::LabelStm(f))))
    );
    return new tr::ExpAndTy(ret, type::VoidTy::Instance());
  } else {
    auto r = temp::TempFactory::NewTemp();
    ret = new tr::ExExp(
      new tree::EseqExp(cx.stm_, 
        new tree::EseqExp(new tree::LabelStm(t), 
          new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), then->exp_->UnEx()), 
            new tree::EseqExp(new tree::JumpStm(new tree::NameExp(done), new std::vector<temp::Label *>({done})), 
              new tree::EseqExp(new tree::LabelStm(f), 
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), elsee->exp_->UnEx()), 
                  new tree::EseqExp(new tree::JumpStm(new tree::NameExp(done), new std::vector<temp::Label *>({done})), 
                    new tree::EseqExp(new tree::LabelStm(done), new tree::TempExp(r)))))))))
    );
    return new tr::ExpAndTy(ret, then->ty_->ActualTy());
  }
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto loop = temp::LabelFactory::NewLabel();
  auto body = temp::LabelFactory::NewLabel();
  auto done = temp::LabelFactory::NewLabel();

  auto test = test_->Translate(venv, tenv, level, label, errormsg);
  auto bd = body_->Translate(venv, tenv, level, done, errormsg);

  auto test_cx = test->exp_->UnCx(errormsg);
  test_cx.trues_.DoPatch(body);
  test_cx.falses_.DoPatch(done);

  auto ret = new tr::NxExp(
    new tree::SeqStm(new tree::LabelStm(loop), 
      new tree::SeqStm(test_cx.stm_, 
        new tree::SeqStm(new tree::LabelStm(body), 
          new tree::SeqStm(bd->exp_->UnNx(), 
            new tree::SeqStm(new tree::JumpStm(new tree::NameExp(loop), new std::vector<temp::Label *>({loop})), new tree::LabelStm(done))))))
  );
  return new tr::ExpAndTy(ret, type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto left = lo_->Translate(venv, tenv, level, label, errormsg);
  auto right = hi_->Translate(venv, tenv, level, label, errormsg);
  venv->BeginScope();
  auto access = tr::Access::AllocLocal(level, escape_);
  auto acc_limit = tr::Access::AllocLocal(level, false);
  venv->Enter(var_, new env::VarEntry(access, type::IntTy::Instance(), true));

  temp::Label *done = temp::LabelFactory::NewLabel();
  temp::Label *body = temp::LabelFactory::NewLabel();
  temp::Label *loop = temp::LabelFactory::NewLabel();
  auto bdy = body_->Translate(venv, tenv, level, done, errormsg);
  auto body_stm = bdy->exp_->UnNx();

  auto i = access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  auto limit = acc_limit->access_->ToExp(nullptr);
  tree::Stm *i_stm = new tree::MoveStm(i, left->exp_->UnEx());
  tree::Stm *lim_stm = new tree::MoveStm(limit, right->exp_->UnEx());
  auto label_list = new std::vector<temp::Label*>();
  label_list->emplace_back(body);

  tree::Stm *first_t = new tree::CjumpStm(tree::GE_OP, i, limit, done, body);
  tree::Stm *test = new tree::CjumpStm(tree::EQ_OP, i, limit, done, loop);
  
  auto ret = new tr::NxExp(
    new tree::SeqStm(i_stm,
      new tree::SeqStm(lim_stm,
        new tree::SeqStm(first_t,
          new tree::SeqStm(new tree::LabelStm(body),
            new tree::SeqStm(body_stm,
              new tree::SeqStm(test,
                new tree::SeqStm(new tree::LabelStm(loop),
                  new tree::SeqStm(new tree::MoveStm(i, new tree::BinopExp(tree::PLUS_OP, i, new tree::ConstExp(1))),
                    new tree::SeqStm(new tree::JumpStm(new tree::NameExp(body), new std::vector<temp::Label *>{body}),
                      new tree::LabelStm(done)))))))))
  )); 
  venv->EndScope();

  return new tr::ExpAndTy(ret, type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::NxExp(new tree::JumpStm(new tree::NameExp(label), new std::vector<temp::Label *>({label}))), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tenv->BeginScope();
  venv->BeginScope();
  if (!body_){
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }

  tree::Stm *init = nullptr;
  tree::Exp *ret;
  auto decs = decs_->GetList();
  for(auto dec : decs) {
    auto stm = dec->Translate(venv, tenv, level, label, errormsg);
    if (init == nullptr) {
      init = stm->UnNx();
    } else {
      init = new tree::SeqStm(init, stm->UnNx());
    }
  }

  auto body_info = body_->Translate(venv, tenv, level, label, errormsg);
  if (init) {
    ret = new tree::EseqExp(init, body_info->exp_->UnEx());
  } else {
    ret = body_info->exp_->UnEx();
  }

  venv->EndScope();
  tenv->EndScope();
  return new tr::ExpAndTy(new tr::ExExp(ret), body_info->ty_->ActualTy());
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto size = this->size_->Translate(venv, tenv, level, label, errormsg);
  auto init = this->init_->Translate(venv, tenv, level, label, errormsg);
  auto init_att = frame::ExternalCall("init_array", new tree::ExpList({size->exp_->UnEx(), init->exp_->UnEx()}));

  return new tr::ExpAndTy(new tr::ExExp(init_att), static_cast<type::ArrayTy *>(tenv->Look(typ_)));
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::NxExp(nullptr), type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto fun_list = this->functions_->GetList();

  for (auto &func : fun_list) {
    auto type_list = func->params_->MakeFormalTyList(tenv, errormsg);
    std::list<bool> escapes;
    auto field_list = func->params_->GetList();
    for (auto &field : field_list) {
      escapes.emplace_back(field->escape_);
    }
    type::Ty *res_ty = type::VoidTy::Instance();
    if (func->result_ != nullptr)
      res_ty = tenv->Look(func->result_);
    auto fun_name = func->name_;
    auto func_label = temp::LabelFactory::NamedLabel(fun_name->Name());
    venv->Enter(fun_name, new env::FunEntry(tr::Level::NewLevel(level, fun_name, escapes), func_label, type_list, res_ty));
  }

  for (auto &func : fun_list) {
    venv->BeginScope();
    auto fun_ent = static_cast<env::FunEntry *>(venv->Look(func->name_));
    auto field_list = func->params_->MakeFieldList(tenv, errormsg)->GetList();
    auto formals = func->params_->MakeFormalTyList(tenv, errormsg);
    auto formal_iter = fun_ent->level_->frame_->formals_.begin();
    formal_iter++;
    auto type_iter = field_list.begin();
    auto param_iter = func->params_->GetList().begin();
    for (; formal_iter != fun_ent->level_->frame_->formals_.end() && type_iter != field_list.end(); type_iter++, formal_iter++) {
      venv->Enter((*type_iter)->name_, new env::VarEntry(new tr::Access(fun_ent->level_, *formal_iter), (*type_iter)->ty_));
    }
    auto fun_body = func->body_->Translate(venv, tenv, fun_ent->level_, fun_ent->label_, errormsg);
    tree::Stm *ret;
    if (func->result_ != nullptr) {
      ret = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), fun_body->exp_->UnEx());
    } else {
      ret = fun_body->exp_->UnNx();
    }
    auto li_st = ProcEntryExit1(fun_ent->level_->frame_, ret);
    auto seq_stm = tr::list_seq(li_st);
    frags->PushBack(new frame::ProcFrag(seq_stm, fun_ent->level_->frame_));
    venv->EndScope();
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto init_ty = init_->Translate(venv, tenv, level, label, errormsg);
  auto access = tr::Access::AllocLocal(level, this->escape_); 
  type::Ty *var;
  if (this->typ_ != nullptr) {
    var = tenv->Look(typ_)->ActualTy();
  } else {
    var = init_ty->ty_->ActualTy();
  }

  venv->Enter(var_, new env::VarEntry(access, var));
  return new tr::NxExp(new tree::MoveStm(access->access_->ToExp(static_link(level, access->level_)), init_ty->exp_->UnEx()));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto ty_decs = this->types_->GetList();
  for (auto list : ty_decs) {
    tenv->Enter(list->name_, new type::NameTy(list->name_, nullptr));
  }

  for (auto list : ty_decs) {
    auto name_ty = static_cast<type::NameTy *>(tenv->Look(list->name_));
    name_ty->ty_ = list->ty_->Translate(tenv, errormsg);
    tenv->Set(list->name_, name_ty->ty_);
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::NameTy(this->name_, tenv->Look(name_));
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::RecordTy(record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::ArrayTy(tenv->Look(array_));
}

} // namespace absyn
