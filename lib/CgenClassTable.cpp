#include <cstddef>
#include <iostream>
#include <llvm/Support/FileSystem.h>
#include <sstream>
#include <string>

#include "CgenClassTable.h"
#include "CgenEnvironment.h"
#include "CgenNode.h"
#include "stringtab.h"
#include "symbol.h"

extern int cgen_debug, curr_lineno;
using namespace llvm;

/*********************************************************************
 For convenience, a large number of symbols are predefined here.
 These symbols include the primitive type and method names, as well
 as fixed names used by the runtime system. Feel free to add your
 own definitions as you see fit.
*********************************************************************/
Symbol
    // required classes
    Object,
    IO, String, Int, Bool, Main,

    // class methods
    cool_abort, type_name, cool_copy, out_string, out_int, in_string, in_int,
    length, concat, substr,

    // class members
    val,

    // special symbols
    No_class,  // symbol that can't be the name of any user-defined class
    No_type,   // If e : No_type, then no code is generated for e.
    SELF_TYPE, // Special code is generated for new SELF_TYPE.
    self,      // self generates code differently than other references

    // extras
    arg, arg2, newobj, Mainmain, prim_string, prim_int, prim_bool;

// Initializing the predefined symbols.
static void initializeConstants(void) {
  Object = idtable.add_string("Object");
  IO = idtable.add_string("IO");
  String = idtable.add_string("String");
  Int = idtable.add_string("Int");
  Bool = idtable.add_string("Bool");
  Main = idtable.add_string("Main");

  cool_abort = idtable.add_string("abort");
  type_name = idtable.add_string("type_name");
  cool_copy = idtable.add_string("copy");
  out_string = idtable.add_string("out_string");
  out_int = idtable.add_string("out_int");
  in_string = idtable.add_string("in_string");
  in_int = idtable.add_string("in_int");
  length = idtable.add_string("length");
  ::concat = idtable.add_string("concat");
  substr = idtable.add_string("substr");

  val = idtable.add_string("val");

  No_class = idtable.add_string("_no_class");
  No_type = idtable.add_string("_no_type");
  SELF_TYPE = idtable.add_string("SELF_TYPE");
  self = idtable.add_string("self");

  arg = idtable.add_string("arg");
  arg2 = idtable.add_string("arg2");
  newobj = idtable.add_string("_newobj");
  Mainmain = idtable.add_string("main");
  prim_string = idtable.add_string("sbyte*");
  prim_int = idtable.add_string("int");
  prim_bool = idtable.add_string("bool");
}

/*********************************************************************

  CgenClassTable methods

*********************************************************************/

// CgenClassTable constructor orchestrates all code generation
CgenClassTable::CgenClassTable(Classes classes)
    : nds(), currentTag(0), context(), builder(this->context),
      theModule("module", this->context) {
  if (cgen_debug)
    errs() << "Building CgenClassTable" << "\n";
  // Make sure we have a scope, both for classes and for constants
  enterscope();

  // Create an inheritance tree with one CgenNode per class.
  installBasicClasses();
  installClasses(classes);
  buildInheritanceTree();

  // First pass
  setup();

  // Second pass
  // code gen
  codeModule();
  // Done with code generation: exit scopes
  exitscope();
}

