/*********************************************************************
 Intermediate code generator for COOL: SKELETON

 Read the comments carefully and add code to build an LLVM program
*********************************************************************/
#include <cstddef>
#include <string>
#include <stack>

#include "CgenClassTable.h"
#include "CgenEnvironment.h"
#include "CgenNode.h"
#include "symbol.h"

extern int cgen_debug, curr_lineno;
using namespace llvm;

/*********************************************************************

  The next two functions are actually called at the end of code
  generation, but are mostly copied from Lab 2 so placed here at
  the top.

*********************************************************************/

// Create LLVM entry point. This function will initiate our Cool program
// by generating the code to execute (new Main).main()
//
// HINT: This will actually be called after you generate the code for
// Main_main() using the CgenNode::codegen_mainmain() function below, so do this
// after you create a simple codegen_mainmain().

void CgenClassTable::codeMain() {
    // TODO: add code here

    Function *main_func = this->createLlvmFunction("main", this->i32, {}, false);
    BasicBlock *BB = BasicBlock::Create(this->context, "entry", main_func);
    builder.SetInsertPoint(BB);
    PointerType *ptr = PointerType::get(this->context, 0);
    FunctionType *FTInit = FunctionType::get(ptr, false);
    Value *ptrToMainInstance =
      builder.CreateCall(this->theModule.getOrInsertFunction("Main_new", FTInit));
    FunctionCallee FCMainMain = this->theModule.getFunction("Main_main");
    Value *retVFromMainMain = builder.CreateCall(FCMainMain, {ptrToMainInstance});

    FunctionType *FTPrintf = FunctionType::get(i32, ptr, true);
    FunctionCallee FCPrintf =
        this->theModule.getOrInsertFunction("printf", FTPrintf);
    GlobalVariable *MainLogString =
        builder.CreateGlobalString("Main.main() returned %d\n", ".str");
    Value *args[] = {MainLogString, retVFromMainMain};
    builder.CreateRet(ConstantInt::get(this->i32, 0));
    // TODO: In Lab 3 you will have to allocate the Main object on the heap
    //       and initialize it somehow. There are multiple good ways to do this.
    // HINT: This will involve calling the constructor

    // TODO: Call Main_main(). This returned int for Lab 2 and Lab 3 CP1
    //       Later it will return the appropriate type defined in the
    //       Cool program.
    // HINT: Copy from Lab 2 initially, but don't forget to pass in the object
    //       pointer you allocated above.

    // TODO: Call printf with the string address of "Main_main() returned %d\n"
    // and the return value of Main_main() as its arguments
    // TODO:  Remove this after Lab 3 CP1.

    // Insert return 0
}

// For Lab 3 CP1, we will use the same approach as with Lab 2
// code-gen function main() in class Main
void CgenNode::codegenMainmain() {
    // Copy code from Lab 2 initially, later just handle as part of
    // Setting up class Main.
    // assert(this->name->get_string() == "Main");

    // codeClass();
    // for (auto *f : features) {
    //   if (f->get_name()->get_string() == "main") {
    //     f->code(new CgenEnvironment(this));
    //   }
    // }
}

/*********************************************************************

  First pass functions start here

*********************************************************************/

//
// Class setup. You may need to add parameters to this function so that
// the classtable can provide setup information (such as the class tag
// that should be used by this class).
//
// Things that setup should do:
//  - layout the features of the class
//  - create the types for the class and its vtable
//  - create global definitions used by the class such as the class vtable
//
void CgenNode::setup(int tag, int depth) {
    this->tag = tag;

    // TODO: Layout the features of the class by implementing
    //      layoutFeatures(), which is called from here.
    layoutFeatures();

    // this->getType();

    // TODO: CP2 below this point
    // TODO: Create the global vtable instance
    // HINT: This will be a llvm::GlobalVariable. You should leak it (e.g. `new GlobalVariable(...)`)

    // HINT: Don't forget the metadata (including a raw character
    //       string for the type name)
    // HINT: You can get the allocated size of a type with
    //       llvm::DataLayout;
}

// Laying out the features involves creating a Function for each method
// and assigning each attribute a slot in the class structure.
void CgenNode::layoutFeatures() {

    CgenClassTable &classTable = getClasstable();
    struct_type = StructType::create(classTable.context, getTypeName());
    std::stack<CgenNode *> inheritance_stack;
    CgenNode *current = this;
    while (current) {
      inheritance_stack.push(current);
      current = current->getParentnd();
    }

    while (!inheritance_stack.empty()) {
      current = inheritance_stack.top();
      for (auto const &feature: current->features)
	    feature->layout_feature(this, current);
        inheritance_stack.pop();
    }

    struct_type->setBody(get_feature_type());
    this->insert_method_body(classTable.builder.CreateGlobalString(name->get_string(), ".str_" + name->get_string()));
    llvm::DataLayout DL = classTable.theModule.getDataLayout();
    llvm::TypeSize alloc_size = DL.getTypeAllocSize(struct_type);
    Constant *struct_size = classTable.builder.getInt32(alloc_size);
    this->insert_method_body(struct_size);
    this->insert_method_body(ConstantInt::get(classTable.i32, getTag()));

    struct_type_of_vtable = StructType::create(get_vtable_ty_list(), getVtableTypeName());
    Constant* vtable_const = ConstantStruct::get(struct_type_of_vtable, get_vtable_const_list());
    llvm::GlobalVariable* vtable_global_var = new llvm::GlobalVariable(classTable.theModule,
                                                                    struct_type_of_vtable,
                                                                    true, // constant
                                                                    llvm::GlobalValue::ExternalLinkage,
                                                                    vtable_const,
                                                                    getVtableName());
}



