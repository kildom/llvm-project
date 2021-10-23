//===-- Program.cpp - Implement OS Program Concept --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements the operating system Program concept.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Program.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/raw_ostream.h"
#include <system_error>
#include <sstream>
#include <forward_list>
using namespace llvm;
using namespace sys;

//===----------------------------------------------------------------------===//
//=== WARNING: Implementation here must contain only TRULY operating system
//===          independent code.
//===----------------------------------------------------------------------===//

int sys::ExecuteAndWait(StringRef Program, ArrayRef<StringRef> Args,
                        Optional<ArrayRef<StringRef>> Env,
                        ArrayRef<Optional<StringRef>> Redirects,
                        unsigned SecondsToWait, unsigned MemoryLimit,
                        std::string *ErrMsg, bool *ExecutionFailed,
                        Optional<ProcessStatistics> *ProcStat) {

  char **c_arg = new char*[Args.size()];
  const char **c_env = Env ? new const char*[Env->size()] : nullptr;
  int c_env_count = 0;
  const char *c_redir[3] = { nullptr, nullptr, nullptr };

  std::forward_list<std::string> cache;

  for (size_t i = 0; i < Args.size(); i++) {
    cache.push_front(Args[i].str());
    c_arg[i] = (char *)cache.front().c_str();
  }

  if (c_env) {
    c_env_count = Env->size();
    for (int i = 0; i < c_env_count; i++) {
      cache.push_front((*Env)[i].str());
      c_env[i] = cache.front().c_str();
    }
  }

  for (size_t i = 0; i < Redirects.size(); i++) {
    if (Redirects[i]) {
      cache.push_front(Redirects[i]->str());
      c_redir[i] = cache.front().c_str();
    } else {
      c_redir[i] = nullptr;
    }
  }

  int _my_port_exec(const char* program, const char** args, int arg_count, const char** envs, int env_count, const char** redir);

  int r = _my_port_exec(Program.str().c_str(), (const char**)c_arg, Args.size(), c_env, c_env_count, c_redir);

  cache.clear();
  delete[] c_arg;
  delete[] c_env;

  if (ExecutionFailed)
    *ExecutionFailed = r < 0;

  if (r < 0 && ErrMsg) {
      *ErrMsg = "Program execution failed";
  }

  if (ProcStat && *ProcStat) {
    (*ProcStat)->TotalTime = std::chrono::microseconds(1000000);
    (*ProcStat)->UserTime = std::chrono::microseconds(1000000);
    (*ProcStat)->PeakMemory = 1000000;
  }

  return r;
}

ProcessInfo sys::ExecuteNoWait(StringRef Program, ArrayRef<StringRef> Args,
                               Optional<ArrayRef<StringRef>> Env,
                               ArrayRef<Optional<StringRef>> Redirects,
                               unsigned MemoryLimit, std::string *ErrMsg,
                               bool *ExecutionFailed) {
  if (ErrMsg) {
      *ErrMsg = "Asynchronous program execution not supported!";
  }

  if (ExecutionFailed)
    *ExecutionFailed = true;

  return ProcessInfo();
}

bool sys::commandLineFitsWithinSystemLimits(StringRef Program,
                                            ArrayRef<const char *> Args) {
  SmallVector<StringRef, 8> StringRefArgs;
  StringRefArgs.reserve(Args.size());
  for (const char *A : Args)
    StringRefArgs.emplace_back(A);
  return commandLineFitsWithinSystemLimits(Program, StringRefArgs);
}

void sys::printArg(raw_ostream &OS, StringRef Arg, bool Quote) {
  const bool Escape = Arg.find_first_of(" \"\\$") != StringRef::npos;

  if (!Quote && !Escape) {
    OS << Arg;
    return;
  }

  // Quote and escape. This isn't really complete, but good enough.
  OS << '"';
  for (const auto c : Arg) {
    if (c == '"' || c == '\\' || c == '$')
      OS << '\\';
    OS << c;
  }
  OS << '"';
}

// Include the platform-specific parts of this class.
#ifdef LLVM_ON_UNIX
#include "Unix/Program.inc"
#endif
#ifdef _WIN32
#include "Windows/Program.inc"
#endif
