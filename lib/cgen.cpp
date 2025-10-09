/*********************************************************************
 Intermediate code generator for COOL: SKELETON

 Read the comments carefully and add code to build an LLVM program
*********************************************************************/
#include <cstddef>
#include <string>

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

  CgenNode *main_cls = this->getMainMain(this->root());
  Function *main_init_func = theModule.getFunction(main_cls->getInitFunctionName());

  Value *main_obj = builder.CreateCall(main_init_func, {});

  Function *main_main_func = theModule.getFunction(main_cls->getFullMethodName("main"));

  Value *main_main_ret_val = builder.CreateCall(main_main_func, {main_obj});

  Constant *main_main_ret_string_fmt = builder.CreateGlobalString("Main.main() returned %d\n", ".str");
  Function *printf_fn = theModule.getFunction("printf");
  assert(printf_fn && "printf function not found");
  builder.CreateCall(printf_fn, {main_main_ret_string_fmt, main_main_ret_val});

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
  assert(this->name->get_string() == "Main");
  for (auto *f : features) {
    if (f->get_name()->get_string() == "main") {
      f->code(new CgenEnvironment(this));
    }
  }
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

  this->getType();

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

  for(auto feature : this->features){ // Layout features of this class
    feature->layout_feature(this);
  }

  for(auto feature : this->parentnd->features){ // Layout features of parent class this class inherits from
    feature->layout_feature(this);
  }
}

// Assign this attribute a slot in the class structure
void attr_class::layout_feature(CgenNode *cls) {
  // TODO: add code here to add an attribute to the layout
  // HINT: Consider inheritance, byt remember that new definitions
  //       of an argument ID are added, leaving the previous ones
  //       alone to correctly and easily deal with inheritance
  cls->insert_attribute(this->type_decl, this);
}

// Create the LLVM Function corresponding to this method.
void method_class::layout_feature(CgenNode *cls) {
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

  auto [meth_name, meth] = cls->insert_method(this->get_name()->get_string(), this);
  Type *ret_type = cls->getClasstable().get_llvm_type_from_symbol(this->return_type);
  std::vector<Type *> formal_types;
  formal_types.push_back(cls->getClasstable().ptr); // self obj
  for(auto formal : this->formals){
    formal_types.push_back(cls->getClasstable().get_llvm_type_from_symbol(formal->get_type_decl()));
  }

  cls->getClasstable().createLlvmFunction(meth_name, ret_type, formal_types, false);
}

StructType *CgenNode::getType() const {

  if (body) {
    assert(body->isSized());
    return body;
  }

  std::vector<Type *> attr_types;
  attr_types.push_back(this->getClasstable().ptr); // For Vtable
  for(int i = 1; i < this->attribute_layout.size(); i++){
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

  for(auto [method_name, method] : this->method_layout){
    method->code(env);
  }
}

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

  Function *init_func = env->createLlvmFunction(this->getInitFunctionName(), env->ptr, {});
  BasicBlock *entry_bb = BasicBlock::Create(env->context, "entry", init_func, nullptr);
  env->builder.SetInsertPoint(entry_bb);

  DataLayout DL = env->theModule.getDataLayout();
  Value *class_alloc_size = ConstantInt::get(env->i64, DL.getTypeAllocSize(body));
  Value *class_obj_ptr = env->builder.CreateCall(env->theModule.getFunction("malloc"), class_alloc_size, "alloc_classtype", nullptr);

  for(int i=0; i<attribute_layout.size(); i++){
    auto [attr_type, cool_attr_obj] = this->attribute_layout[i];
    Value *init_val = cool_attr_obj->code(env);
    Value *attr_ptr = env->builder.CreateStructGEP(this->getType(), class_obj_ptr, i+1);

    env->builder.CreateStore(init_val, attr_ptr);
  }

  env->builder.CreateRet(class_obj_ptr);
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
  return this->init->code(env);
}

// Create a method body
Function *method_class::code(CgenEnvironment *env) {
  if (cgen_debug) {
    errs() << "method" << "\n";
  }

  // TODO: for CP1, you should be able to copy your code from Lab 2, but you
  // will need
  //       to grab the self pointer from the first argument of the function. See
  //       hints below.
  // TODO: For CP2, you will need to add code here to generate any method body,
  // not just Main_main.
  //       In P2, instead of defining the function here, you will have defined
  //       it in layout_feature().

  Function *curr_meth = env->getFunction();

  env->openScope();

  Argument *self_arg = curr_meth->arg_begin();
  if(self_arg != nullptr) env->addBinding(self, self_arg); // Add self obj to scope

  // Add bindings for parameters as local variables here

  Value *ret_val = this->expr->code(env); // Code method body
  env->builder.CreateRet(ret_val); // create return value

  env->closeScope();
  
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

  return curr_meth;
}