// Assign this attribute a slot in the class structure
// CTL: this one should be fine.
void attr_class::layout_feature(CgenNode *cls) {
    // TODO: add code here to add an attribute to the layout
    // HINT: Consider inheritance, byt remember that new definitions
    //       of an argument ID are added, leaving the previous ones
    //       alone to correctly and easily deal with inheritance
    // cls->insert_attribute(this->type_decl, this);
    auto attr_type = type_decl->get_string();
    if (attr_type == "Bool" or attr_type == "bool") {
        CgenClassTable &classTab = cls->getClasstable();
        cls->push_attributes(name->get_string(), llvm::Type::getInt1Ty(classTab.context));
    } else if (attr_type == "Int" or attr_type == "int") {
        CgenClassTable &classTab = cls->getClasstable();
        cls->push_attributes(name->get_string(), llvm::Type::getInt32Ty(classTab.context));
    } else if (attr_type == "sbyte*") {
        CgenClassTable &classTab = cls->getClasstable();
        cls->push_attributes(name->get_string(), llvm::PointerType::get(classTab.context, 0));
    } else {
        CgenClassTable &classTab = cls->getClasstable();
        cls->push_attributes(name->get_string(), llvm::PointerType::get(classTab.context, 0));
    }
}

void attr_class::layout_feature(CgenNode *cls, CgenNode *par) {
  this->layout_feature(cls);
}

// Create the LLVM Function corresponding to this method.
void method_class::layout_feature(CgenNode *cls, CgenNode *par) {
    // TODO: CP2 -- not needed in CP1

    // TODO: Declare the method and add it to the class
    // HINT: use the method formals and the helper function from ClassTable
    // HINT: Use the methods in CgenNode to find names for things
    // HINT: You will need to think about how to handle inheritance
    // HINT: Declare the function similarly to what you did in Lab 2 for
    // Main_main()
    //       though at this point you are only adding a declaration, with the
    //       actual definition happening in the second pass (in code_class()).
    // HINT: Look at the definition of code_class in ClassTable or step through
    // debug.
    
    CgenClassTable &classTab = cls->getClasstable();
    std::string method_name = cls->getFullMethodName(this->get_name()->get_string());
    if(classTab.theModule.getFunction(method_name)){
        return;
    }

    bool flag = (cls == par);
    if (!flag) {
        std::string name_of_the_inherited_method = par->getFullMethodName(this->get_name()->get_string());
        Function *func = classTab.theModule.getFunction(name_of_the_inherited_method);
        FunctionType *func_type = func->getFunctionType();
        cls->push_method(ConstantExpr::getBitCast(func, PointerType::get(func_type, 0)));
        return;
    }

    Type *method_ret_type = classTab.get_llvm_type_from_symbol(this->return_type);
    llvm::SmallVector<llvm::Type *> list_of_arguments_type;
    list_of_arguments_type.emplace_back(PointerType::get(classTab.context, 0));

    for (auto const &formal: this->formals) {
        Type *formal_type = classTab.get_llvm_type_from_symbol(formal->get_type_decl());
        list_of_arguments_type.emplace_back(formal_type);
    }
    // CTL: SUSSSSSSSSSSSSSSSS
    auto [ft, func] = classTab.createLlvmFunctionDetails(method_name, method_ret_type, list_of_arguments_type, false);
    cls->push_method(ConstantExpr::getBitCast(func, PointerType::get(ft, 0)));
}

void method_class::layout_feature(CgenNode *cls) {
}

StructType *CgenNode::getType() const {
    if (body) {
        assert(body->isSized());
        return body;
    }

    std::vector<Type *> attr_types;
    attr_types.push_back(this->getClasstable().ptr);  // For Vtable
    for (int i = 0; i < this->attribute_layout.size(); i++) {
        Type *attr_type = std::get<0>(this->attribute_layout[i]);
        attr_types.push_back(attr_type);
    }

    StructType *this_class_type = StructType::create(this->getClasstable().context, this->getTypeName());
    this_class_type->setBody(attr_types, false);
    body = this_class_type;
    return this_class_type;
}

// TODO: Use information from feature layout to create the vtable type.
//       Make sure to use get_type_name() for the name of the class type
// Ignore padding
StructType *CgenNode::getVtableType() const {
    if (vtable) {
        assert(vtable->isSized());
        return vtable;
    }
    // TODO: initalize the vtable type
    assert(false && "todo");
}

/*********************************************************************

  Second pass functions start here

*********************************************************************/

// Class codegen. This should be performed after every class has been set up.
// Generate code for each method of the class.
void CgenNode::codeClass() {
    // No code generation for basic classes. The runtime will handle that.
    if (basic()) {
        return;
    }

    // TODO: add code here for programmer-defined classes
    // HINT: You'll need an environment and generate code for each method,
    //       including the initializer for an object (a.k.a. constructor).

    CgenEnvironment *env = new CgenEnvironment(this);

    this->codeInitFunction(env);

    // Jer:
    // for (auto [method_name, method] : this->method_layout) {
    //     errs() << "Class: " << this->name->get_string() << "\tmethod: " << method_name << "\n";
    //     Function *current_method_dec = this->getClasstable().theModule.getFunction(method_name);
    //     BasicBlock *entry_BB = BasicBlock::Create(this->getClasstable().context, "entry", current_method_dec);
    //     env->builder.SetInsertPoint(entry_BB);
    //     method->code(env);
    // }
}

// void CgenNode::codeInitFunction(CgenEnvironment *env) {
//     // TODO: Add code here for class initialization (constructor)
//     // HINT: You can choose to allocate memory with malloc here, or
//     //       you can decide to do it elsewhere.
//     // HINT: To allocate memory, generate a call to malloc with
//     //       IRBuilder that will be emitted into the LLVM bitcode
//     //       and run at runtime. We suggest that you do *not*
//     //       use builder.CreateMalloc().
//     // HINT: You may find theModule.getNamedGlobal() useful.
//     // HINT: You may find llvm::StructType::getTypeByName() useful.
//     // HINT: You will need to use builder.CreateStructGEP().
//     // If you are doing garbage collection, type-aware allocation is highly recommended
//     // e.g. use LLVM's type-aware allocation facilities to make sure that
//     // each object has a unique allocator and unique allocation pool

