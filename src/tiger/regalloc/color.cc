#include "tiger/regalloc/color.h"

extern frame::RegManager *reg_manager;

namespace col {
/* TODO: Put your lab6 code here */
Color::Color(live::LiveGraph live_graph) : livegraph(live_graph) {
    auto regs = reg_manager->ExceptRsp()->GetList();
    for (auto &reg : regs) {
        precolored.insert(reg);
    }
    simplify_work = new live::INodeList();
    freeze_work = new live::INodeList();
    spill_node = new live::INodeList();
    spill_work = new live::INodeList();
    select_stack = new live::INodeList();
    coalesced_node = new live::INodeList();
    colored_node = new live::INodeList();
    ini = livegraph.interf_graph->Nodes();

    work_moves = new live::MoveList();
    active_move = new live::MoveList();
    colaesce_move = new live::MoveList();
    constrained_move = new live::MoveList();
    frozen_move = new live::MoveList();
}

Color::~Color() {
    delete simplify_work;
    delete freeze_work;
    delete spill_node;
    delete spill_work;
    delete select_stack;
    delete coalesced_node;
    delete colored_node;
    delete work_moves;
    delete active_move;
    delete colaesce_move;
    delete constrained_move;
    delete frozen_move;
}

void Color::Build() {
    auto node_list = livegraph.interf_graph->Nodes()->GetList();
    auto live_move = livegraph.moves->GetList();
    work_moves = livegraph.moves;
    for (auto &mv : live_move) {
        auto src = mv.first, dst = mv.second;
        if (!move_list.count(src))
            move_list[src] = new live::MoveList();
        if (!move_list.count(dst))
            move_list[dst] = new live::MoveList();
        move_list[src]->Append(src, dst);
        move_list[dst]->Append(src, dst);
    }

    for (auto &node : node_list) {
        degrees[node] = 0;
        if (!precolored.count(node->NodeInfo())) 
            adj_list[node] = new live::INodeList();
    }

    for (auto &node : node_list) {
        for (auto &adj_node : node->Adj()->GetList())
            AddEdge(node, adj_node);
    }

    auto idx = 0;
    for (auto &tmp : reg_manager->ExceptRsp()->GetList()) {
        if (idx == 7)
            idx++;
        color[tmp] = idx;
        idx++;
    }
}

void Color::AddEdge(live::INodePtr s, live::INodePtr d) {
    auto paired = std::pair<live::INodePtr, live::INodePtr>(s, d);
    auto reverse = std::pair<live::INodePtr, live::INodePtr>(d, s);
    if (s != d && !adj_set.count(paired)) {
        adj_set.insert(paired);
        adj_set.insert(reverse);
        if (!precolored.count(s->NodeInfo())) {
            adj_list[s]->Append(d);
            degrees[s] += 1;
        }
        if (!precolored.count(d->NodeInfo())) {
            adj_list[d]->Append(s);
            degrees[d] += 1;
        }
    }
}

void Color::MakeWorkList() {
    for (auto &node : ini->GetList()) {
        if (precolored.count(node->NodeInfo()))
            continue;
        if (degrees[node] >= 15)
            spill_work->Append(node);
        else if (MoveRelated(node)) {
            freeze_work->Append(node);
        } else {
            simplify_work->Append(node);
        }
        
    }
}

void Color::Simplify() {
    auto node = simplify_work->GetList().front();
    select_stack->Prepend(node);
    auto adj_list = Adjacent(node)->GetList();
    for (auto &adj : adj_list)
        Decrement(adj);
    simplify_work->DeleteNode(node);
}

void Color::Coalesce() {
    auto mv = work_moves->GetList().front();
    auto paired = std::pair<live::INodePtr, live::INodePtr>(mv);
    auto x = GetAlias(mv.first), y = GetAlias(mv.second);
    if (precolored.count(y->NodeInfo())) {
        paired.first = y;
        paired.second = x;
    } else {
        paired.first = x;
        paired.second = y;
    }

    work_moves->Delete(mv.first, mv.second);

    bool geo = true;
    if (precolored.count(paired.first->NodeInfo()) && !precolored.count(paired.second->NodeInfo())) {
        auto adjs = Adjacent(paired.second)->GetList();
        geo = AllOK(adjs, paired.first);
    }

    bool briggs = false;
    if (!precolored.count(paired.second->NodeInfo()) && !precolored.count(paired.first->NodeInfo())) {
        briggs = Conservertive(Adjacent(paired.first)->Union(Adjacent(paired.second)));
    }

    if (x == y) {
        colaesce_move->Append(mv.first, mv.second);
        AddWorkList(paired.first);
    } else if (precolored.count(paired.second->NodeInfo()) || adj_set.count(paired)) {
        constrained_move->Append(mv.first, mv.second);
        AddWorkList(paired.first);
        AddWorkList(paired.second);
    } else if ((precolored.count(paired.first->NodeInfo()) && geo) || (!precolored.count(paired.first->NodeInfo()) && briggs)) {
        colaesce_move->Append(mv.first, mv.second);
        Combine(paired.first, paired.second);
        AddWorkList(paired.first);
    } else {
        active_move->Append(mv.first, mv.second);
    }
}

void Color::Freeze() {
    auto fz  = freeze_work->GetList().front();
    freeze_work->DeleteNode(fz);
    simplify_work->Append(fz);
    FreezeMove(fz);
}

void Color::SelectSpill() {
    int max_degree = 0;
    live::INodePtr ptr = nullptr;
    for (auto &node : spill_work->GetList()) {
        if (degrees[node] > max_degree) {
            max_degree = degrees[node];
            ptr = node;
        }
    }
    if (ptr == nullptr)
        ptr = spill_work->GetList().front();
    
    spill_work->DeleteNode(ptr);
    simplify_work->Append(ptr);
    FreezeMove(ptr);
}

void Color::Assign() {
    std::set<int> colors;
    auto callersave = reg_manager->CallerSaves();
    while (!select_stack->GetList().empty()) {
        auto n = select_stack->GetList().front();
        select_stack->DeleteNode(n);
        Init(colors);
        bool is_self = false;
        for (auto &adj : adj_list[n]->GetList()) {
            auto alias = GetAlias(adj);
            if (colored_node->Contain(alias) || precolored.count(alias->NodeInfo())) {
                auto alias_co= color[alias->NodeInfo()];
                colors.erase(alias_co);
            }
        }
        if (colors.empty()) {
            spill_node->Append(n);
        } else {
            colored_node->Append(n);
            auto sz = colors.size();
            auto frt = *colors.begin();
            bool find  = false;
            temp::Temp *acc;
            for (auto &iter : colors) {
                acc = reg_manager->GetRegister(iter);
                bool contain = false;
                for (auto &it : callersave->GetList()) {
                    if (acc == it)
                        contain = true;
                }
                if (contain) {
                    frt = iter;
                    break;
                }
            }
            color[n->NodeInfo()] = frt;
        }
    }
    for (auto &node : coalesced_node->GetList()) {
        auto alias = GetAlias(node);
        color[node->NodeInfo()] = color[alias->NodeInfo()];
    }
}

void Color::Exe() {
    Build();
    MakeWorkList();
    do {
            if(!simplify_work->GetList().empty()) {
                Simplify();
            } else if(!work_moves->GetList().empty()) {
                Coalesce();
            } else if(!freeze_work->GetList().empty()) {
                Freeze();
            } else if(!spill_work->GetList().empty()) {
                SelectSpill();
            }
        } 
    while(!simplify_work->GetList().empty() || !work_moves->GetList().empty()
        || !freeze_work->GetList().empty() || !spill_work->GetList().empty());
    Assign();
    res.coloring = temp::Map::Empty();
    for (auto &co : color) {
        res.coloring->Enter(co.first, reg_manager->temp_map_->Look(reg_manager->GetRegister(co.second)));
    }
    res.spills = new live::INodeList();
    res.spills->CatList(spill_node);
}

bool Color::MoveRelated(live::INodePtr  ptr) {
    return !NodeMoves(ptr)->GetList().empty();
}

bool Color::OK(live::INodePtr s, live::INodePtr d) {
    auto paired = std::pair<live::INodePtr, live::INodePtr>(s, d);
    return degrees[s] < 15 || precolored.count(s->NodeInfo()) || adj_set.count(paired);
}

bool Color::AllOK(std::list<live::INodePtr> &list, live::INodePtr d) {
    for (auto &l_iter : list) {
        if (!OK(l_iter, d))
            return false;
    }
    return true;
}

bool Color::Conservertive(live::INodeListPtr ptr) {
    auto k = 0;
    for (auto &l_iter : ptr->GetList()) {
        if (degrees[l_iter] >= 15 || precolored.count(l_iter->NodeInfo())) {
            k += 1;
        }
    }
    return k < 15;
}

void Color::Decrement(live::INodePtr ptr) {
    if (precolored.count(ptr->NodeInfo())) {
        return;
    }

    int d = degrees[ptr];
    degrees[ptr] = d - 1;
    if (d == 15) {
        auto adj_nodes = Adjacent(ptr);
        adj_nodes->Append(ptr);
        EnableMoves(adj_nodes);
        spill_work->DeleteNode(ptr);
        if (MoveRelated(ptr))
            freeze_work->Append(ptr);
        else 
            simplify_work->Append(ptr);
    }
}

void Color::EnableMoves(live::INodeListPtr ptr) {
    for (auto &l_iter : ptr->GetList()) {
        auto mv = NodeMoves(l_iter)->GetList();
        for (auto &m_iter : mv) {
            if (active_move->Contain(m_iter.first, m_iter.second)) {
                active_move->Delete(m_iter.first, m_iter.second);
                work_moves->Append(m_iter.first, m_iter.second);
            }
        }
    }
}

void Color::AddWorkList(live::INodePtr ptr) {
    if (!precolored.count(ptr->NodeInfo()) && !MoveRelated(ptr) && degrees[ptr] < 15) {
        freeze_work->DeleteNode(ptr);
        simplify_work->Append(ptr);
    }
}

void Color::Combine(live::INodePtr s, live::INodePtr d) {
    if (freeze_work->Contain(d))
        freeze_work->DeleteNode(d);
    else
        spill_work->DeleteNode(d);

    coalesced_node->Append(d);
    alias[d] = s;
    move_list[s] = move_list[s]->Union(move_list[d]);
    auto list1 = new live::INodeList();
    list1->Append(d);
    EnableMoves(list1);
    delete list1;

    auto adjlist = Adjacent(d)->GetList();
    for (auto &adj : adjlist) {
        AddEdge(adj, s);
        Decrement(adj);
    }
}

void Color::FreezeMove(live::INodePtr ptr) {
    auto mvs = NodeMoves(ptr)->GetList();
    for (auto &mv : mvs) {
        auto x = mv.first, y = mv.second;
        auto v = live::INodePtr();
        if (GetAlias(y) == GetAlias(ptr))
            v = GetAlias(x);
        else 
            v = GetAlias(y);
        
        active_move->Delete(x, y);
        frozen_move->Append(x, y);

        if (!precolored.count(v->NodeInfo()) && NodeMoves(v)->GetList().empty() && degrees[v] < 15) {
            freeze_work->DeleteNode(v);
            simplify_work->Append(v);
        }
    }
}

void Color::Init(std::set<int> &colors) {
    for (auto i = 0; i <= 15; i++) {
        if (i == 7)
            continue;
        colors.insert(i);
    }
}

live::INodePtr Color::GetAlias(live::INodePtr ptr) {
    if (coalesced_node->Contain(ptr)) {
        return GetAlias(alias[ptr]);
    } else 
        return ptr;
}

live::MoveList *Color::NodeMoves(live::INodePtr ptr) {
    if (!move_list.count(ptr)) {
        return new live::MoveList();
    }
    auto move_nth = move_list[ptr];
    return move_nth->Intersect(active_move->Union(work_moves));
}

live::INodeListPtr Color::Adjacent(live::INodePtr ptr) {
    auto adj_n = adj_list[ptr];
    return adj_n->Diff(select_stack->Union(coalesced_node));
}
} // namespace col