// Expression to create a new object
Value *new__class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "newClass" << "\n";
  // TODO: add code here and replace `return nullptr`

  // HINT: This is where you will need to allocate memory for the object
  //       and call the constructor. You can choose to allocate memory here
  //       or in the constructor. Coolrt does it in the constructor.

  return env->getDefaultInit(this->get_type());
}

// Codegen for expressions. Note that each expression has a value.
// HINT: Think about what each of these generates code for and read
//       the Cool manual carefully and the longer CoolAid document
// HINT: If you type this->, the IDE will show you all the members
//       you have access to. The include files have them too, of course.
// TODO: Many of these are just copied over from Lab 2

Value *int_const_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "Integer Constant" << "\n";

    long long int_val = std::stoll(this->token->get_string());

    return ConstantInt::get(env->i32, (int32_t)int_val);
}

Value *bool_const_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "Boolean Constant" << "\n";

  return ConstantInt::get(env->i1, (this->val)? 1 : 0);
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
    return env->builder.CreateCmp(llvm::CmpInst::ICMP_EQ, L, R);
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

  Value *init_val = nullptr;
  if(this->init == no_expr()){
    init_val = env->getDefaultInit(this->type_decl);
  }

  Type *this_type = env->classTable.get_llvm_type_from_symbol(this->type_decl);

  env->openScope();

  Value *val_ptr = env->builder.CreateAlloca(this_type);

  env->addBinding(this->identifier, val_ptr);

  Value *body_ret_val = this->body->code(env);

  env->closeScope();

  return body_ret_val;
}

Value *assign_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "assign" << "\n";

  // TODO: add code here and replace `return nullptr`
  // Copy this from your Lab 2, but update for objects
  // HINT: You will need to use builder.CreateStructGEP()
  //       for attributes somehow.

  Value *new_val = this->expr->code(env);

  Value *obj_ptr = env->findInScopes(this->name);

  if(obj_ptr != nullptr){
    env->builder.CreateStore(new_val, obj_ptr);
    return new_val;
  }

  CgenNode *cur_class = env->getClass();

  Value *self_ptr = env->findInScopes(self);
  
  Value *item_ptr = env->get_attr_ptr(cur_class, this->name->get_string(), self_ptr);

  env->builder.CreateStore(new_val, item_ptr);

  return new_val;
}

Value *cond_class::code(CgenEnvironment *env) {
  if (cgen_debug)
        errs() << "cond" << "\n";

    // TODO: Copy this from your Lab 2 to start, but update to handle
    //       cases where the branches are not the same type.
    // HINT: This is where boxing might happen.
    // HINT: Unboxing can only happen with a case expression
    Value *cond = pred->code(env);
    if (!cond) {
        return nullptr;
    }
    Value *alloc_val = nullptr;
    Type *alloc_ty = nullptr;

    // Create if block
    BasicBlock *if_bb = env->newBbAtFend("if");
    // Jump to if block
    env->builder.CreateBr(if_bb);
    // Create then, else and merge blocks
    BasicBlock *then_bb = env->newBbAtFend("then");
    BasicBlock *else_bb = env->newBbAtFend("else");
    BasicBlock *merge_bb = env->newBbAtFend("merge");

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
    Value *then_block_ret_val = then_exp->code(env);
    env->builder.CreateStore(then_block_ret_val, alloc_val);
    env->builder.CreateBr(merge_bb);

    // else block
    env->builder.SetInsertPoint(else_bb);
    Value *else_block_ret_val = else_exp->code(env);
    env->builder.CreateStore(else_block_ret_val, alloc_val);
    env->builder.CreateBr(merge_bb);

    // merge block
    env->builder.SetInsertPoint(merge_bb);
    // Finally, load the result and return it
    Value *phi_func = env->builder.CreateLoad(alloc_ty, alloc_val, "iftmp");
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
    errs() << "Object" << "\n";

  // TODO: Copy this from your Lab 2, but you will need to update
  //       for real objects and for attributes.
  // HINT: There are no nested attributes and attributes cannot be
  //       overridden in Cool.
  // HINT: You will need to use builder.CreateStructGEP()

  return nullptr;
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
  return nullptr;
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

  return nullptr;
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

  return nullptr;
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

  return nullptr;
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
