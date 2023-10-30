#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  auto entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    return static_cast<env::VarEntry *>(entry)->ty_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return type::IntTy::Instance();
}


type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  auto var_type = var_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (var_type == nullptr) {
    errormsg->Error(pos_, "undefined variable");
    return var_type;
  } else if (typeid(*(var_type->ActualTy())) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return type::IntTy::Instance();
  }

  for (auto &field : static_cast<type::RecordTy *>(var_type)->fields_->GetList()) {
    if (field->name_->Name() == sym_->Name()) {
      return field->ty_->ActualTy();
    }
  }

  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
  return type::IntTy::Instance();
  
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto var_type = var_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (var_type == nullptr) {
    errormsg->Error(pos_, "undefined variable");
    return var_type;
  } else if (typeid(*(var_type->ActualTy())) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }

  auto sub_type = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (typeid(*(sub_type->ActualTy())) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "not int type in SubscriptVar");
    return type::IntTy::Instance();
  }

  return static_cast<type::ArrayTy *>(var_type)->ty_->ActualTy();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto callee = venv->Look(func_);
  if (callee == nullptr || typeid(*callee) != typeid(env::FunEntry)) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().c_str());
    return type::NilTy::Instance();
  }

  auto formals = static_cast<env::FunEntry *>(callee)->formals_;
  auto res = static_cast<env::FunEntry *>(callee)->result_;

  if (formals->GetList().size() > args_->GetList().size()) {
    errormsg->Error(pos_, "too few params in function %s", func_->Name().data());
  } else if (formals->GetList().size() < args_->GetList().size()) {
    errormsg->Error(pos_, "too many params in function %s", func_->Name().data());
  }

  auto arg = args_->GetList().begin();
  auto formal = formals->GetList().begin();

  for (; arg != args_->GetList().end() && formal != formals->GetList().end(); arg++, formal++) {
    auto arg_ty = (*arg)->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!arg_ty->IsSameType(* formal)) {
      errormsg->Error((*arg)->pos_, "para type mismatch");
    }
  }

  return res->ActualTy();
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  auto *le = left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  auto *re = right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (oper_ == absyn::PLUS_OP || oper_ == absyn::DIVIDE_OP || oper_ == absyn::TIMES_OP || oper_ == absyn::MINUS_OP) {
    if (typeid(*le) != typeid(type::IntTy)) {
      errormsg->Error(left_->pos_, "integer required");
    }
    if (typeid(*re) != typeid(type::IntTy)) {
      errormsg->Error(right_->pos_, "integer required");
    }
    return type::IntTy::Instance();
  }
  if(!le->IsSameType(re)) {
    errormsg->Error(pos_, "same type required");
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  auto ty = tenv->Look(typ_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::VoidTy::Instance();
  }
  auto act = ty->ActualTy();
  if (typeid(*act) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return type::VoidTy::Instance();
  }
  auto efields = fields_->GetList();
  auto fields = static_cast<type::RecordTy *>(ty)->fields_->GetList();

  if (efields.size() != fields.size()) {
    errormsg->Error(pos_, "undefined record type");
    return type::VoidTy::Instance();
  }

  auto efield = efields.begin();
  auto field = fields.begin();
  for (; efield != efields.end(); efield++, field++) {
    if ((*field)->name_->Name() != (*efield)->name_->Name()) {
      errormsg->Error(pos_, "field name unmatched");
    }
    if (!(*field)->ty_->IsSameType((*efield)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg))) {
      errormsg->Error(pos_, "field type unmatched");
    }
  }

  return ty;
}


type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *tp;
  if (seq_ == nullptr) {
    return type::VoidTy::Instance();
  }

  for (auto &exp : seq_->GetList()) {
    tp = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return tp;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  auto var_tp = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto exp_tp = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (!var_tp->IsSameType(exp_tp)) {
    errormsg->Error(pos_, "unmatched assign exp");
  }

  if (typeid(*var_) == typeid(absyn::SimpleVar)) {
    auto entry = venv->Look(static_cast<SimpleVar *>(var_)->sym_);
    auto var_entry = static_cast<env::VarEntry *>(entry);
    if (!entry) {
      errormsg->Error(var_->pos_, "undefined variable %s", static_cast<SimpleVar *>(var_)->sym_->Name().data());
    }
    if (var_entry->readonly_) {
      errormsg->Error(var_->pos_, "loop variable can't be assigned");
    }
  }

  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  auto test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!test_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(pos_, "test exp type should be integer");
    return type::VoidTy::Instance();
  }

  auto then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (elsee_ == nullptr) {
    if (!then_ty->IsSameType(type::VoidTy::Instance())) {
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
      return type::VoidTy::Instance();
    } else {
      return type::VoidTy::Instance();
    }
  } else {
    auto eles_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!then_ty->IsSameType(eles_ty)) {
      errormsg->Error(pos_, "then exp and else exp type mismatch");
      return type::VoidTy::Instance();
    }

    return then_ty;
  }

}