//     Function *init_func = env->createLlvmFunction(this->getInitFunctionName(), env->ptr, {});
//     BasicBlock *entry_bb = BasicBlock::Create(env->context, "entry", init_func, nullptr);
//     env->builder.SetInsertPoint(entry_bb);

//     DataLayout DL = env->theModule.getDataLayout();
//     Value *class_alloc_size = ConstantInt::get(env->i64, DL.getTypeAllocSize(body));
//     Value *class_obj_ptr = env->builder.CreateCall(env->theModule.getFunction("malloc"), class_alloc_size, "alloc_classtype", nullptr);

//     for (int i = 0; i < attribute_layout.size(); i++) {
//         auto [attr_type, cool_attr_obj] = this->attribute_layout[i];
//         Value *init_val = cool_attr_obj->code(env);

//         if (init_val == nullptr) {
//             init_val = env->getDefaultInit(attr_type);
//         }

//         Value *attr_ptr = env->builder.CreateStructGEP(this->getType(), class_obj_ptr, i + 1);

//         env->builder.CreateStore(init_val, attr_ptr);
//     }

//     env->builder.CreateRet(class_obj_ptr);
// }

void CgenNode::codeInitFunction(CgenEnvironment *env) {
    // TODO: Add code here for class initialization (constructor)
    // HINT: You can choose to allocate memory with malloc here, or
    //       you can decide to do it elsewhere.
    // HINT: To allocate memory, generate a call to malloc with
    //       IRBuilder that will be emitted into the LLVM bitcode
    //       and run at runtime. We suggest that you do *not*
    //       use builder.CreateMalloc().
    // HINT: You may find theModule.getNamedGlobal() useful.
    // HINT: You may find llvm::StructType::getTypeByName() useful.
    // HINT: You will need to use builder.CreateStructGEP().
    // If you are doing garbage collection, type-aware allocation is highly recommended
    // e.g. use LLVM's type-aware allocation facilities to make sure that
    // each object has a unique allocator and unique allocation pool

    CgenClassTable &classTab = this->getClasstable();
    llvm::DataLayout DL = env->theModule.getDataLayout();
    auto data_alloc_size = DL.getTypeAllocSize(this->get_struct_type());
    Constant *struct_size = env->builder.getInt32(data_alloc_size);

    
    PointerType *ptr = PointerType::get(env->context, 0);
    FunctionType *func = FunctionType::get(ptr, false);
    Function *par = env->theModule.getFunction(this->getInitFunctionName());
    if (!par)
        par = Function::Create(func, Function::ExternalLinkage, this->getInitFunctionName(), env->theModule);

    BasicBlock *BB = BasicBlock::Create(env->context, "entry", par);
    env->builder.SetInsertPoint(BB);

    
    CgenClassTable &classTabb = this->getClasstable();
    FunctionType *func_malloc = FunctionType::get(ptr, classTabb.i32, false);
    auto func_malloc_callee = env->theModule.getOrInsertFunction("malloc", func_malloc);
    Value *call_instruction = env->builder.CreateCall(func_malloc_callee, {struct_size});

    int index = 0;
    
    // 只有基本類才有 vtable 和 self 指標 (AI生的)
    if (this->name == Object || this->name == IO || this->name == Int || this->name == Bool || this->name == String) {
        Value *pointer_at_field = env->builder.CreateStructGEP(struct_type, call_instruction, index++);
        env->builder.CreateStore(env->theModule.getNamedGlobal(getVtableName()), pointer_at_field);
        pointer_at_field = env->builder.CreateStructGEP(struct_type, call_instruction, index++);
        env->builder.CreateStore(call_instruction, pointer_at_field);
    }
    
    // Value *pointer_at_field = env->builder.CreateStructGEP(struct_type, call_instruction, index++);
    // env->builder.CreateStore(env->theModule.getNamedGlobal(getVtableName()), pointer_at_field);
    // pointer_at_field = env->builder.CreateStructGEP(struct_type, call_instruction, index++);
    // env->builder.CreateStore(call_instruction, pointer_at_field);

    env->set_inst(call_instruction);


    CgenNode *current = this;
    std::stack<CgenNode *> inheritance_stack;
    while (current) {
        inheritance_stack.emplace(current);
        current = current->getParentnd();
    }

    while (!inheritance_stack.empty()) {
        current  = inheritance_stack.top();
        env->set_par(current);
        llvm::StructType *current_struct_type = env->getClass()->get_struct_type();

        for (auto const &feature : current->features) {
            Value *r_val = feature->code(env);
            if (r_val and !isa<Function>(r_val)) {
                Value *ptr_at_Field = env->builder.CreateStructGEP(current_struct_type,
                                        call_instruction,
                                        index);
                env->builder.CreateStore(r_val, ptr_at_Field);
                index++;
            } // 
            //   
        }

        inheritance_stack.pop();
    }

    env->builder.CreateRet(call_instruction);
}

/*********************************************************************

  APS class methods

    Fill in the following methods to produce code for the
    appropriate expression. You may add or remove parameters
    as you wish, but if you do, remember to change the parameters
    of the declarations in `cool-tree.handcode.h'.

    Look for the various TODO comments

*********************************************************************/

Value *attr_class::code(CgenEnvironment *env) {
    // TODO: Add code here for emitting initializing an attribute
    // return this->init->code(env);
    Value *ret_val = this->init->code(env);

    auto [index, type_of_attribute] = env->getClass()->get_type_of_attribute(name->get_string());

    if (!ret_val and isa<PointerType>(type_of_attribute)) {
        ret_val = ConstantPointerNull::get(dyn_cast<PointerType>(type_of_attribute));
    } else if (!ret_val and isa<IntegerType>(type_of_attribute)) {
        ret_val = ConstantInt::get(type_of_attribute, 0);
    }

    return ret_val;
}

