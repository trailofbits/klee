#!/usr/bin/env python
"""
  Copyright (c) 2019 Trail of Bits, Inc.
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
      http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
"""
from binaryninja import *
from os import listdir
from sys import argv

# this program assumes that the memory file provided is in the workspace
if len(argv) < 2:
    print("please specify the location of the memory directory in the workspace as an argument for this program")
    print("Example: `python locate_traces.py ./ws/memory/`")
    exit(1)


memory_directory_path = argv[1]
if memory_directory_path[-1] != "/":
    memory_directory_path += "/"

traces = []

def mark_traces_in_mapping(mapping):
    bv = binaryview.BinaryViewType["ELF"].open(memory_directory_path + mapping)
    if(not bv):
        return
    print(memory_directory_path + mapping)
    bv.update_analysis_and_wait()
    base = int(mapping.split("_")[0], 16)
    pc = base
    for func in bv.functions:
        for bb in func:
            # make the beginning of basic blocks a trace
            pc = bb.start if bb.start > base else base + bb.start
            traces.append(pc)
            for ins in bb:
                # this loop is for marking in the return addresses of function calls
                ins_array, size = ins
                pc += size
                if ins_array[0].text == 'call':
                    # print("call pc is " + hex(pc))
                    traces.append(pc)


def is_executable(mapping):
    umask = "".join(mapping.split("_")[2:4])
    return "x" in umask

def mark_all_traces():
    for mapping in listdir(memory_directory_path):
        if is_executable(mapping):
            mark_traces_in_mapping(mapping)

def write_all_traces_to_file():
    workspace = "/".join(memory_directory_path.split("/")[:-2])
    print("workspace: {}".format(workspace))
    with open(workspace + "/trace_list","a+") as trace_file:
        trace_file.write("======TRACE=ADDRESSES======\n")
        for trace in traces:
            trace_file.write(hex(trace).strip("L") + '\n')

if __name__  == "__main__":
    mark_all_traces()
    write_all_traces_to_file()
