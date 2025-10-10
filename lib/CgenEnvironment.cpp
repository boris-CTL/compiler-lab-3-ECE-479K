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
  return this->varTable.find_in_scopes(name);
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
  // 
}

llvm::Value *CgenEnvironment::getDefaultInit(Symbol type) const {
  if(type == Int){
    return ConstantInt::get(this->i32, 0);
  } else if(type == Bool){
    return ConstantInt::get(this->i1, 0);
  }

  CgenNode *this_class = this->typeToClass(type);
  errs() << "OH GEE " << this_class->getInitFunctionName() << "\n";
  Function *init_func = this->theModule.getFunction(this_class->getInitFunctionName());
  return this->builder.CreateCall(init_func, {});
}

llvm::Value *CgenEnvironment::getDefaultInit(Type *type) const {
  if(type == this->i32){
    return ConstantInt::get(this->i32, 0);
  } else if(type == this->i1){
    return ConstantInt::get(this->i1, 0);
  }

  return nullptr;
}


llvm::Value *CgenEnvironment::box(llvm::Value *src, Symbol from) const {
  // TODO: implement
}
llvm::Value *CgenEnvironment::unbox(llvm::Value *src, Symbol to) const {
  // TODO: implement
}