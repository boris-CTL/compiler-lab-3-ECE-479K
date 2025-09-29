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

  // Define a function main that has no parameters and returns an i32
  // Copy from Lab 2

  // Define an entry basic block
  // Copy from Lab 2

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
  // TODO: In CP1, you will only need to layout the attributes
  // TODO: For CP1, include methods as well
  // HINT: Remember that you are not *generating* any code here
  //       e.g. functions should only be `declare`'d
  // HINT: Don't forget about inheritance!
}

// Assign this attribute a slot in the class structure
void attr_class::layout_feature(CgenNode *cls) {
  // TODO: add code here to add an attribute to the layout
  // HINT: Consider inheritance, byt remember that new definitions
  //       of an argument ID are added, leaving the previous ones
  //       alone to correctly and easily deal with inheritance.
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
}

// TODO: Use information from feature layout to create the class type.
//       Make sure to use get_type_name() for the name of the class type
// Ignore padding
StructType *CgenNode::getType() const {
  if (body) {
    assert(body->isSized());
    return body;
  }
  // TODO: initialize the class type
  assert(false && "todo");
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
  return nullptr;
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

  return nullptr;
}

// Expression to create a new object
Value *new__class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "newClass" << "\n";
  // TODO: add code here and replace `return nullptr`

  // HINT: This is where you will need to allocate memory for the object
  //       and call the constructor. You can choose to allocate memory here
  //       or in the constructor. Coolrt does it in the constructor.
  return nullptr;
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

  // TODO: Copy this from your Lab 2
  return nullptr;
}

Value *bool_const_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "Boolean Constant" << "\n";

  // TODO: Copy this from your Lab 2
  return nullptr;
}

Value *plus_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "plus" << "\n";

  // TODO: Copy from Lab 2
  return nullptr;
}

Value *sub_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "sub" << "\n";

  // TODO: Copy from Lab 2
  return nullptr;
}

Value *mul_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "mul" << "\n";

  // TODO: Copy this from Lab 2
  return nullptr;
}

Value *divide_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "div" << "\n";

  // TODO: Copy this from your Lab 2
  return nullptr;
}

Value *neg_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "neg" << "\n";

  // TODO: Copy this from your Lab 2
  return nullptr;
}

Value *comp_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "complement" << "\n";

  // TODO: Copy this from your Lab 2
  return nullptr;
}

Value *lt_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "lt" << "\n";

  // TODO: Copy this from your Lab 2
  return nullptr;
}

Value *eq_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "eq" << "\n";

  // TODO: Copy this from your Lab 2
  return nullptr;
}

Value *leq_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "leq" << "\n";

  // TODO: Copy this from your Lab 2
  return nullptr;
}

Value *block_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "block" << "\n";

  // TODO: Copy this from your Lab 2
  return nullptr;
}

Value *let_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "let" << "\n";

  // TODO: Start by copying from Lab 2, but this will
  //       need to be updated for objects
  // NOTE: While the operational semantics suggest that objects are copied
  //       we will instead just assign the pointer to the object

  return nullptr;
}

Value *assign_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "assign" << "\n";

  // TODO: add code here and replace `return nullptr`
  // Copy this from your Lab 2, but update for objects
  // HINT: You will need to use builder.CreateStructGEP()
  //       for attributes somehow.
  return nullptr;
}

Value *cond_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "cond" << "\n";

  // TODO: Copy this from your Lab 2 to start, but update to handle
  //       cases where the branches are not the same type.
  // HINT: This is where boxing might happen.
  // HINT: Unboxing can only happen with a case expression

  return nullptr;
}

Value *loop_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    errs() << "loop" << "\n";

  // TODO: Copy this from your Lab 2
  return nullptr;
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