type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  auto test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!test_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(pos_, "test exp must be integer");
    return type::VoidTy::Instance();
  }
  if (!body_) {
    return type::VoidTy::Instance();
  }

  auto body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (!body_ty->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(body_->pos_, "while body must produce no value");
  }
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto lo_ty = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto hi_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (!lo_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  }
  if (!hi_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }

  venv->BeginScope();
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
  auto body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (!body_ty->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(body_->pos_, "for body must produce no value");
  }
  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (labelcount == 0) {
    errormsg->Error(pos_, "break is not inside any loop");
  }

  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  venv->BeginScope();
  tenv->BeginScope();
  for (auto dec : decs_->GetList()) {
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  type::Ty *res;

  if (!body_) {
    res = type::VoidTy::Instance();
  } else {
    res = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  }

  tenv->EndScope();
  venv->EndScope();
  return res;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  auto sb_ty = tenv->Look(typ_);
  if (!sb_ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::VoidTy::Instance();
  }
  if (typeid(*sb_ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required");
    return type::VoidTy::Instance();
  }

  auto size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!size_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(pos_, "size type should be integer");
    return type::VoidTy::Instance();
  }

  if (!init_ty->IsSameType(static_cast<type::ArrayTy *>(sb_ty)->ty_)) {
    errormsg->Error(pos_, "array type mismatch");
    return type::VoidTy::Instance();
  }

  return sb_ty;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto fun_list = functions_->GetList();

  for (auto &func : fun_list) {
    type::Ty *res;
    auto args = func->params_;
    if (func->result_) {
      res = tenv->Look(func->result_);
    } else {
      res = type::VoidTy::Instance();
    }
    if (venv->Look(func->name_)) {
      errormsg->Error(pos_, "two functions have the same name");
      return;
    }

    auto formals = args->MakeFormalTyList(tenv, errormsg);
    venv->Enter(func->name_, new env::FunEntry(formals, res));
  }

  for (auto &func : fun_list) {
    auto params = func->params_;
    auto formals = params->MakeFormalTyList(tenv, errormsg);
    venv->BeginScope();
    auto formal = formals->GetList().begin();
    auto param = params->GetList().begin();
    for (; formal != formals->GetList().end(); formal++, param++) {
      venv->Enter((*param)->name_, new env::VarEntry((*formal)));
    }
    
    type::Ty *resu = type::VoidTy::Instance();
    if (func->result_) {
      resu = tenv->Look(func->result_);
    }
    auto body_ty = func->body_->SemAnalyze(venv, tenv, labelcount, errormsg);

    if (!body_ty->IsSameType(resu)) {
      if (resu->IsSameType(type::VoidTy::Instance())) {
        errormsg->Error(pos_, "procedure returns value");
      } else {
        errormsg->Error(pos_, "return value mismatch");
      }
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  auto init_type = init_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (typ_ == nullptr) {
    if (typeid(*init_type) == typeid(type::NilTy)) {
      errormsg->Error(pos_, "init should not be nil without type specified");
    }
  } else {
    auto ty = tenv->Look(typ_);
    if (!ty->IsSameType(init_type)) {
      errormsg->Error(pos_, "type mismatch");
    }
  }

  venv->Enter(var_, new env::VarEntry(init_type));
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto tp_list = types_->GetList();
  for (auto list : tp_list) {
    if (tenv->Look(list->name_) != nullptr) {
      errormsg->Error(pos_, "two types have the same name");
    }
    tenv->Enter(list->name_, new type::NameTy(list->name_, nullptr));
  }

  for (auto list : tp_list) {
    auto name_ty = static_cast<type::NameTy *>(tenv->Look(list->name_));
    name_ty->ty_ = list->ty_->SemAnalyze(tenv, errormsg);
    tenv->Set(list->name_, name_ty->ty_);
  }

  for (auto list : tp_list) {
    auto cur = tenv->Look(list->name_);
    while (typeid(*cur) == typeid(type::NameTy)) {
      if (static_cast<type::NameTy *>(cur)->sym_ == list->name_) {
        errormsg->Error(pos_, "illegal type cycle");
        return;
      }
      cur = static_cast<type::NameTy *>(cur)->ty_;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  auto tp = tenv->Look(name_);
  if (tp != nullptr) {
    return new type::NameTy(name_, tp);
  }

  errormsg->Error(pos_, "undefined type %s", name_->Name().data());
  return type::VoidTy::Instance();
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  auto filed_list = record_->MakeFieldList(tenv, errormsg);
  return new type::RecordTy(filed_list);
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  auto tp = tenv->Look(array_);
  if (tp != nullptr) {
    return new type::ArrayTy(tp);
  }

  errormsg->Error(pos_, "undefined type %s", array_->Name().data());
  return new type::ArrayTy(type::IntTy::Instance());
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}
} // namespace sem
