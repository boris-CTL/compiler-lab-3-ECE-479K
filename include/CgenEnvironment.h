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

namespace cool
{
  template <class T, class V>
  class SymbolTablePlusType
  {
    private:
      using TableEntry = std::pair<T *, V *>;
      using Scope = std::unordered_map<Symbol, TableEntry>;

      std::vector<Scope> scopes;

    public:
      SymbolTablePlusType() = default;

      void enterscope() { this->scopes.emplace_back(); }
      void exitscope()
      {
        assert(!this->scopes.empty() &&
          "exitscope: Can't remove scope from an empty symbol table.");
        this->scopes.pop_back();
      }

      void insert(Symbol const &k, T *type, V *v)
      {
        assert(!this->scopes.empty() &&
          "insert: Can't add a symbol without a scope.");
        assert(v && "insert: Can't add a nullptr value.");
        this->scopes.back().emplace(k, std::make_pair(type, v));
      }

      TableEntry find_in_scopes(Symbol const &k) const
      {
        for (auto it = this->scopes.rbegin(); it != this->scopes.rend(); ++it)
        {
          auto entry = it->find(k);
          if (entry != it->end())
          {
            return entry->second;
          }
        }
        return {nullptr, nullptr};
      }
  };
}


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
  std::pair<llvm::Type *, llvm::Value *> findInScopes(Symbol name);

  void addBinding(Symbol name, llvm::Value *var, llvm::Type *type) {
    varTable.insert(name, type, var);
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

  void set_inst(llvm::Value *val) {
    this->current_instance = val;
  }
  
  void set_par(CgenNode *cls) { current_par_class = cls; }

  bool check_if_inherited() {
    bool ret = (curClass != current_par_class);
    return ret;
  }

  llvm::Value *Boxing(llvm::Value *primitiv) {
    llvm::Type *obj_type;
    llvm::Value *obj_being_boxed;
    llvm::FunctionType *func_type = llvm::FunctionType::get(classTable.ptr, false);
    auto primitive_type = primitiv->getType();

    if (primitive_type == classTable.i32) {
      obj_type = llvm::StructType::getTypeByName(context, "Int");
      obj_being_boxed = builder.CreateCall(theModule.getOrInsertFunction("Int_new", func_type));
    } else if (primitive_type == classTable.i1) {
      obj_type = llvm::StructType::getTypeByName(context, "Bool");
      obj_being_boxed = builder.CreateCall(theModule.getOrInsertFunction("Bool_new", func_type));
    } else if (primitive_type == classTable.ptr) {
      obj_type = llvm::StructType::getTypeByName(context, "String");
      obj_being_boxed = builder.CreateCall(theModule.getOrInsertFunction("String_new", func_type));
    }

    llvm::Value *address_of_val = builder.CreateStructGEP(obj_type, obj_being_boxed, 1);
    builder.CreateStore(primitiv, address_of_val);
    return obj_being_boxed;
  }

  llvm::Value *get_inst() {
    return this->current_instance;
  }


private:
  // Augument varTable
  // cool::SymbolTable<llvm::Value> varTable;
  cool::SymbolTablePlusType<llvm::Type, llvm::Value> varTable;
  CgenNode *curClass;
  llvm::Value *current_instance;
  CgenNode *current_par_class;

public:
  CgenClassTable &classTable;
  // These are references to the current LLVM context and module,
  // and they point to the ones in CgenClassTable.
  llvm::LLVMContext &context;
  llvm::IRBuilder<llvm::NoFolder> &builder;
  llvm::Module &theModule;

  // The following types are declared here for convenience.
  llvm::Type *i64, *i32, *i8, *i1, *voidTy, *ptr;
  // llvm::PointerType *ptr;
};
#endif // CGENENVIRONMENT_H
