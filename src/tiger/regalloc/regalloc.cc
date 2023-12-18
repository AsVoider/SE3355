#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
void RegAllocator::RegAlloc() {
    bool ed = false;
    while (true) {
        fg::FlowGraphFactory flow_fac(instr_list_->GetInstrList());
        flow_fac.AssemFlowGraph();
        fg::FGraphPtr fptr = flow_fac.GetFlowGraph();

        live::LiveGraphFactory live_fac(fptr);
        live_fac.Liveness();
        auto live_graph = live_fac.GetLiveGraph();

        col::Color coloring(live_graph);
        coloring.Exe();
        auto res = coloring.res;
        Colors = res.coloring;
        spills = res.spills;

        if (!spills || spills->GetList().empty())
            break;
        new_temps.clear();
        auto newIns = ReWrite(new_temps);
        instr_list_ = new cg::AssemInstr(newIns);
    }

    auto instrs = instr_list_->GetInstrList()->GetList();
    auto rsp = reg_manager->StackPointer();
    auto sim = new assem::InstrList();

    for (auto &ins : instrs) {
        if (typeid(*ins) == typeid(assem::MoveInstr) && !ins->Def()->GetList().empty() && !ins->Use()->GetList().empty()) {
            auto dst = ins->Def()->GetList().front();
            auto src = ins->Use()->GetList().front();
            if (dst == rsp || src == rsp)
                continue;
            if (Colors->Look(dst) == Colors->Look(src))
                continue;
        }
        sim->Append(ins);
    }
    instr_list_ = new cg::AssemInstr(sim);
}

assem::InstrList *RegAllocator::ReWrite(std::list<temp::Temp *> &news) {
    auto new_inses = new assem::InstrList();
    auto spill_list = spills->GetList();
    auto tmp_inses = std::list<assem::Instr *>();
    auto prev_inses = instr_list_->GetInstrList()->GetList();
    auto rsp = reg_manager->StackPointer();
    auto wordsz = reg_manager->WordSize();
    std::string assem_lang;
    temp::TempList *src, *dst;
    bool ischanged = false;

    for (auto &spilled : spill_list) {
        auto spill_temp = spilled->NodeInfo();
        frame_->offset_ -= wordsz;

        for (auto &ins : prev_inses) {
            dst = ins->Def();
            src = ins->Use();
            ischanged = false;
            if (src && src->Contain(spill_temp)) {
                assem_lang = "movq (" + frame_->labels_->Name() + "_framesize" + std::to_string(frame_->offset_) + ")(`s0), `d0";
                auto tmp = temp::TempFactory::NewTemp();
                auto new_ins = new assem::OperInstr(
                    assem_lang, new temp::TempList{tmp}, new temp::TempList{rsp}, nullptr
                );
                tmp_inses.emplace_back(new_ins);
                news.emplace_back(tmp);
                src->Replace(spill_temp, tmp);
            }

            if (dst && dst->Contain(spill_temp)) {
                ischanged = true;
                auto tmp = temp::TempFactory::NewTemp();
                assem_lang = "movq `s0, (" + frame_->labels_->Name() + "_framesize" + std::to_string(frame_->offset_) + ")(`d0)";
                auto new_ins = new assem::OperInstr(
                    assem_lang, new temp::TempList{rsp}, new temp::TempList{tmp}, nullptr
                );
                dst->Replace(spill_temp, tmp);
                tmp_inses.emplace_back(ins);
                tmp_inses.emplace_back(new_ins);
                new_temps.emplace_back(tmp);
            }
            if (!ischanged)
                tmp_inses.emplace_back(ins);
        }
        prev_inses = tmp_inses;
        tmp_inses.clear();
    }
    new_inses->InsertAll(prev_inses);
    return new_inses;
}

std::unique_ptr<ra::Result> RegAllocator::TransferResult() {
    auto resu = std::make_unique<ra::Result>();
    resu->coloring_ = Colors;
    resu->il_ = instr_list_->GetInstrList();
    return std::move(resu);
}
} // namespace ra