// Create a method body
Function *method_class::code(CgenEnvironment *env) {
    if (cgen_debug) {
        errs() << "method" << "\n";
    }

    // Chen-Tao:
    // auto full_method_name = env->getClass()->getTypeName() + "_" + this->name->get_string();
    auto full_method_name = env->getClass()->getFullMethodName(this->name->get_string());
//     std::string getFullMethodName(std::string method_name) const {
//     return getTypeName() + "_" + method_name;
//   }
    Function *method_func = env->theModule.getFunction(full_method_name);
    if (method_func == nullptr or expr->no_code()) {
        errs() << "method boris" << "\n";
      return method_func;
    }

    if (env->check_if_inherited())
      return nullptr;

    Type *ret_type = method_func->getReturnType();

    BasicBlock *original_basic_block = env->builder.GetInsertBlock();
    BasicBlock *BB = BasicBlock::Create(env->context, "method_entry", method_func);
    env->builder.SetInsertPoint(BB);

    env->openScope();
    auto meth_iterator = method_func->arg_begin() + 1; 
    for (auto const &formal: formals) {
      Value *Inst = env->builder.CreateAlloca(meth_iterator->getType());
      env->builder.CreateStore(meth_iterator, Inst);
      env->addBinding(formal->get_name(), Inst, meth_iterator->getType());
      meth_iterator++;
    }

    Value *ret_val = expr->code(env);
    if (!ret_val and isa<PointerType>(ret_type))
      ret_val = ConstantPointerNull::get(dyn_cast<PointerType>(ret_type));

    if (ret_val->getType() != ret_type)
      ret_val = env->Boxing(ret_val);
    env->builder.CreateRet(ret_val);
    env->closeScope();


    env->builder.SetInsertPoint(original_basic_block);
    return method_func;

    // TODO: for CP1, you should be able to copy your code from Lab 2, but you
    // will need
    //       to grab the self pointer from the first argument of the function. See
    //       hints below.
    // TODO: For CP2, you will need to add code here to generate any method body,
    // not just Main_main.
    //       In P2, instead of defining the function here, you will have defined
    //       it in layout_feature().

    // Function *curr_meth = env->getFunction();

    // env->openScope();

    // Argument *self_arg = curr_meth->arg_begin();
    // if (self_arg != nullptr) env->addBinding(self, self_arg);  // Add self obj to scope

    // // Add bindings for parameters as local variables here

    // Value *ret_val = this->expr->code(env);  // Code method body
    // if (ret_val == nullptr) {
    //     Type *ret_ty = curr_meth->getReturnType();
    //     if (ret_ty == env->i32)
    //         ret_val = ConstantInt::get(env->i32, 0);
    //     else if (ret_ty == env->i1)
    //         ret_val = ConstantInt::get(env->i1, 0);
    //     else if (ret_ty == env->ptr)
    //         ret_val = self_arg;
    // }

    // env->builder.CreateRet(ret_val);  // create return value

    // env->closeScope();

    // HINT: Recall that you can and should use helper functions from the include
    // files
    //       and especially CgenEnvironment.h
    // HINT: Recall that every LLVM procedure needs and entry basic block
    // HINT: Look at the IRBuilder tutorial and/or documentation for help
    // HINT: The names of the class and method are available as a Symbol in the
    //       class (through the environment) and the method (using the "this"
    //       pointer). You can turn symbols to strings with ->get_string().

    // HINT: You may find Function* func->arg_begin() useful to iterate over
    // arguments,
    //       but you may not need it depending on how you chose to implement
    //       things overall.

    // return curr_meth;
}

// Expression to create a new object
Value *new__class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "newClass" << "\n";
    // TODO: add code here and replace `return nullptr`

    // HINT: This is where you will need to allocate memory for the object
    //       and call the constructor. You can choose to allocate memory here
    //       or in the constructor. Coolrt does it in the constructor.

    // return env->getDefaultInit(this->get_type());
    CgenClassTable *cls_table = &env->classTable;
    auto Init = type_name->get_string() + "_new";
    PointerType *ptr_type = PointerType::get(env->context, 0);
    FunctionType *func_type = FunctionType::get(ptr_type, false);

    
    if (type_name == Int) {
        return ConstantInt::get(cls_table->i32, 0);
    } else if (type_name == Bool) {
        return ConstantInt::get(cls_table->i1, 0);
    } else {
        Value *r_val = env->builder.CreateCall(env->theModule.getOrInsertFunction(Init, func_type));
        return r_val;
    }
}

Value *int_const_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "Integer Constant" << "\n";

    long long int_val = std::stoll(this->token->get_string());

    return ConstantInt::get(env->i32, (int32_t)int_val);
}

Value *bool_const_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "Boolean Constant" << "\n";

    return ConstantInt::get(env->i1, (this->val) ? 1 : 0);
}

