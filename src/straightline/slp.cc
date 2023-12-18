#include "straightline/slp.h"

#include <iostream>
#include "slp.h"

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return std::max(this->stm1->MaxArgs(), this->stm2->MaxArgs());
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  t = this->stm1->Interp(t);
  return this->stm2->Interp(t);
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return this->exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *IAT = this->exp->Interp(t);
  t = IAT->t;
  int value = IAT->i;
  Table *NewTable = new Table(this->id, value, t);
  return NewTable;
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return std::max(this->exps->NumExps(), this->exps->MaxArgs());
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  std::list<int> list;
  IntAndTable *IAT = this->exps->Interp(t, list);
  while(!list.empty()) {
    std::cout << list.front() << " ";
    list.pop_front();
  }
  std::cout << std::endl;
  return IAT->t;
}

int A::IdExp::MaxArgs() const {
  return 0;
}

IntAndTable *A::IdExp::Interp(Table *t) const {
  return new IntAndTable(t->Lookup(this->id), t);
}

int A::NumExp::MaxArgs() const {
  return 0;
}

IntAndTable *A::NumExp::Interp(Table *t) const {
  return new IntAndTable(this->num, t);
}

int A::OpExp::MaxArgs() const {
  return std::max(this->left->MaxArgs(), this->right->MaxArgs());
}

IntAndTable *A::OpExp::Interp(Table *t) const {
  IntAndTable *IATL = this->left->Interp(t);
  IntAndTable *IATR = this->right->Interp(IATL->t);
  int value = 0;
  switch (this->oper)
  {
    case 0:
      value = IATL->i + IATR->i;
      break;
    case 1:
      value = IATL->i - IATR->i;
      break;
    case 2:
      value = IATL->i * IATR->i;
      break;
    case 3:
      if(IATR->i == 0)
        break;
      else 
        value = IATL->i / IATR->i;
    default:
      break;
  }
  return new IntAndTable(value, IATR->t);
}

int A::EseqExp::MaxArgs() const {
  return std::max(this->stm->MaxArgs(), this->exp->MaxArgs());
}

IntAndTable *A::EseqExp::Interp(Table *t) const {
  Table *TSTM = this->stm->Interp(t);
  return this->exp->Interp(TSTM);
}

int A::PairExpList::MaxArgs() const {
  return std::max(this->exp->MaxArgs(), this->tail->MaxArgs());
}

int A::PairExpList::NumExps() const {
  return 1 + this->tail->NumExps();
}

IntAndTable *A::PairExpList::Interp(Table *t, std::list<int> &list) const {
  IntAndTable *THISIAT = this->exp->Interp(t);
  list.push_back(THISIAT->i);
  IntAndTable *RESIAT = this->tail->Interp(THISIAT->t, list);
  return RESIAT;
}

int A::LastExpList::MaxArgs() const {
  return this->exp->MaxArgs();
}

int A::LastExpList::NumExps() const {
  return 1;
}

IntAndTable *A::LastExpList::Interp(Table *t, std::list<int> &list) const {
  IntAndTable *IAT = this->exp->Interp(t);
  list.push_back(IAT->i);
  return IAT;
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
} // namespace A