// Creates AST nodes for the basic classes and installs them in the class list
void CgenClassTable::installBasicClasses() {
  // The tree package uses these globals to annotate the classes built below.
  curr_lineno = 0;
  Symbol filename = stringtable.add_string("<basic class>");

  //
  // A few special class names are installed in the lookup table but not
  // the class list. Thus, these classes exist, but are not part of the
  // inheritance hierarchy.

  // No_class serves as the parent of Object and the other special classes.
  Class_ noclasscls = class_(No_class, No_class, nil_Features(), filename);
  installSpecialClass(new CgenNode(noclasscls, CgenNode::Basic, this));
  delete noclasscls;

  // Lab 3 changes from Lab 2
  // SELF_TYPE is the self class; it cannot be redefined or inherited.
  Class_ selftypecls = class_(SELF_TYPE, No_class, nil_Features(), filename);
  installSpecialClass(new CgenNode(selftypecls, CgenNode::Basic, this));
  delete selftypecls;
  //
  // Primitive types masquerading as classes. This is done so we can
  // get the necessary Symbols for the innards of String, Int, and Bool
  //
  Class_ primstringcls =
      class_(prim_string, No_class, nil_Features(), filename);
  installSpecialClass(new CgenNode(primstringcls, CgenNode::Basic, this));
  delete primstringcls;
  // End of Lab 3 changes

  Class_ primintcls = class_(prim_int, No_class, nil_Features(), filename);
  installSpecialClass(new CgenNode(primintcls, CgenNode::Basic, this));
  delete primintcls;
  Class_ primboolcls = class_(prim_bool, No_class, nil_Features(), filename);
  installSpecialClass(new CgenNode(primboolcls, CgenNode::Basic, this));
  delete primboolcls;
  //
  // The Object class has no parent class. Its methods are
  //    cool_abort() : Object    aborts the program
  //    type_name() : Str        returns a string representation of class name
  //    copy() : SELF_TYPE       returns a copy of the object
  //
  // There is no need for method bodies in the basic classes---these
  // are already built in to the runtime system.
  //
  Class_ objcls = class_(
      Object, No_class,
      append_Features(
          append_Features(single_Features(method(cool_abort, nil_Formals(),
                                                 Object, no_expr())),
                          single_Features(method(type_name, nil_Formals(),
                                                 String, no_expr()))),
          single_Features(
              method(cool_copy, nil_Formals(), SELF_TYPE, no_expr()))),
      filename);
  installClass(new CgenNode(objcls, CgenNode::Basic, this));
  delete objcls;

  //
  // The Int class has no methods and only a single attribute, the
  // "val" for the integer.
  //
  Class_ intcls = class_(
      Int, Object, single_Features(attr(val, prim_int, no_expr())), filename);
  installClass(new CgenNode(intcls, CgenNode::Basic, this));
  delete intcls;

  //
  // Bool also has only the "val" slot.
  //
  Class_ boolcls = class_(
      Bool, Object, single_Features(attr(val, prim_bool, no_expr())), filename);
  installClass(new CgenNode(boolcls, CgenNode::Basic, this));
  delete boolcls;

  // Lab 3 changes from Lab 2
  //
  // The class String has a number of slots and operations:
  //       val                                  the string itself
  //       length() : Int                       length of the string
  //       concat(arg: Str) : Str               string concatenation
  //       substr(arg: Int, arg2: Int): Str     substring
  //
  Class_ stringcls =
      class_(String, Object,
             append_Features(
                 append_Features(
                     append_Features(
                         single_Features(attr(val, prim_string, no_expr())),
                         single_Features(
                             method(length, nil_Formals(), Int, no_expr()))),
                     single_Features(method(::concat,
                                            single_Formals(formal(arg, String)),
                                            String, no_expr()))),
                 single_Features(
                     method(substr,
                            append_Formals(single_Formals(formal(arg, Int)),
                                           single_Formals(formal(arg2, Int))),
                            String, no_expr()))),
             filename);
  installClass(new CgenNode(stringcls, CgenNode::Basic, this));
  delete stringcls;

  //
  // The IO class inherits from Object. Its methods are
  //        out_string(Str) : SELF_TYPE          writes a string to the output
  //        out_int(Int) : SELF_TYPE               "    an int    "  "     "
  //        in_string() : Str                    reads a string from the input
  //        in_int() : Int                         "   an int     "  "     "
  //
  Class_ iocls = class_(
      IO, Object,
      append_Features(
          append_Features(
              append_Features(
                  single_Features(method(out_string,
                                         single_Formals(formal(arg, String)),
                                         SELF_TYPE, no_expr())),
                  single_Features(method(out_int,
                                         single_Formals(formal(arg, Int)),
                                         SELF_TYPE, no_expr()))),
              single_Features(
                  method(in_string, nil_Formals(), String, no_expr()))),
          single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
      filename);
  installClass(new CgenNode(iocls, CgenNode::Basic, this));
  delete iocls;
  // End of Lab 3 changes
}

// install_classes enters a list of classes in the symbol table.
void CgenClassTable::installClasses(Classes cs) {
  for (auto *cls : cs) {
    installClass(new CgenNode(cls, CgenNode::NotBasic, this));
  }
}

// Add this CgenNode to the class list and the lookup table
void CgenClassTable::installClass(CgenNode *nd) {
  Symbol name = nd->get_name();
  if (!this->find(name)) {
    // The class name is legal, so add it to the list of classes
    // and the symbol table.
    nds.push_back(nd);
    this->insert(name, nd);
  }
}

// Add this CgenNode to the special class list and the lookup table
void CgenClassTable::installSpecialClass(CgenNode *nd) {
  Symbol name = nd->get_name();
  if (!this->find(name)) {
    // The class name is legal, so add it to the list of special classes
    // and the symbol table.
    specialNds.push_back(nd);
    this->insert(name, nd);
  }
}

// CgenClassTable::build_inheritance_tree
void CgenClassTable::buildInheritanceTree() {
  for (auto *node : nds)
    setRelations(node);
}

// CgenClassTable::set_relations
// Takes a CgenNode and locates its, and its parent's, inheritance nodes
// via the class table. Parent and child pointers are added as appropriate.
//
void CgenClassTable::setRelations(CgenNode *nd) {
  Symbol parent = nd->get_parent();
  auto parent_node = this->find(parent);
  assert(parent_node && std::string{"Class " + nd->get_name()->get_string() +
                                    " inherits from an undefined class " +
                                    parent->get_string()}
                            .data());
  nd->setParent(parent_node);
}

// Sets up declarations for extra functions needed for code generation
// You should modify this code for Lab 3
void CgenClassTable::setupExternalFunctions() {
  i64 = Type::getInt64Ty(this->context);
  i32 = Type::getInt32Ty(this->context);
  i8 = Type::getInt8Ty(this->context);
  i1 = Type::getInt1Ty(this->context);
  voidTy = Type::getVoidTy(this->context);
  // Using the default address space like this is a bad idea,
  // but we only support x86 and arm64 where this is valid.
  // A production compiler would choose a suitable default
  // based on the target.
  ptr = PointerType::get(this->context, 0);

  // setup function: external int strcmp(sbyte*, sbyte*)
  createLlvmFunction("strcmp", i32, {ptr, ptr}, false);
  // setup function: external int printf(sbyte*, ...)
  createLlvmFunction("printf", i32, {ptr}, true);
  // setup function: external void abort(void)
  createLlvmFunction("abort", voidTy, {}, false);
  // setup function: external i8* malloc(i32)
  createLlvmFunction("malloc", ptr, {i64}, false);
  // HINT: This is helpful for passing the initialization tests :)
  createLlvmFunction("memset", ptr, {ptr, i32, i64}, false);

  // Get rid of address sanitizer output for the generated executable after
  // compilation with clang. Disables checking for leaks in your built executable
  // NOTE: If you implement garbage collection, remove this block
  {
    auto *asanFunc =
        createLlvmFunction("__asan_default_options", ptr, {}, false);
    auto *asanFuncBb = BasicBlock::Create(this->context, "entry", asanFunc);
    builder.SetInsertPoint(asanFuncBb);
    auto *asanString =
        builder.CreateGlobalString("detect_leaks=0", "__asan_options_str", 0, &theModule);
    builder.CreateRet(asanString);
  }

  // TODO: You have two options on how to generate code for the method
  //       declarations of the Cool basic classes
  //       (Object, Int, Bool, String, and IO):
  //
  //       1. Add code here to setup the external functions that
  //       correspond to the methods defined and implemented in the
  //       Cool runtime library (coolrt) as special cases. This is manual
  //       and a bit tedious.
  //       2. Generate code for the method like any other class, except
  //       that you will only generate the method declarations, because
  //       the actual implementations are in the runtime library. This
  //       is less code to write, but you need to make sure you handle
  //       basic classes differently from programmer-defined ones.
  //
  //       It is probably most convenient to generate the declarations
  //       of the basic classes like every other class, that way you can
  //       minimize special casing.
}

void CgenClassTable::setupClasses(CgenNode *c, int depth) {
  c->setup(currentTag++, depth);
  for (auto *child : c->getChildren()) {
    setupClasses(child, depth + 1);
  }
  c->setMaxChild(currentTag - 1);
}

// The code generation first pass. Define these two functions to traverse
// the tree and setup each CgenNode
void CgenClassTable::setup() {
  setupExternalFunctions();
  setupClasses(root(), 0);
}

// The code generation second pass. Add code here to traverse the tree and
// emit code for each CgenNode
void CgenClassTable::codeModule() {
  codeConstants();

  // This must be after code_constants() since that emits constants
  // needed by the code() method for expressions

  // HINT: This is where the code generation for the classes happens, including
  // for class Main (and it's member Main.main()).
  codeClasses(root());

  // We'll use the same structure as Lab 2 for Lab 3 Ccheckpoint 1
  // After that we will just treat Main_main as a regular class and method
  CgenNode *mainNode = getMainMain(root());
  mainNode->codegenMainmain();

  // This function will create the main() function that starts the Cool program
  // HINT: This function is independent of the program itself
  codeMain();
}

// This function walks the class tree and generates code for each class
void CgenClassTable::codeClasses(CgenNode *c) {
  // TODO: add code here
  // HINT: Follow our regular approach of recursing down the tree, but
  //       you will generate code for the base class before the derived
  //       classes (unlike for expressions, where you generate bottom-up).

  c->codeClass();
  for (auto child : c->getChildren()){
    errs() << "This class: " << c->get_name() << "\n";
    errs() << "CHild class: " << child->get_name() << "\n";
    this->codeClasses(child);
  }
}

// Create global definitions for constant Cool objects
void CgenClassTable::codeConstants() {
  // TODO: add code here
  // HINT: Think of what sort of constants you might need as part
  //       of the ClassTable.Las
  // HINT: You will need to generate code for the string constants
  //       in/using the string table.
  stringtable.code_string_table(this);
}

// Get the root of the class tree.
CgenNode *CgenClassTable::root() {
  auto *root = this->find(Object);

  assert(root && "Class Object is not defined.");
  return root;
}

// Special-case functions used for the method Int Main::main() for
// Lab 2 and Lab 3 CP1.
CgenNode *CgenClassTable::getMainMain(CgenNode *c) {
  auto *ret = this->find(Main);
  return ret;
}

Function *CgenClassTable::createLlvmFunction(std::string const &funcName,
                                               Type *retType,
                                               ArrayRef<Type *> argTypes,
                                               bool isVarArgs) {
  assert(retType);
  FunctionType *ft = FunctionType::get(retType, argTypes, isVarArgs);
  Function *func = Function::Create(ft, Function::ExternalLinkage, funcName,
                                    this->theModule);
  if (!func) {
    errs() << "Function creation failed for function " << funcName;
    llvm_unreachable("Function creation failed");
  }
  return func;
}

std::pair<FunctionType*, Function*> CgenClassTable::createLlvmFunctionDetails(std::string const &funcName,
                                               Type *retType,
                                               ArrayRef<Type *> argTypes,
                                               bool isVarArgs) {
  // assert(retType);
  FunctionType *ft = FunctionType::get(retType, argTypes, isVarArgs);
  Function *func = Function::Create(ft, Function::ExternalLinkage, funcName,
                                    this->theModule);
  if (!func) {
    errs() << "Function creation failed for function " << funcName;
    llvm_unreachable("Function creation failed");
  }
  return {ft, func};
}

void program_class::cgen(std::optional<std::string> const &outfile) {
  initializeConstants();
  class_table = new CgenClassTable(classes);
  if (outfile) {
    std::error_code err;
    raw_fd_ostream s(*outfile, err, sys::fs::FA_Write);
    if (err) {
      std::cerr << "Cannot open output file " << *outfile << std::endl;
      exit(1);
    }
    s << class_table->theModule;
  } else {
    outs() << class_table->theModule;
  }
}

// TODO: Your own helpers come here.

/*********************************************************************

  StrTable / IntTable methods

 Coding string, int, and boolean constants

 Cool has three kinds of constants: strings, ints, and booleans.
 This section defines code generation for each type.

 All string constants are listed in the global "stringtable" and have
 type stringEntry. stringEntry methods are defined both for string
 constant definitions and references.

 All integer constants are listed in the global "inttable" and have
 type IntEntry. IntEntry methods are defined for Int constant references only.

 Since there are only two Bool values, there is no need for a table.
 The two booleans are represented by instances of the class BoolConst,
 which defines the definition and reference methods for Bools.

*********************************************************************/

// Create definitions for all String constants
void StrTable::code_string_table(CgenClassTable *ct) {
  // TODO: Depending on how you choose to turn charater strings into Cool
  // Strings,
  //       you may need to modify this function.
  for (auto &[_, entry] : this->_table) {
    entry.code_def(ct);
  }
}

// generate code to define a global string constant
void StringEntry::code_def(CgenClassTable *ct) {
  // TODO: define global character string constants
  // TODO: depending on how you choose to represent Cool strings, you may need
  //       to do additional things here beyond defining the global char string
  //       constant.
  ct->builder.CreateGlobalString(this->get_string(), this->get_string());
}