static Value *compare_helper_func_string(CgenEnvironment *env, Value *val) {
  Value *Alloc_Inst = env->builder.CreateAlloca(env->classTable.ptr);

  Value *is_null = env->builder.CreateIsNull(val);
  BasicBlock *bb_is_null = env->newBbAtFend("is_null");
  BasicBlock *bb = env->newBbAtFend("checking_if_string_start");
  BasicBlock *bb_it_is_string = env->newBbAtFend("it_is_string");
  BasicBlock *bb_it_is_not_string = env->newBbAtFend("it_is_not_string");
  BasicBlock *bb_string_checked = env->newBbAtFend("checking_if_string_end");

  env->builder.CreateCondBr(is_null, bb_is_null, bb);
  env->builder.SetInsertPoint(bb_is_null);
  env->builder.CreateBr(bb_it_is_not_string);
  env->builder.SetInsertPoint(bb);

  StructType *object_struct_type = StructType::getTypeByName(env->context, "String");
  Value *object_vtable_addr = env->builder.CreateStructGEP(object_struct_type, val, 0);
  Value *object_vtable = env->builder.CreateLoad(env->classTable.ptr, object_vtable_addr, "obj_vt");
  Value *object_vtable_tag = env->builder.CreateLoad(env->classTable.i32, object_vtable, "tag");

  Value *Condition = env->builder.CreateICmpEQ(object_vtable_tag, ConstantInt::get(env->classTable.i32, 3));

  env->builder.CreateCondBr(Condition, bb_it_is_string, bb_it_is_not_string);
  env->builder.SetInsertPoint(bb_it_is_string);
  Value *string_obj_address = env->builder.CreateStructGEP(object_struct_type, val, 1);
  Value *string_value = env->builder.CreateLoad(env->classTable.ptr, string_obj_address);
  env->builder.CreateStore(string_value, Alloc_Inst);
  env->builder.CreateBr(bb_string_checked);

  env->builder.SetInsertPoint(bb_it_is_not_string);
  env->builder.CreateStore(val, Alloc_Inst);
  env->builder.CreateBr(bb_string_checked);

  env->builder.SetInsertPoint(bb_string_checked);
  return env->builder.CreateLoad(env->classTable.ptr, Alloc_Inst);
}

Value *plus_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "plus" << "\n";

    Value *lhs = e1->code(env);
    Value *rhs = e2->code(env);
    if (!lhs || !rhs) return nullptr;

    return env->builder.CreateAdd(lhs, rhs, "addtmp");
}

Value *sub_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "sub" << "\n";

    Value *lhs = e1->code(env);
    Value *rhs = e2->code(env);
    if (!lhs || !rhs) return nullptr;

    return env->builder.CreateSub(lhs, rhs, "subtmp");
}

Value *mul_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "mul" << "\n";

    Value *lhs = e1->code(env);
    Value *rhs = e2->code(env);
    if (!lhs || !rhs) return nullptr;

    return env->builder.CreateMul(lhs, rhs, "multmp");
}

Value *divide_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "div" << "\n";

    Value *lhs = e1->code(env);
    Value *rhs = e2->code(env);
    if (!lhs || !rhs) return nullptr;

    return env->builder.CreateSDiv(lhs, rhs, "divtmp");
}

Value *neg_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "neg_class::code" << "\n";

    Value *operand = e1->code(env);
    if (!operand) return nullptr;

    return env->builder.CreateNeg(operand, "negtmp");
}

Value *comp_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "comp_class::code" << "\n";

    Value *operand = e1->code(env);
    if (!operand)
        return nullptr;

    return env->builder.CreateNot(operand, "comptmp");
}

Value *lt_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "lt" << "\n";

    Value *L = e1->code(env);
    Value *R = e2->code(env);
    return env->builder.CreateCmp(llvm::CmpInst::ICMP_SLT, L, R);
}

Value *eq_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "eq" << "\n";

    Value *L = e1->code(env);
    Value *R = e2->code(env);

    if (L->getType() == env->classTable.ptr) {
        L = compare_helper_func_string(env, L);
    }
    if (R->getType() == env->classTable.ptr) {
        R = compare_helper_func_string(env, R);
    } 
    return env->builder.CreateICmpEQ(L, R);
}

Value *leq_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "leq" << "\n";

    Value *L = e1->code(env);
    Value *R = e2->code(env);
    return env->builder.CreateCmp(llvm::CmpInst::ICMP_SLE, L, R);
}

Value *block_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "block" << "\n";

    Value *last_val = nullptr;
    for (int i = this->body->first(); this->body->more(i);
         i = this->body->next(i)) {
        last_val = this->body->nth(i)->code(env);
    }
    return last_val;
}

Value *let_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "let" << "\n";
    // Jer:
    // Value *init_val = init_val = this->init->code(env);
    // if (init_val == nullptr) {  // returns nullptr bc init was no_exp()
    //     init_val = env->getDefaultInit(this->type_decl);
    // }

    // if (init_val == nullptr) {
    //     errs() << "OH THE HORROR" << "\n";
    // }

    // Type *this_type = env->classTable.get_llvm_type_from_symbol(this->type_decl);

    // env->openScope();

    // Value *val_ptr = env->builder.CreateAlloca(this_type);
    // env->builder.CreateStore(init_val, val_ptr);

    // env->addBinding(this->identifier, val_ptr);

    // Value *body_ret_val = this->body->code(env);

    // env->closeScope();

    // return body_ret_val;

    Value *init_val = this->init->code(env);
    Type *Typ;

    if (this->type_decl == Int or this->type_decl == prim_int) {
      Typ = Type::getInt32Ty(env->context);
      if (init_val == nullptr) {
        init_val = ConstantInt::get(env->classTable.i32, 0);
      }
    } else if (this->type_decl == Bool or this->type_decl == prim_bool) {
      Typ = Type::getInt1Ty(env->context);
      if (init_val == nullptr) {
        init_val = ConstantInt::get(env->classTable.i1, 0);
      }
    } else {
      Typ = PointerType::get(env->context, 0);
      if (init_val == nullptr) {
        init_val = ConstantPointerNull::get(dyn_cast<PointerType>(Typ));
      }
    }

    BasicBlock *let_basic_block = env->newBbAtFend("let");
    env->builder.CreateBr(let_basic_block);
    env->builder.SetInsertPoint(let_basic_block);
    Value *alloc_inst = env->builder.CreateAlloca(Typ);
    env->builder.CreateStore(init_val, alloc_inst);

    env->openScope();
    env->addBinding(this->identifier, alloc_inst, Typ);
    Value *ret_val = this->body->code(env);
    env->closeScope();

    return ret_val;
}

