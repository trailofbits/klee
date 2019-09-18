/*
 * Copyright (c) 2019 Trail of Bits, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "glog/logging.h"
#include "Native/Arch/TraceManager.h"
#include "Native/Memory/AddressSpace.h"
#include "Native/Util/AreaAllocator.h"
#include "Native/Memory/PolicyHandler.h"
#include "Native/Workspace/Workspace.h"
#include "Native/Memory/MappedRange.h"

#include "ThreadPool/ThreadPool.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

#include "remill/Arch/Arch.h"
#include "remill/Arch/Instruction.h"
#include "remill/BC/IntrinsicTable.h"
#include "remill/BC/Lifter.h"
#include "remill/OS/FileSystem.h"

#include "Native/Memory/AddressSpace.h"
#include "Native/Util/AreaAllocator.h"

#include "Core/PreLifter.h"
#include <fstream>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace klee {
namespace native {

PreLifter::PreLifter(llvm::LLVMContext *context) :
    ctx(context), trace_manager(nullptr) {
}

void PreLifter::RecursiveDescentPass(const klee::native::MemoryMapPtr &map,
    std::vector<std::pair<uint64_t, bool>> &decoder_work_list,
    std::unordered_map<uint64_t, llvm::Function *> &new_lifted_traces) {
  /*
   // first pass of marking traces ahead of time
   auto arch = remill::GetTargetArch();
   const uint64_t addr_mask = trace_manager->memory->addr_mask;
   std::string inst_bytes;
   remill::Instruction inst;
   uint64_t function_start;
   std::deque<uint64_t> q;
   std::unordered_set<uint64_t> visited;

   if (saved_entry_point_address > map->LimitAddress()) {
   // don't want to lift traces before the entry point
   return;
   } else if (map->Contains(saved_entry_point_address)) {
   function_start = saved_entry_point_address;
   decoder_work_list.push_back( { function_start, false });
   (void) new_lifted_traces[function_start];
   //LOG(INFO) << "marking 0x" << std::hex << function_start << std::dec;
   } else {
   function_start = map->BaseAddress();
   // perform the bit of code that finds the entry point or first valid instruction
   }

   q.push_back(function_start);
   while (!q.empty()) {
   auto potential_trace = q.front();
   LOG(INFO) << std::hex << potential_trace << std::dec;
   q.pop_front();
   inst_bytes.clear();
   for (size_t i = 0; i < arch->MaxInstructionSize(); ++i) {
   auto byte_addr = (potential_trace & addr_mask) + i;
   uint8_t byte;
   if (!trace_manager->TryReadExecutableByte(byte_addr, &byte)) {
   LOG(ERROR) << "Failed to read byte during recursive descent at 0x"
   << std::hex << byte_addr << std::dec;
   return;
   } else {
   inst_bytes.push_back(static_cast<char>(byte));
   }
   }
   inst.Reset();
   if (arch->DecodeInstruction(potential_trace, inst_bytes, inst)) {
   visited.insert(potential_trace);
   //LOG(INFO) << "instruction successfully decoded and visited: " << visited.size();
   switch (inst.category) {
   case (remill::Instruction::kCategoryDirectFunctionCall): {
   (void) new_lifted_traces[inst.branch_taken_pc];
   //LOG(INFO) << "on call marking 0x" << std::hex << inst.branch_taken_pc << std::dec;
   //LOG(INFO) << "on call next pc is 0x" << std::hex << inst.next_pc << std::dec;
   if (!visited.count(inst.branch_taken_pc)) {
   decoder_work_list.push_back( { inst.branch_taken_pc, false });
   q.push_back(inst.branch_taken_pc);
   q.push_back(inst.next_pc);
   }
   break;
   }

   case (remill::Instruction::kCategoryConditionalBranch): {
   if (!visited.count(inst.branch_taken_pc)) {
   q.push_back(inst.branch_taken_pc);
   decoder_work_list.push_back( { inst.branch_taken_pc, false });
   //LOG(INFO) << "taken branch marking 0x" << std::hex << inst.branch_taken_pc << std::dec;
   (void) new_lifted_traces[inst.branch_taken_pc];
   }

   if (!visited.count(inst.branch_not_taken_pc)) {
   q.push_back(inst.branch_not_taken_pc);
   decoder_work_list.push_back( { inst.branch_not_taken_pc, false });
   //LOG(INFO) << "not taken branch marking 0x" << std::hex << inst.branch_not_taken_pc << std::dec;
   (void) new_lifted_traces[inst.branch_not_taken_pc];
   }
   break;
   }
   case (remill::Instruction::kCategoryDirectJump): {
   if (!visited.count(inst.branch_taken_pc)) {
   q.push_back(inst.branch_taken_pc);
   decoder_work_list.push_back( { inst.branch_taken_pc, false });
   //LOG(INFO) << "on jmp marking 0x" << std::hex << inst.branch_taken_pc << std::dec;
   (void) new_lifted_traces[inst.branch_taken_pc];
   }
   break;
   }
   case (remill::Instruction::kCategoryIndirectJump):
   case (remill::Instruction::kCategoryIndirectFunctionCall): {
   if (!visited.count(inst.pc)) {
   // linear sweep pass is meant to flesh out these targets
   decoder_work_list.push_back( { inst.pc, false });
   // stops doing "work" on a queued potential trace
   }
   if (!visited.count(inst.next_pc)) {
   q.push_back(inst.next_pc);
   }
   break;
   }
   case (remill::Instruction::kCategoryFunctionReturn): {
   break;
   }
   default: {
   if (!visited.count(inst.next_pc)) {
   q.push_back(inst.next_pc);
   }
   break;
   }
   }
   }
   }
   */
}

