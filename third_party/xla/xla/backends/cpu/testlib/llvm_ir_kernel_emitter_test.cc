/* Copyright 2024 The OpenXLA Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "xla/backends/cpu/testlib/llvm_ir_kernel_emitter.h"

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "xla/codegen/kernel_definition.h"
#include "xla/codegen/kernel_spec.h"
#include "xla/codegen/llvm_ir_kernel_source.h"
#include "xla/runtime/buffer_use.h"
#include "xla/stream_executor/launch_dim.h"
#include "xla/tsl/platform/statusor.h"
#include "tsl/platform/casts.h"

namespace xla::cpu {

TEST(LlvmIrKernelEmitterTest, ParseLlvmIr) {
  static constexpr absl::string_view kLlvmIr = R"(
    define ptr @noop(ptr noundef %0) {
      ret ptr null
    }
  )";

  LlvmIrKernelEmitter::KernelArg arg{1024, BufferUse::kWrite};
  LlvmIrKernelEmitter emitter(kLlvmIr, "noop", se::ThreadDim(), {arg});

  TF_ASSERT_OK_AND_ASSIGN(KernelDefinition kernel_definition,
                          emitter.EmitKernelDefinition());

  const KernelSpec& kernel_spec = kernel_definition.spec();

  // Check that kernel arguments were converted to buffer allocations.
  ASSERT_EQ(kernel_spec.buffer_uses().size(), 1);

  BufferUse buffer_use = kernel_spec.buffer_uses().front();
  EXPECT_EQ(buffer_use.access(), BufferUse::kWrite);
  EXPECT_EQ(buffer_use.slice().index(), 0);
  EXPECT_EQ(buffer_use.slice().offset(), 0);
  EXPECT_EQ(buffer_use.slice().size(), 1024);

  // Check that LLVM IR was parsed and loaded as a LLVM IR kernel source.
  auto& src =
      tsl::down_cast<const LlvmIrKernelSource&>(kernel_definition.source());
  EXPECT_EQ(src.kernel_function()->getName(), "noop");
}

}  // namespace xla::cpu