Value *assign_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "assign" << "\n";

    // TODO: add code here and replace `return nullptr`
    // Copy this from your Lab 2, but update for objects
    // HINT: You will need to use builder.CreateStructGEP()
    //       for attributes somehow.

    // Jer:
    // Value *new_val = this->expr->code(env);

    // Value *obj_ptr = env->findInScopes(this->name);

    // if (obj_ptr != nullptr) {
    //     env->builder.CreateStore(new_val, obj_ptr);
    //     return new_val;
    // }

    // CgenNode *cur_class = env->getClass();

    // Value *self_ptr = env->findInScopes(self);

    // auto [_, item_ptr] = env->get_attr_ptr(cur_class, this->name->get_string(), self_ptr);

    // env->builder.CreateStore(new_val, item_ptr);

    // return new_val;

    Value *expression_val = this->expr->code(env);
    auto [obj_type, expr_ptr] = env->findInScopes(this->name);
    if (expr_ptr != nullptr) {
      env->builder.CreateStore(expression_val, expr_ptr);
    } else {  
      CgenNode *cur_class = env->getClass();
      auto [index, type_of_attr] = cur_class->get_type_of_attribute(this->name->get_string());
      Value *self = env->builder.GetInsertBlock()->getParent()->arg_begin();
      Value *expr_ptr_ = env->builder.CreateStructGEP(cur_class->get_struct_type(), self, index);
      env->builder.CreateStore(expression_val, expr_ptr_);
    }

    return expression_val;

}

Value *cond_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "cond" << "\n";

    // TODO: Copy this from your Lab 2 to start, but update to handle
    //       cases where the branches are not the same type.
    // HINT: This is where boxing might happen.
    // HINT: Unboxing can only happen with a case expression

    // Jer:
    // Value *cond = this->pred->code(env);

    // if (!cond) {
    //     return nullptr;
    // }

    // BasicBlock *prev_BB = env->builder.GetInsertBlock();

    // BasicBlock *then_BB = BasicBlock::Create(env->context, "then_BB", env->getFunction());
    // env->builder.SetInsertPoint(then_BB);
    // Value *then_ret = this->then_exp->code(env);
    // BasicBlock *finish_then_BB = env->builder.GetInsertBlock();

    // BasicBlock *else_BB = BasicBlock::Create(env->context, "else_BB", env->getFunction());
    // env->builder.SetInsertPoint(else_BB);
    // Value *else_ret = this->else_exp->code(env);
    // BasicBlock *finish_else_BB = env->builder.GetInsertBlock();

    // BasicBlock *resolve_BB = BasicBlock::Create(env->context, "merge_BB", env->getFunction());

    // Type *then_ty = env->classTable.get_llvm_type_from_symbol(this->then_exp->get_type());
    // Type *else_ty = env->classTable.get_llvm_type_from_symbol(this->else_exp->get_type());

    // env->builder.SetInsertPoint(prev_BB);
    // env->builder.CreateCondBr(cond, then_BB, else_BB);

    // env->builder.SetInsertPoint(finish_then_BB);
    // env->builder.CreateBr(resolve_BB);

    // env->builder.SetInsertPoint(finish_else_BB);
    // env->builder.CreateBr(resolve_BB);

    // // Waahhh Wahhh, we are doing cp1 so we forget about finding common obj type

    // env->builder.SetInsertPoint(resolve_BB);
    // PHINode *phi = env->builder.CreatePHI(then_ty, 2);
    // phi->addIncoming(then_ret, finish_then_BB);
    // phi->addIncoming(else_ret, finish_else_BB);

    // return phi;

    Value* cond = pred->code(env);
    if (!cond) {
        return nullptr;
    }
    Value *alloc_val = nullptr;
    Type *alloc_ty = nullptr;

    // Create if block
    BasicBlock* if_bb = env->newBbAtFend("if");
    // Jump to if block
    env->builder.CreateBr(if_bb);
    // Create then, else and merge blocks
    BasicBlock* then_bb = env->newBbAtFend("then");
    BasicBlock* else_bb = env->newBbAtFend("else");
    BasicBlock* merge_bb = env->newBbAtFend("merge");

    // Get thier types
    Value *Then = then_exp->code(env);
    Type *then_ty = Then->getType();
    Value *Else = else_exp->code(env);
    Type *else_ty = Else->getType();
    
    // Chen-Tao: for Lab 3, we need to handle the case of different types in condition branches of cond
    if (isa<IntegerType>(then_ty) && isa<IntegerType>(else_ty)) {
      unsigned then_exp_bit_width = cast<IntegerType>(then_ty)->getBitWidth();
      unsigned else_exp_bit_width = cast<IntegerType>(else_ty)->getBitWidth();

      if (then_exp_bit_width != else_exp_bit_width) {
        alloc_ty = env->classTable.i32;
      } else {
        if (then_exp_bit_width == 1) {
          alloc_ty = env->classTable.i1;
        } else {
          alloc_ty = env->classTable.i32;
        }
      }
    } else {
      alloc_ty = PointerType::get(env->context, 0);
    }

    // Set insertion point to if block, allocate memory and jump to then or else block
    env->builder.SetInsertPoint(if_bb);
    alloc_val = env->builder.CreateAlloca(alloc_ty);
    env->builder.CreateCondBr(cond, then_bb, else_bb);

    // then block
    env->builder.SetInsertPoint(then_bb);
    Value* then_block_ret_val = then_exp->code(env);
    env->builder.CreateStore(then_block_ret_val, alloc_val);
    env->builder.CreateBr(merge_bb);

    // else block
    env->builder.SetInsertPoint(else_bb);
    Value* else_block_ret_val = else_exp->code(env);
    env->builder.CreateStore(else_block_ret_val, alloc_val);
    env->builder.CreateBr(merge_bb);

    // merge block
    env->builder.SetInsertPoint(merge_bb);
    // Finally, load the result and return it
    Value* phi_func = env->builder.CreateLoad(alloc_ty, alloc_val, "iftmp");
    return phi_func;
}

