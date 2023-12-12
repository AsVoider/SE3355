#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  return this->root_->Traverse(env, 0);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto res = env->Look(this->sym_);
  if (res == nullptr) {
    return;
  }
  if (res->depth_ < depth) {
    *(res->escape_) = true;
  }
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
  this->subscript_->Traverse(env, depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto arg = this->args_->GetList();
  for (auto &ag :arg) {
    ag->Traverse(env, depth);
  }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->left_->Traverse(env, depth);
  this->right_->Traverse(env, depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto rec_list = this->fields_->GetList();
  for (auto &rec : rec_list) {
    rec->exp_->Traverse(env, depth);
  }
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto seqs = this->seq_->GetList();
  for (auto &seq : seqs) {
    seq->Traverse(env, depth);
  }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
  this->exp_->Traverse(env, depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env, depth);
  this->then_->Traverse(env, depth);
  if (this->elsee_ != nullptr) {
    this->elsee_->Traverse(env, depth);
  }
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env, depth);
  this->body_->Traverse(env, depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  env->BeginScope();
  this->escape_ =false;
  env->Enter(this->var_, new esc::EscapeEntry(depth, &this->escape_));
  this->lo_->Traverse(env, depth);
  this->hi_->Traverse(env, depth);
  this->body_->Traverse(env, depth);
  env->EndScope();
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  env->BeginScope();
  auto dec_list = this->decs_->GetList();
  for (auto &dec : dec_list) {
    dec->Traverse(env, depth);
  }
  if (body_ != nullptr) {
    this->body_->Traverse(env, depth);
  }
  env->EndScope();
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->size_->Traverse(env, depth);
  this->init_->Traverse(env, depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto fun_list = this->functions_->GetList();
  for (auto &fun : fun_list) {
    env->BeginScope();
    auto f_list = fun->params_->GetList();
    for (auto &f : f_list) {
      f->escape_ = false;
      env->Enter(f->name_, new esc::EscapeEntry(depth + 1, &f->escape_));
    }
    fun->body_->Traverse(env, depth + 1);
    env->EndScope();
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->escape_ = false;
  if (this->init_ != nullptr) {
    this->init_->Traverse(env, depth);
  }
  env->Enter(this->var_, new esc::EscapeEntry(depth, &this->escape_));
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

} // namespace absyn
