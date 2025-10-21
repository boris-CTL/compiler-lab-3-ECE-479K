#ifndef CGENNODE_H
#define CGENNODE_H

#include "CgenClassTable.h"
#include "cool_tree.h"
#include "symbol.h"
#include <llvm/ADT/SmallVector.h>
#include <span>
#include <vector>

using namespace llvm;

// Each CgenNode corresponds to a Cool class. As such, it is responsible for
// performing code generation on the class level. This includes laying out
// the class attributes, creating the necessary Types for the class and
// generating code for each of its methods.
class CgenNode : public class__class {
public:
  std::vector<std::tuple<Type *, attr_class *>> attribute_layout;
  std::vector<std::tuple<std::string, method_class *>> method_layout;

  std::tuple<Type *, attr_class *> insert_attribute(Symbol type_decl, attr_class *attr){
    Type *type = this->getClasstable().get_llvm_type_from_symbol(type_decl);
    std::tuple<Type *, attr_class *> entry = std::tuple<Type *, attr_class *>(type, attr);
    this->attribute_layout.push_back(entry);
    return entry;
  }

  std::tuple<std::string, method_class *> insert_method(std::string method_name, method_class *meth){
    std::string meth_name = this->getFullMethodName(method_name);
    std::tuple<std::string, method_class *> entry = std::tuple<std::string, method_class *>(meth_name, meth);
    method_layout.push_back(entry);
    return entry;
  }

  int find_idx(std::string name){
    for(int i=0; i<this->attribute_layout.size(); i++){
      auto [attr_type, attr_obj] = this->attribute_layout[i];
      if(name == attr_obj->get_name()->get_string()){
        return i + 1;
      }
    }
    return -1;
  }

  enum Basicness { Basic, NotBasic };
  CgenNode(Class_ c, Basicness bstatus, CgenClassTable *classTable)
      : class__class(static_cast<class__class const &>(*c)), parentnd(nullptr), children(0),
        basicStatus(bstatus), classTable(*classTable), tag(-1) {
        }

  // Relationships with other nodes in the tree
  void setParent(CgenNode *p) {
    assert(this->parentnd == nullptr);
    assert(p);
    p->children.push_back(this);
    this->parentnd = p;
  }
  CgenNode *getParentnd() const { return parentnd; }
  int basic() const { return basicStatus == Basic; }
  std::span<CgenNode *const> getChildren() { return children; }
  void setMaxChild(int mc) { maxChild = mc; }
  int getMaxChild() const { return maxChild; }

  // Accessors for other provided fields
  int getTag() const { return tag; }
  CgenClassTable &getClasstable() const { return classTable; }

  // Lab 3 additions
  std::string getTypeName() const { return name->get_string(); }
  std::string getVtableTypeName() const {
    return "_" + getTypeName() + "_vtable";
  }
  std::string getVtableName() const {
    return "_" + getTypeName() + "_vtable_prototype";
  }
  std::string getInitFunctionName() const { return getTypeName() + "_new"; }

  std::string getFullMethodName(std::string method_name) const {
    return getTypeName() + "_" + method_name;
  }

  llvm::StructType *getType() const;
  llvm::StructType *getVtableType() const;
  // End Lab 3 additions

  void setup(int tag, int depth);
  // Layout the methods and attributes for code generation
  void layoutFeatures();
  // Class codegen. You need to write the body of this function.
  void codeClass();
  // Codegen for the init function of every class
  void codeInitFunction(CgenEnvironment *env);
  // TODO: Complete the implementation of the following function
  void codegenMainmain();

  llvm::SmallVector<llvm::Type *> get_feature_type() {
        return list_of_types_of_features;
  }
  
  void insert_method_body(llvm::Constant *c) {
    vtable_const_list.insert(vtable_const_list.begin(), c);
    vtable_ty_list.insert(vtable_ty_list.begin(), c->getType());
  }

  llvm::SmallVector<llvm::Type *> get_vtable_ty_list() {
    return vtable_ty_list;
  }

  llvm::SmallVector<llvm::Constant *> get_vtable_const_list() {
    return vtable_const_list;
  }

  void push_attributes(std::string attr, llvm::Type *llvm_type) {
      list_names_of_attributes.emplace_back(attr);
      list_of_types_of_features.emplace_back(llvm_type);
  }

  bool check_vtable(llvm::Constant *c)
  {
    auto vtable_size = vtable_const_list.size();
    if (vtable_size <= 2)
    {
      vtable_const_list.emplace_back(c);
      vtable_ty_list.emplace_back(c->getType());
      return true;
    }
    else {
      return false;
    }
  }

  void push_method(llvm::Constant *c) {
    bool checking_res = check_vtable(c);
    if (checking_res) {
      return;
    }

    StringRef method_given = c->getName();
    size_t substring_pos = method_given.find("_");
    StringRef root_of_given_method = method_given.substr(substring_pos + 1, method_given.size());
    bool if_changed = false;
    for (int i = 0; i < vtable_const_list.size(); i++) {
      StringRef cur_method = vtable_const_list[i]->getName();
      size_t substr_pos_cur = cur_method.find("_");
      StringRef cur_root = cur_method.substr(substr_pos_cur + 1,	cur_method.size());

      if (cur_root == root_of_given_method) {
        vtable_const_list[i] = c;
        if_changed = true;
        break;
      }
    }
    if (!if_changed) {
      vtable_const_list.emplace_back(c);
      vtable_ty_list.emplace_back(c->getType());
    }
  }

  llvm::StructType *get_struct_type() {
    return struct_type;
  }

  std::pair<int, llvm::Type*> get_type_of_attribute(std::string given_name) {
    int idx = 0;
    for (auto const &name: list_names_of_attributes) {
      if (name == given_name) {
	      return {idx, list_of_types_of_features[idx]};
      }
      idx++;
    }
    return {-27, nullptr};
  }


private:
  CgenNode *parentnd;                     // Parent of class
  llvm::SmallVector<CgenNode *> children; // Children of class
  Basicness basicStatus;                 // `Basic' or 'NotBasic'
  CgenClassTable &classTable;
  // Class tag. Should be unique for each class in the tree
  int tag, maxChild;

  mutable llvm::StructType *body = nullptr;
  mutable llvm::StructType *vtable = nullptr;
  llvm::GlobalVariable *name_str;

  llvm::SmallVector<llvm::Type *> list_of_types_of_features;
  llvm::SmallVector<std::string> list_names_of_attributes;
  llvm::StructType *struct_type;
  llvm::StructType *struct_type_of_vtable;
  // llvm::GlobalVariable *vtable;
  llvm::GlobalVariable *vtable_global_var;

  llvm::SmallVector<llvm::Type *> vtable_ty_list;
  llvm::SmallVector<llvm::Constant *> vtable_const_list; // vtable instance in global
  llvm::SmallVector<std::string> method_name;
};


#endif // CGENNODE_H