Value *loop_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "loop" << "\n";

    BasicBlock *loop_pred = env->newBbAtFend("loop_pred");
    BasicBlock *loop_body = env->newBbAtFend("loop_body");
    BasicBlock *finish_body = env->newBbAtFend("loop_finish");

    env->builder.CreateBr(loop_pred);  // Add branch to previous bb we were in

    env->builder.SetInsertPoint(loop_pred);  // Write to the loop predicate block
    Value *cond_ret_val = this->pred->code(env);
    env->builder.CreateCondBr(cond_ret_val, loop_body, finish_body);

    env->builder.SetInsertPoint(loop_body);  // Write to body block
    Value *body = this->body->code(env);
    env->builder.CreateBr(loop_pred);

    env->builder.SetInsertPoint(finish_body);  // Continue outputing code
    return body;
}

Value *object_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << this->name->get_string() << " Object" << "\n";

    // TODO: Copy this from your Lab 2, but you will need to update
    //       for real objects and for attributes.
    // HINT: There are no nested attributes and attributes cannot be
    //       overridden in Cool.
    // HINT: You will need to use builder.CreateStructGEP()
    
    //Jer:
    // Type *obj_type = env->classTable.get_llvm_type_from_symbol(this->type);

    // Value *obj_ptr = env->findInScopes(this->name);

    // if (obj_ptr != nullptr) {
    //     return env->builder.CreateLoad(obj_type, obj_ptr);
    // }

    // CgenNode *cur_class = env->getClass();

    // Value *self_ptr = env->findInScopes(self);

    // auto [item_ty, item_ptr] = env->get_attr_ptr(cur_class, this->name->get_string(), self_ptr);
    // errs() << "Name: " << this->name->get_string() << "\tInt: " << (item_ty == env->i32) << "\tBool: " << (item_ty == env->i1) << "\tptr: " << (item_ty == env->ptr) << "\n";
    // errs() << item_ptr << "\n";
    // return env->builder.CreateLoad(item_ty, item_ptr);

    auto [obj_type, object_ptr] = env->findInScopes(this->name);
    if (obj_type != nullptr and object_ptr != nullptr) {
      return env->builder.CreateLoad(obj_type, object_ptr);
    } else if (obj_type != nullptr and isa<IntegerType>(obj_type)) {
      return ConstantInt::get(obj_type, 0);
    } else if (obj_type != nullptr and isa<PointerType>(obj_type)) {
      return ConstantPointerNull::get(dyn_cast<PointerType>(obj_type));
    }

    // 特殊處理 self
    if (this->name == self) {
        return env->get_inst();
    }

    CgenNode *cur_class = env->getClass();
    auto [index, type_of_attr] = cur_class->get_type_of_attribute(this->name->get_string());
    
    // 檢查是否找到屬性
    if (type_of_attr == nullptr) {
        // 屬性不存在，返回適當的預設值
        if (cgen_debug)
            errs() << "Warning: Attribute '" << this->name->get_string() << "' not found in class\n";
        return ConstantPointerNull::get(PointerType::get(env->classTable.context, 0));
    }
    
    Value *self_ptr = env->get_inst();
    Value *attrPtr = env->builder.CreateStructGEP(cur_class->get_struct_type(), self_ptr, index);
    return env->builder.CreateLoad(type_of_attr, attrPtr);
}

Value *no_expr_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "No_expr" << "\n";

    // HINT: Already implemented
    // HINT: Check the methods of this class
    return nullptr;
}

//*****************************************************************
// The next few functions are for node types that were not supported
// in Lab 2 but that you now need to implement in Lab 3.
//*****************************************************************

Value *isvoid_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "isvoid" << "\n";
    // TODO: add code here and replace `return nullptr`
    //       This is for the dynamic test of an object being void
    // TODO: CP2 -- not needed in CP1
    // HINT: Consider builder.CreateIsNull()
    // return nullptr;
    Value *r_val = e1->code(env);
    // Handle boxing
    if (r_val->getType() == env->classTable.i32 or r_val->getType() == env->classTable.i1) {
        r_val = env->Boxing(r_val);
    }
    return env->builder.CreateIsNull(r_val);
}

Value *dispatch_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "dispatch" << "\n";
    // TODO: add code here and replace `return nullptr`
    // TODO: CP2 -- not needed in CP1

    // HINT: Cool actually passes objects by reference and Ints and Bools
    //       by unboxed value -- this is what LLVM already does for calls.
    // HINT: Don't forget about vtables.
    // HINT: Add a dynamic check for void dispatch (i.e., on this->expr).
    // HINT: To use a function pointer, you will need to:
    //       1. Get the value of the function pointer
    //       2. Create the type for the function using llvm::FunctionType::get()
    //       3. Create the call: builder.CreateCall(funcTy, funcPtr, formals);
    // HINT: The return type of the method is "type"
    // HINT: The method name is "name"
    // HINT: The method arguments are "actual" (and it's a list)
    // HINT: The method is called on "expr"
    // HINT: You may find the following helpful (but you can get by with it):
    //       llvm::StructType::getTypeByName()

    // return nullptr;
    CgenClassTable *table = &env->classTable;
    auto method_name = name->get_string();
    Value *Condition_ = (new isvoid_class(expr))->code(env);
    BasicBlock *abort_BB = env->newBbAtFend("abort_basic_block");
    BasicBlock *dispatch_BB = env->newBbAtFend("dispatch_block");
    env->builder.CreateCondBr(Condition_, abort_BB, dispatch_BB);
    env->builder.SetInsertPoint(abort_BB);
    env->builder.CreateCall(env->theModule.getFunction("abort"));
    env->builder.CreateBr(dispatch_BB);
    env->builder.SetInsertPoint(dispatch_BB);


    Value *obj = expr->code(env);
    auto obj_type = obj->getType();
    // Need to handle boxing
    if (obj_type == table->i32 or obj_type == table->i1) {
        obj = env->Boxing(obj);
    }

    CgenNode *obj_cls_node = env->typeToClass(expr->get_type());

    Function *func;
    while (obj_cls_node != nullptr) {
      GlobalVariable *vtable_global = env->theModule.getNamedGlobal(obj_cls_node->getVtableName());
      Constant *vtable_init = vtable_global->getInitializer();

      for (int i = 3; i < vtable_init->getNumOperands(); i++) {
        Value *field = vtable_init->getOperand(i);
        auto name_of_field = field->getName();
        auto pos_in_name = name_of_field.find("_");
        auto name_of_field_substring = name_of_field.substr(pos_in_name + 1, name_of_field.size());

        if (name_of_field_substring == method_name) {
            func = env->theModule.getFunction(name_of_field);
            break;
        }
      }
      if (func != nullptr) {
        break;
      }
      obj_cls_node = obj_cls_node->getParentnd();
    }

    
    llvm::SmallVector<Value *> list_of_arguments_vals;
    llvm::SmallVector<Type *> list_of_arguments_types;
    list_of_arguments_vals.emplace_back(obj);
    list_of_arguments_types.emplace_back(obj->getType());

    for (auto const &a: actual) {
      Value *r_val = a->code(env);
      list_of_arguments_vals.emplace_back(r_val);
      list_of_arguments_types.emplace_back(r_val->getType());
    }
    Type *ret_type = table->get_llvm_type_from_symbol(type);
    Value *ret_val = env->builder.CreateCall(func, list_of_arguments_vals);

    
    if (ret_type != ret_val->getType()) {
      StructType *ret_val_of_struct_type = StructType::getTypeByName(env->context, "Int");
      Value *addr_ptr = env->builder.CreateStructGEP(ret_val_of_struct_type, ret_val, 1);
      ret_val = env->builder.CreateLoad(ret_type, addr_ptr);
    }

    return ret_val;
}