void PreLifter::LinearSweepPass(const native::MemoryMapPtr &map,
    std::vector<std::pair<uint64_t, bool>> &decoder_work_list,
    std::unordered_map<uint64_t, llvm::Function *> &new_lifted_traces) {
  /*
   // second pass in trace marking
   auto arch = remill::GetTargetArch();
   const uint64_t addr_mask = trace_manager->memory->addr_mask;
   std::string inst_bytes;
   remill::Instruction inst;

   bool work_on_trace;

   while (!decoder_work_list.empty()) {
   LOG(INFO) << "work list size is " << decoder_work_list.size();
   auto trace_pair = decoder_work_list.back();
   decoder_work_list.pop_back();
   auto addr = trace_pair.first;
   auto targeted = trace_pair.second;
   work_on_trace = true;

   if (targeted) {
   uint8_t byte;
   auto byte_addr = addr & addr_mask;
   while (trace_manager->TryReadExecutableByte(byte_addr++, &byte)) {
   if (!byte) {
   ++addr;
   } else {
   break;
   }
   }
   }

   for (auto inst_addr = addr;
   inst_addr < map->LimitAddress() && work_on_trace; ++inst_addr) {

   inst_bytes.clear();
   std::unordered_set<uint64_t> visited;
   for (size_t i = 0; i < arch->MaxInstructionSize(); ++i) {
   const auto byte_addr = (inst_addr + i) & addr_mask;
   uint8_t byte;
   if (!trace_manager->TryReadExecutableByte(byte_addr, &byte)) {
   LOG(INFO) << " failed to read executable while decoding bytes on "
   << inst_addr;
   } else {
   inst_bytes.push_back(static_cast<char>(byte));
   }
   }
   inst.Reset();

   if (arch->DecodeInstruction(inst_addr, inst_bytes, inst)) {

   if (visited.count(inst_addr)) {
   inst_addr = inst.next_pc;
   continue;
   } else {
   visited.insert(inst_addr);
   }

   switch (inst.category) {
   case remill::Instruction::kCategoryDirectJump: {
   work_on_trace = false;
   (void) new_lifted_traces[addr];
   LOG(INFO) << "marking 0x" << std::hex << addr << std::dec;
   if (!visited.count(inst.branch_taken_pc)) {
   decoder_work_list.push_back( { inst.branch_taken_pc, false });
   LOG(INFO) << "direct jmp instruction at 0x" << std::hex << inst_addr
   << std::dec;
   }
   break;
   }
   case remill::Instruction::kCategoryConditionalBranch: {
   work_on_trace = false;
   (void) new_lifted_traces[addr];
   LOG(INFO) << "marking 0x" << std::hex << addr << std::dec;

   if (!visited.count(inst.branch_taken_pc)) {
   decoder_work_list.push_back( { inst.branch_taken_pc, false });
   }

   if (!visited.count(inst.branch_not_taken_pc)) {
   decoder_work_list.push_back( { inst.branch_not_taken_pc, false });
   }

   LOG(INFO) << "conditional branch instruction at 0x" << std::hex
   << inst_addr << std::dec << "- branch taken: " << std::hex
   << inst.branch_taken_pc << std::dec << "- branch not taken: "
   << std::hex << inst.branch_not_taken_pc << std::dec;
   break;
   }
   case remill::Instruction::kCategoryDirectFunctionCall:
   case remill::Instruction::kCategoryIndirectFunctionCall: {
   LOG(INFO) << "function call instruction at 0x" << std::hex
   << inst_addr << std::dec;
   work_on_trace = false;
   (void) new_lifted_traces[addr];
   LOG(INFO) << "marking 0x" << std::hex << addr << std::dec;
   LOG(INFO) << "the location of the call is at " << std::hex
   << inst.branch_taken_pc << std::dec;
   if (!visited.count(inst.next_pc)) {
   LOG(INFO) << "marking 0x" << std::hex << inst.branch_taken_pc
   << std::dec;
   (void) new_lifted_traces[inst.branch_taken_pc];
   decoder_work_list.push_back( { inst.next_pc, false });
   }
   break;
   }
   case remill::Instruction::kCategoryFunctionReturn: {
   LOG(INFO) << "ret instruction at 0x" << std::hex << inst_addr
   << std::dec;
   work_on_trace = false;
   (void) new_lifted_traces[addr];
   if (!visited.count(inst.next_pc)) {
   decoder_work_list.push_back( { inst.next_pc, true });
   }
   break;
   }
   case remill::Instruction::kCategoryNoOp: {
   LOG(INFO) << "noop instruction at 0x" << std::hex << inst_addr
   << std::dec;
   if (!targeted) {
   work_on_trace = false;
   }
   break;
   }
   default: {
   LOG(INFO) << "other instruction at 0x" << std::hex << inst_addr
   << std::dec << " of size " << inst.NumBytes();
   if (!visited.count(inst.next_pc)) {
   inst_addr = inst.next_pc - 1;
   } else {
   work_on_trace = false;
   }
   break;
   }
   }
   } else {
   work_on_trace = false;
   LOG(INFO) << "failed to decode instruction at 0x" << std::hex
   << inst_addr << std::dec << " throwing away trace " << std::hex
   << addr << std::dec;
   //if (!visited.count(inst_addr + 1)) {
   //  decoder_work_list.push_back( { inst_addr + 1, false });
   //}
   }
   }

   if (addr >= map->LimitAddress()) {
   break;
   }
   }
   */

}

