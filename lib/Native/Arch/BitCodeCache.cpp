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
#include "Native/Workspace/Workspace.h"
#include "Native/Arch/TraceManager.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

#include "remill/BC/Util.h"

namespace klee {

class Executor;

namespace native {


void BitCodeCache::WriteToWorkspace(llvm::Module &module) {
  remill::StoreModuleToFile(&module, Workspace::BitcodeCachePath(), false);
}


}//  namespace native
}// namespace klee