Value *static_dispatch_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "static dispatch" << "\n";
    // TODO: add code here and replace `return nullptr`
    // TODO: CP2 -- not needed in CP1

    // HINT: Cool actually passes objects by reference and Ints and Bools
    //       by unboxed value -- this is what LLVM already does for calls.
    // HINT: Don't forget about vtables.
    // HINT: Add a dynamic check for void dispatch (i.e., on this->expr).

    // return nullptr;
    CgenClassTable *table = &env->classTable;

    Value *Condition_ = (new isvoid_class(expr))->code(env);
    BasicBlock *abort_BB = env->newBbAtFend("abort_basic_block");
    BasicBlock *dispatch_BB = env->newBbAtFend("dispatch_block");
    env->builder.CreateCondBr(Condition_, abort_BB, dispatch_BB);
    env->builder.SetInsertPoint(abort_BB);
    env->builder.CreateCall(env->theModule.getFunction("abort"));
    env->builder.CreateBr(dispatch_BB);
    env->builder.SetInsertPoint(dispatch_BB);
    Value *obj = expr->code(env);
    auto obj_type = obj->getType();

    // Need to handle boxing
    if (obj_type == table->i32 or obj_type == table->i1) {
        obj = env->Boxing(obj);
    }
    

    GlobalVariable *vtable_global_var = env->theModule.getNamedGlobal("_" +
							type_name->get_string() +
							"_vtable_prototype");
    Constant *vtable_init = vtable_global_var->getInitializer();
    auto method_name = name->get_string();
    Type *ret_type;
    Function *func;
    for (int i = 3; i < vtable_init->getNumOperands(); i++) {
      Value *field = vtable_init->getOperand(i);
      auto name_of_field = field->getName();
      auto pos_in_name = name_of_field.find("_");
      auto name_of_field_substring = name_of_field.substr(pos_in_name + 1, name_of_field.size());
      if (name_of_field_substring == method_name) {
        ret_type = field->getType();
        func = env->theModule.getFunction(name_of_field);
        break;
      }
    }

    if (func == nullptr)
      return nullptr;

    llvm::SmallVector<Value *> list_of_arguments_vals;
    list_of_arguments_vals.emplace_back(obj);
    for (auto const &a: actual) {
      Value *r_val = a->code(env);
      list_of_arguments_vals.emplace_back(r_val);
    }

    Type *ret_type_real = table->get_llvm_type_from_symbol(type);
    Value *final_ret_val = env->builder.CreateCall(func, list_of_arguments_vals);
    
    if (ret_type_real != final_ret_val->getType()) {
      StructType *ret_val_of_struct_type = StructType::getTypeByName(env->context, "Int");
      Value *addr_ptr = env->builder.CreateStructGEP(ret_val_of_struct_type, final_ret_val, 1);
      final_ret_val = env->builder.CreateLoad(ret_type_real, addr_ptr);
    }
    return final_ret_val;
}

Value *string_const_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "string_const" << "\n";
    // TODO: add code here and replace `return nullptr`
    // TODO: CP2 -- not needed in CP1

    // HINT: The easiest way to handle this is to just box a char
    //       string every time. You can also choose to box the string
    //       just once, but that is more complicated.
    // HINT: The coolrt runtime library implements String_new().

    // return nullptr;
    Value *global_string = env->theModule.getNamedGlobal(token->get_string());

    return env->Boxing(global_string);
}

// Handle a Cool case expression (selecting based on the type of an object)
Value *typcase_class::code(CgenEnvironment *env) {
    if (cgen_debug)
        errs() << "typecase::code()" << "\n";

    // TODO: add code here and replace `return nullptr`
    // TODO: CP3 -- not needed in CP1 or CP2

    return nullptr;
}

// Handle one branch of a Cool case expression.
// If the source tag is >= the branch tag
// and <= (max child of the branch class) tag,
// then the branch is a superclass of the source.
// See the Lab 3 handout for more information about our use of class tags.
Value *branch_class::code(Value *expr_val, Value *tag, Type *join_type,
                          CgenEnvironment *env) {
    // TODO: add code here and replace `return nullptr`
    // TODO: CP3 -- not needed in CP1 or CP2

    return nullptr;
}