/*
 void PreLifter::decodeAndMarkTraces(const native::MemoryMapPtr &map,
 std::unordered_map<uint64_t, llvm::Function *> &new_marked_traces) {
 std::vector<std::pair<uint64_t, bool>> decoder_work_list;
 exit(0);
 RecursiveDescentPass(map, decoder_work_list, new_marked_traces);
 LOG(INFO) << "Traces from recursive descent";
 for (const auto &trace : new_marked_traces) {
 LOG(INFO) << std::hex << trace.first << std::dec;
 }
 LinearSweepPass(map, decoder_work_list, new_marked_traces);
 }
 */

void PreLifter::LiftMapping(klee::native::Worker *worker, klee::native::PreLifter *pre_lifter) {
  //  traces are expected to be in sorted order and have at least size
  auto &traces = worker->traces;
  auto start = traces.front();
  auto end = traces.back();
  auto *trace_manager = pre_lifter->trace_manager;
  LOG(INFO) << "starting a lift job on range " << std::hex << traces.front()
      << std::dec << "- " << std::hex << traces.back() << std::dec;
  auto memory = trace_manager->memory;
  LOG(INFO) << "passed semantics module creation";
  auto mapping_manager = native::TraceManager(*worker->map_semantics, nullptr);
  mapping_manager.memory = memory;
  auto mapping_intrinsics = remill::IntrinsicTable(worker->map_semantics.get());
  auto mapping_inst_lifter = remill::InstructionLifter(remill::GetTargetArch(),
      mapping_intrinsics);
  auto mapping_lifter = remill::TraceLifter(&mapping_inst_lifter,
      &mapping_manager);
  auto &new_marked_traces = mapping_manager.traces;
  LOG(INFO) << "made thread specific lifter";

  while (!traces.empty()) {
    uint64_t trace = traces.back();
    (void) mapping_lifter.Lift(trace,
        [&new_marked_traces] (uint64_t trace_addr, llvm::Function *func) {
          func->setLinkage(llvm::GlobalValue::ExternalLinkage);
          new_marked_traces[trace_addr] = func;
        });
    traces.pop_back();
  }
  LOG(INFO) << "completed lifting and starting optimization";
  //mapping_manager.memory = nullptr;
  remill::OptimizationGuide guide = { };
  guide.slp_vectorize = false;
  guide.loop_vectorize = false;
  guide.verify_input = false;
  guide.eliminate_dead_stores = true;

  LOG(INFO) << "successfully lifted traces for mapping";
  remill::OptimizeModule(worker->map_semantics.get(), new_marked_traces, guide);
  LOG(INFO) << "finished optimizing the map module and moving to aot_traces";
  std::stringstream ss;
  auto &ws = klee::native::Workspace::Dir();
  ss << ws << "/prelift_traces/0x" << std::hex << start << std::dec << "-0x" << std::hex << end
      << std::dec;

  LOG(INFO) << "map file is " << ss.str();
  remill::StoreModuleToFile(worker->map_semantics.get(),ss.str(), false);
  LOG(INFO) << "finished loading traces for mapping and optimizing";
}

