#include "CgenEnvironment.h"
#include "CgenNode.h"
#include "symbol.h"
#include "symtab.h"

#include "llvm/IR/ValueSymbolTable.h"

using namespace llvm;
/*********************************************************************

CgenEnvironment functions

*********************************************************************/

// Look up a CgenNode given a symbol
CgenNode *CgenEnvironment::typeToClass(Symbol t) const {
  if (t == SELF_TYPE)
    return getClass();

  return classTable.find_in_scopes(t);
}

llvm::Value *CgenEnvironment::findInScopes(Symbol name) {
  // TODO: implement
  return nullptr;
}

llvm::BasicBlock *CgenEnvironment::newBbAtFend(std::string const &name) const {
    return llvm::BasicBlock::Create(context, name,
                                    getFunction());
}

llvm::Function *CgenEnvironment::getFunction() const { return builder.GetInsertBlock()->getParent(); }



BasicBlock *CgenEnvironment::getOrInsertAbortBlock(Function *f) const {
  if (auto *abort = f->getValueSymbolTable()->lookup("abort")) {
    return cast<BasicBlock>(abort);
  }
  auto *bb = BasicBlock::Create(this->context, "abort", f);
  IRBuilder<> builder(bb);
  FunctionCallee abort = this->theModule.getOrInsertFunction("abort", voidTy);
  builder.CreateCall(abort, {});
  builder.CreateUnreachable();
  return bb;
}

AllocaInst *CgenEnvironment::insertAllocaAtHead(Type *ty, const Twine &name) const {
  BasicBlock &entry = getFunction()->getEntryBlock();
  // Insert before the first instruction of this bb
  return new AllocaInst(ty, 0, name, entry.begin());
}


llvm::Type *CgenEnvironment::getType(Symbol decl) const {
  // TODO: implement
}
llvm::Value *CgenEnvironment::getDefaultInit(Symbol type) const {
  // TODO: implement
}
llvm::Value *CgenEnvironment::box(llvm::Value *src, Symbol from) const {
  // TODO: implement
}
llvm::Value *CgenEnvironment::unbox(llvm::Value *src, Symbol to) const {
  // TODO: implement
}