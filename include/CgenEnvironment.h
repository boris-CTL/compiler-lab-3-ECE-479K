#ifndef CGENENVIRONMENT_H
#define CGENENVIRONMENT_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/NoFolder.h>

#include "CgenClassTable.h"
#include "CgenNode.h"

// CgenEnvironment provides the environment for code generation of a method.
// Its main task is to provide a mapping from Cool names to LLVM Values.
// This mapping needs to be maintained as scopes are entered and exited, new
// variables are declared, and so on. CgenEnvironment is also a good place
// to put non-local information you will need during code generation. Two
// examples are the current CgenNode and the current Function.
class CgenEnvironment {
public:
  // Class CgenEnvironment should be constructed by a class prior to code
  // generation for each method. You may need to add parameters to this
  // constructor.
  CgenEnvironment(CgenNode *curClass)
      : varTable(), curClass(curClass),
        classTable(curClass->getClasstable()), context(classTable.context),
        builder(classTable.builder), theModule(classTable.theModule) {
    varTable.enterscope();
    i64 = classTable.i64;
    i32 = classTable.i32;
    i8 = classTable.i8;
    i1 = classTable.i1;
    ptr = classTable.ptr;
    voidTy = classTable.voidTy;
    // add code here as needed
  }

  CgenNode *getClass() const { return curClass; }
  void setClass(CgenNode *c) { curClass = c; }

  // Must return the CgenNode for a class given the symbol of its name
  CgenNode *typeToClass(Symbol t) const;

  // NOTE: You should use cool tree type information
  llvm::Value *findInScopes(Symbol name);

  void addBinding(Symbol name, llvm::Value *var) {
    varTable.insert(name, var);
  }
  void openScope() { varTable.enterscope(); }
  void closeScope() { varTable.exitscope(); }

  // LLVM Utils:
  // Create a new llvm function in the current module
  llvm::Function *createLlvmFunction(std::string const &funcName,
                                       llvm::Type *retType,
                                       llvm::ArrayRef<llvm::Type *> argTypes) const {
    return classTable.createLlvmFunction(funcName, retType, argTypes,
                                            /*isVarArgs=*/false);
  }
  // Insert a new BasicBlock at the end of the current function (the function
  // that builder is in)
  llvm::BasicBlock *newBbAtFend(std::string const &name) const;

  // Insert an alloca instruction in the head BasicBlock of the current
  // function, such that this alloca is available in all BasicBlocks of the
  // function.
  llvm::AllocaInst *insertAllocaAtHead(llvm::Type *ty, const llvm::Twine& = "") const;
  // Get or insert a BasicBlock with the name "abort" which calls the ::abort
  // function. This block will be inserted at the end of the given function,
  // without moving the builder.
  llvm::BasicBlock *getOrInsertAbortBlock(llvm::Function *f) const;

  // Gets the function the IRBuilder is currently in
  llvm::Function *getFunction() const;

  llvm::Type *getType(Symbol decl) const;

  llvm::Value *getDefaultInit(Symbol type) const;
  llvm::Value *getDefaultInit(Type *type) const;

  // Implement and use the following to handle boxing and unboxing

  llvm::Value *box(llvm::Value *src, Symbol from) const;
  llvm::Value *unbox(llvm::Value *src, Symbol to) const;

  // You may add more functions here as necessary

  std::tuple<Type *, Value *>get_attr_ptr(CgenNode *cur_class, std::string name, Value *obj_ptr){
    int idx = curClass->find_idx(name);

    while(idx == -1){
      // offset += rents->attribute_layout.size() + 1;
      // rents = rents->getParentnd();
      // idx = rents->find_idx(name);
      return std::tuple<Type *, Value *>(nullptr, nullptr);
    }
    
    auto [attr_ty, _] = curClass->attribute_layout[idx-1];
    Value *attr_ptr = this->builder.CreateStructGEP(cur_class->getType(), obj_ptr, idx);

    return std::tuple<Type *, Value *>(attr_ty, attr_ptr);
  }
  

private:
  cool::SymbolTable<llvm::Value> varTable;
  CgenNode *curClass;

public:
  CgenClassTable &classTable;
  // These are references to the current LLVM context and module,
  // and they point to the ones in CgenClassTable.
  llvm::LLVMContext &context;
  llvm::IRBuilder<llvm::NoFolder> &builder;
  llvm::Module &theModule;

  // The following types are declared here for convenience.
  llvm::Type *i64, *i32, *i8, *i1, *voidTy;
  llvm::PointerType *ptr;
};
#endif // CGENENVIRONMENT_H