void PreLifter::decodeAndLiftMappings() {
  const auto &trace_list_path = native::Workspace::TraceListPath();
  if (remill::FileExists(trace_list_path)) {
    const auto &memory = trace_manager->memory;
    LOG(INFO) << "grabbing traces from the trace list file in the workspace";
    std::ifstream trace_file_stream;
    uint64_t prev_base = 0;
    uint64_t trace_address;
    trace_file_stream.open(trace_list_path);
    std::vector<uint64_t> trace_batch;
    if (trace_file_stream) {
      std::string trace_heads_label;
      trace_file_stream >> trace_heads_label;
      while (trace_file_stream >> std::hex >> trace_address) {
        if (!memory->IsSameMappedRange(trace_address, prev_base)) {
          if (!trace_batch.empty()) {
            workers.emplace_back(new Worker(new llvm::LLVMContext()));
            auto &worker = workers.back();
            worker->traces = trace_batch;
          }
          prev_base = trace_address;
          trace_batch.clear();
        }

        trace_batch.push_back(trace_address);
      }
      if (!trace_batch.empty()) {
        workers.emplace_back(new Worker(new llvm::LLVMContext()));
        auto &worker = workers.back();
        worker->traces = trace_batch;
      }
      trace_file_stream.close();
    } else {
      LOG(ERROR) << "cannot read the trace list file at " << trace_list_path
          << " , falling back to recursive descent and linear sweep";
      return;
    }
  }
}

void PreLifter::preLift(void) {
  const auto path = Workspace::Dir() + "/prelift_traces";
  (void) remill::TryCreateDirectory(path);
  decodeAndLiftMappings();
  for (auto &worker : workers) {
    worker->thread = std::thread(&LiftMapping, worker.get(), this);
  }
  for (auto &worker : workers) {
    worker->thread.join();
  }
  auto dir = opendir(path.c_str());
  CHECK(dir != nullptr)
      << "Could not list the " << path << " directory";

  while (auto ent = readdir(dir)) {
    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
      continue;
    }

    std::stringstream ss;
    std::stringstream addr_stream;
    uint64_t map_label;
    ss << path << "/" << ent->d_name;
    auto name = std::string(ent->d_name);
    name = name.substr(0,name.find("-"));
    LOG(INFO) << "name is " << name;
    addr_stream << name;
    addr_stream >> std::hex >> map_label;
    auto page_name = std::string(trace_manager->memory->FindRange(map_label).Name());
    LOG(INFO) << "page_name" << page_name << " " << std::hex << map_label << std::dec;
    trace_manager->memory->aot_traces[page_name] =
        std::shared_ptr<llvm::Module>(remill::LoadModuleFromFile(ctx, ss.str(), true));
  }
  closedir(dir);
  // NOTE the native address space will be a shared resource among all threads
  // load trace files into address space under the same module here
}

} // namespace native
} //  namespace klee

