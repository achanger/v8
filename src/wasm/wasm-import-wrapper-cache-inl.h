// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_WASM_WASM_IMPORT_WRAPPER_CACHE_INL_H_
#define V8_WASM_WASM_IMPORT_WRAPPER_CACHE_INL_H_

#include "src/compiler/wasm-compiler.h"
#include "src/counters.h"
#include "src/handles-inl.h"
#include "src/objects/code-inl.h"
#include "src/wasm/value-type.h"
#include "src/wasm/wasm-code-manager.h"

namespace v8 {
namespace internal {
namespace wasm {

// Implements a cache for import wrappers.
class WasmImportWrapperCache {
 public:
  WasmCode* GetOrCompile(Isolate* isolate, compiler::WasmImportCallKind kind,
                         FunctionSig* sig) {
    // TODO(titzer): remove the isolate parameter.
    base::MutexGuard lock(&mutex_);
    CacheKey key(static_cast<uint8_t>(kind), *sig);
    WasmCode*& cached = entry_map_[key];
    if (cached == nullptr) {
      // TODO(wasm): no need to hold the lock while compiling an import wrapper.
      // TODO(wasm): no need to compile the code onto the heap and copy back.
      HandleScope scope(isolate);
      bool source_positions = native_module_->module()->origin == kAsmJsOrigin;
      Handle<Code> code = compiler::CompileWasmImportCallWrapper(
                              isolate, kind, sig, source_positions)
                              .ToHandleChecked();
      auto counters = isolate->counters();
      counters->wasm_generated_code_size()->Increment(code->body_size());
      counters->wasm_reloc_size()->Increment(code->relocation_info()->length());
      cached =
          native_module_->AddAnonymousCode(code, WasmCode::kWasmToJsWrapper);
    }
    return cached;
  }

 private:
  friend class NativeModule;
  mutable base::Mutex mutex_;
  NativeModule* native_module_;
  using CacheKey = std::pair<uint8_t, FunctionSig>;
  std::unordered_map<CacheKey, WasmCode*, base::hash<CacheKey>> entry_map_;

  explicit WasmImportWrapperCache(NativeModule* native_module)
      : native_module_(native_module) {}
};

}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_WASM_IMPORT_WRAPPER_CACHE_INL_H_
