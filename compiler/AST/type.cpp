#include <typeinfo>
#define TYPE_EXTERN
#include "expr.h"
#include "files.h"
#include "misc.h"
#include "stringutil.h"
#include "symbol.h"
#include "symtab.h"
#include "type.h"


Type::Type(astType_t astType, Expr* init_defaultVal) :
  BaseAST(astType),
  name(nilSymbol),
  defaultVal(init_defaultVal),
  asymbol(NULL)
{
  SET_BACK(defaultVal);
}


void Type::addName(Symbol* newname) {
  name = newname;
}


bool Type::isNull(void) {
  return (this == nilType);
}


bool Type::isComplex(void) {
  return (this == dtComplex);
}


Type* Type::copy(bool clone, CloneCallback* analysis_clone) {
  Type* new_type = copyType(clone, analysis_clone);

  new_type->lineno = lineno;
  new_type->filename = filename;
  if (analysis_clone) {
    analysis_clone->clone(this, new_type);
  }
  return new_type;
}


Type* Type::copyType(bool clone, CloneCallback* analysis_clone) {
  INT_FATAL(this, "Unanticipated call to Type::copyType");
  return nilType;
}


void Type::traverse(Traversal* traversal, bool atTop) {
  if (isNull()) {
    return;
  }
  if (traversal->processTop || !atTop) {
    traversal->preProcessType(this);
  }
  if (atTop || traversal->exploreChildTypes) {
    if (atTop || name == nilSymbol) {
      traverseDefType(traversal);
    }
    else {
      traverseType(traversal);
    }
  }
  if (traversal->processTop || !atTop) {
    traversal->postProcessType(this);
  }
}


void Type::traverseDef(Type* _this, Traversal* traversal, bool atTop) {
  if (isNull()) {
    return;
  }

  // expore Type and components
  if (traversal->processTop || !atTop) {
    traversal->preProcessType(_this);
  }
  if (atTop || traversal->exploreChildTypes) {
    TRAVERSE(_this->name, traversal, false);
    _this->traverseDefType(traversal);
  }
  if (traversal->processTop || !atTop) {
    traversal->postProcessType(_this);
  }
}


void Type::traverseType(Traversal* traversal) {
}


void Type::traverseDefType(Traversal* traversal) {
  TRAVERSE(defaultVal, traversal, false);
}


int Type::rank(void) {
  return 0;
}


void Type::print(FILE* outfile) {
  name->print(outfile);
}


void Type::printDef(FILE* outfile) {
  print(outfile);
}

void Type::codegen(FILE* outfile) {
  name->codegen(outfile);
}


void Type::codegenDef(FILE* outfile) {
  INT_FATAL(this, "Don't know how to codegenDef() for all types yet");
}


void Type::codegenSafeInit(FILE* outfile) {
  if (this == dtString) {
    fprintf(outfile, " = NULL");
  } else {
    // Most types won't need an initializer to be safe
  }
}


void Type::codegenStringToType(FILE* outfile) {
}


void Type::codegenIORoutines(FILE* outfile) {
}


void Type::codegenConfigVarRoutines(FILE* outfile) {
}


void Type::codegenDefaultFormat(FILE* outfile, bool isRead) {
  fprintf(outfile, "_default_format");
  if (isRead) {
    fprintf(outfile, "_read");
  } else {
    fprintf(outfile, "_write");
  }
  this->codegen(outfile);
}


void Type::codegenConstructors(FILE* outfile) {
}


bool Type::needsInit(void) {
  return false;
}


void Type::generateInit(FILE* outfile, VarSymbol* var) {
  INT_FATAL(this, "Don't know how to generateInit() for all types yet");
}


bool Type::outParamNeedsPtr(void) {
  return true;
}


bool Type::requiresCParamTmp(paramType intent) {
  switch (intent) {
  case PARAM_BLANK:
  case PARAM_IN:
  case PARAM_CONST:
  case PARAM_INOUT:
  case PARAM_OUT:
    return true;
  case PARAM_REF:
    return false;
  default:
    INT_FATAL(this, "case not handled in requiresCParamTmp");
    return false;
  }
}


EnumType::EnumType(EnumSymbol* init_valList) :
  Type(TYPE_ENUM, new Variable(init_valList)),
  valList(init_valList)
{
  Symbol* val = valList;
  while (val) {
    val->type = this;
    val = nextLink(Symbol, val);
  }
}


Type* EnumType::copyType(bool clone, CloneCallback* analysis_clone) {
  Type* copy = new EnumType(valList);
  copy->addName(name);
  return copy;

    /*
  Symbol* newSyms = valList->copyList(clone, analysis_clone);

  if (typeid(*newSyms) != typeid(EnumSymbol)) {
    INT_FATAL(this, "valList is not EnumSymbol in EnumType::copyType()");
    return nilType;
  } else {
    EnumSymbol* newEnums = (EnumSymbol*)newSyms;
    Type* copy = new EnumType(newEnums);
    copy->addName(name);
    return copy;
  }
    */
}


void EnumType::traverseDefType(Traversal* traversal) {
  TRAVERSE_LS(valList, traversal, false);
  TRAVERSE(defaultVal, traversal, false);
}


void EnumType::printDef(FILE* outfile) {
  printf("enum ");
  name->print(outfile);
  printf(" = ");
  valList->printList(outfile, " | ");
}


void EnumType::codegen(FILE* outfile) {
  name->codegen(outfile);
}


void EnumType::codegenDef(FILE* outfile) {
  EnumSymbol* enumSym;
  int last = -1;

  fprintf(outfile, "typedef enum {\n");
  enumSym = valList;
  while (enumSym) {
    enumSym->printDef(outfile);

    if (enumSym->val != last + 1) {
      fprintf(outfile, " = %d", enumSym->val);
    }
    last = enumSym->val;

    enumSym = nextLink(EnumSymbol, enumSym);

    if (enumSym) {
      fprintf(outfile, ",");
    }
    fprintf(outfile, "\n");
  }
  fprintf(outfile, "} ");
  name->codegen(outfile);
  fprintf(outfile, ";\n\n");
}


static void codegenIOPrototype(FILE* outfile, Symbol* name, bool isRead) {
  fprintf(outfile, "void ");
  if (isRead) {
    fprintf(outfile, "_read");
  } else {
    fprintf(outfile, "_write");
  }
  name->codegen(outfile);
  fprintf(outfile, "(FILE* ");
  if (isRead) {
    fprintf(outfile, "infile");
  } else {
    fprintf(outfile, "outfile");
  }
  fprintf(outfile, ", char* format, ");
  name->codegen(outfile);
  if (isRead) {
    fprintf(outfile, "*");
  }
  fprintf(outfile, " val)");
}


void EnumType::codegenStringToType(FILE* outfile) {
  EnumSymbol* enumSym = valList;

  fprintf(outfile, "int _convert_string_to_enum");
  name->codegen(outfile);
  fprintf(outfile, "(char* inputString, ");
  name->codegen(outfile);
  fprintf(outfile, "* val) {\n");
  
  while (enumSym) {
    fprintf(outfile, "if (strcmp(inputString, \"");
    enumSym->codegen(outfile);
    fprintf(outfile, "\") == 0) {\n");
    fprintf(outfile, "*val = ");
    enumSym->codegen(outfile);
    fprintf(outfile, ";\n");
    fprintf(outfile, "} else ");
    enumSym = nextLink(EnumSymbol, enumSym);
  }
  fprintf(outfile, "{ \n");
  fprintf(outfile, "return 0;\n");
  fprintf(outfile, "}\n");
  fprintf(outfile, "return 1;\n}\n\n");
}

void EnumType::codegenIORoutines(FILE* outfile) {
  EnumSymbol* enumSym = valList;
  bool isRead;

  isRead = true;
  codegenIOPrototype(intheadfile, name, isRead);
  fprintf(intheadfile, ";\n");
  
  isRead = false;
  codegenIOPrototype(intheadfile, name, isRead);
  fprintf(intheadfile, ";\n\n");

  isRead = true;
  codegenIOPrototype(outfile, name, isRead);
  fprintf(outfile, " {\n");
  fprintf(outfile, "char* inputString = NULL;\n");
  fprintf(outfile, "_read_string(stdin, format, &inputString);\n");
  fprintf(outfile, "if (!(_convert_string_to_enum");
  name->codegen(outfile);
  fprintf(outfile, "(inputString, val))) {\n");
  fprintf(outfile, "fflush(stdout);\n");
  fprintf(outfile, "fprintf (stderr, \"***ERROR:  Not of ");
  name->codegen(outfile);
  fprintf(outfile, " type***\\n\");\n");
  fprintf(outfile, "exit(0);\n");
  fprintf(outfile, "}\n");
  fprintf(outfile, "}\n\n");

  isRead = false;
  codegenIOPrototype(outfile, name, isRead);
  fprintf(outfile, " {\n");
  fprintf(outfile, "switch (val) {\n");
  while (enumSym) {
    fprintf(outfile, "case ");
    enumSym->codegen(outfile);
    fprintf(outfile, ":\n");
    fprintf(outfile, "fprintf(outfile, format, \"");
    enumSym->codegen(outfile);
    fprintf(outfile, "\");\n");
    fprintf(outfile, "break;\n");

    enumSym = nextLink(EnumSymbol, enumSym);
  }
  fprintf(outfile, "}\n");
  fprintf(outfile, "}\n\n");
}


void EnumType::codegenConfigVarRoutines(FILE* outfile) {
  fprintf(outfile, "int setInCommandLine");
  name->codegen(outfile);
  fprintf(outfile, "(char* varName, ");
  name->codegen(outfile);
  fprintf(outfile, "* value, char* moduleName) {\n");
  fprintf(outfile, "int isDefaultValue = 0;\n");
  fprintf(outfile, "int varSet = 0;\n");
  fprintf(outfile, "char* setValue = lookupSetValue(varName, moduleName);\n");
  fprintf(outfile, "if (setValue) {\n");
  fprintf(outfile, "int validEnum = _convert_string_to_enum");
  name->codegen(outfile);
  fprintf(outfile, "(setValue, value);\n");
  fprintf(outfile, "if (validEnum) {\n");
  fprintf(outfile, "varSet = 1;\n");
  fprintf(outfile, "} else {\n");
  fprintf(outfile, "fprintf(stderr, \"***Error: \\\"%%s\\\" is not a valid "
	  "value for a config var \\\"%%s\\\" of type ");
  name->codegen(outfile);
  fprintf(outfile, "***\\n\", setValue, varName);\n");
  fprintf(outfile, "exit(0);\n");
  fprintf(outfile, "}\n");
  fprintf(outfile, "}\n");
  fprintf(outfile, "return varSet;\n");
  fprintf(outfile, "}\n\n");
}


void EnumType::codegenDefaultFormat(FILE* outfile, bool isRead) {
  fprintf(outfile, "_default_format");
  if (isRead) {
    fprintf(outfile, "_read");
  } else {
    fprintf(outfile, "_write");
  }
  fprintf(outfile, "_enum");
}


DomainType::DomainType(Expr* init_expr) :
  Type(TYPE_DOMAIN, nilExpr),
  numdims(0),
  parent(nilExpr)
{
  if (!init_expr->isNull()) {
    if (typeid(*init_expr) == typeid(IntLiteral)) {
      numdims = init_expr->intVal();
    } else {
      numdims = init_expr->rank();
      parent = init_expr;
      SET_BACK(parent);
    }
  }
}


DomainType::DomainType(int init_numdims) :
  Type(TYPE_DOMAIN, nilExpr),
  numdims(init_numdims),
  parent(nilExpr)
{}


Type* DomainType::copyType(bool clone, CloneCallback* analysis_clone) {
  Type* copy;
  if (parent->isNull()) {
    copy = new DomainType(numdims);
  } else {
    copy = new DomainType(parent->copy(clone, analysis_clone));
  }
  copy->addName(name);
  return copy;
}


int DomainType::rank(void) {
  return numdims;
}


void DomainType::print(FILE* outfile) {
  fprintf(outfile, "domain(");
  if (parent->isNull()) {
    if (numdims != 0) {
      fprintf(outfile, "%d", numdims);
    } else {
      fprintf(outfile, "???");
    }
  } else {
    parent->print(outfile);
  }
  fprintf(outfile, ")");
}


void DomainType::codegen(FILE* outfile) {
  fprintf(outfile, "_domain%d", numdims ? numdims : 1);  // BLC: hack!
}


IndexType::IndexType(Expr* init_expr) :
  DomainType(init_expr)
{
  astType = TYPE_INDEX;
}


IndexType::IndexType(int init_numdims) :
  DomainType(init_numdims)
{
  astType = TYPE_INDEX;
}


Type* IndexType::copyType(bool clone, CloneCallback* analysis_clone) {
  Type* copy;
  if (parent->isNull()) {
    copy = new IndexType(numdims);
  } else {
    copy = new IndexType(parent->copy(clone, analysis_clone));
  }
  copy->addName(name);
  return copy;
}


void IndexType::print(FILE* outfile) {
  fprintf(outfile, "index(");
  if (parent->isNull()) {
    if (numdims != 0) {
      fprintf(outfile, "%d", numdims);
    } else {
      fprintf(outfile, "???");
    }
  } else {
    parent->print(outfile);
  }
  fprintf(outfile, ")");
}


ArrayType::ArrayType(Expr* init_domain, Type* init_elementType):
  Type(TYPE_ARRAY, init_elementType->defaultVal),
  domain(init_domain),
  elementType(init_elementType)
{
  SET_BACK(domain);
}


Type* ArrayType::copyType(bool clone, CloneCallback* analysis_clone) {
  Type* copy = new ArrayType(domain->copy(clone, analysis_clone),
			     elementType->copy(clone, analysis_clone));
  copy->addName(name);
  return copy;
}


void ArrayType::traverseDefType(Traversal* traversal) {
  TRAVERSE(domain, traversal, false);
  TRAVERSE(elementType, traversal, false);
  TRAVERSE(defaultVal, traversal, false);
}


int ArrayType::rank(void) {
  return domain->rank();
}


void ArrayType::print(FILE* outfile) {
  //  fprintf(outfile, "[");
  domain->print(outfile);
  //  fprintf(outfile, "] ");
  fprintf(outfile, " ");
  elementType->print(outfile);
}


void ArrayType::codegen(FILE* outfile) {
  fprintf(outfile, "_array%d", domain->rank());
  elementType->codegen(outfile);
}


void ArrayType::codegenDefaultFormat(FILE* outfile, bool isRead) {
  elementType->codegenDefaultFormat(outfile, isRead);
}


bool ArrayType::needsInit(void) {
  return true;
}


void ArrayType::generateInit(FILE* outfile, VarSymbol* sym) {
  fprintf(outfile, "_init");
  this->codegen(outfile);
  fprintf(outfile, "(&(");
  sym->codegen(outfile);
  fprintf(outfile, "), &(");
  domain->codegen(outfile);
  fprintf(outfile, "));\n");
}


UserType::UserType(Type* init_definition, Expr* init_defaultVal) :
  Type(TYPE_USER, init_defaultVal),
  definition(init_definition)
{}


Type* UserType::copyType(bool clone, CloneCallback* analysis_clone) {
  Type* copy = new UserType(definition,
			    defaultVal->copy(clone, analysis_clone));
  copy->addName(name);
  return copy;
}


bool UserType::isComplex(void) {
  return definition->isComplex();
}


void UserType::traverseDefType(Traversal* traversal) {
  TRAVERSE(definition, traversal, false);
  TRAVERSE(defaultVal, traversal, false);
}


void UserType::printDef(FILE* outfile) {
  fprintf(outfile, "type ");
  name->print(outfile);
  fprintf(outfile, " = ");
  definition->print(outfile);
}


void UserType::codegen(FILE* outfile) {
  name->codegen(outfile);
}


void UserType::codegenDef(FILE* outfile) {
  fprintf(outfile, "typedef ");
  definition->codegen(outfile);
  fprintf(outfile, " ");
  name->codegen(outfile);
  fprintf(outfile, ";\n");
}


// TODO: We should probably instead have types print out
// their own write routines and have UserType print its
// definition's write routine

static void codegenIOPrototypeBody(FILE* outfile, Symbol* name, Type* definition, bool isRead) {
  codegenIOPrototype(outfile, name, isRead);
  fprintf(outfile, " {\n");
  if (isRead) {
    fprintf(outfile, " _read");
  } else {
  fprintf(outfile, "  _write");
  }
  definition->codegen(outfile);  
  if (isRead) {
    fprintf(outfile, "(infile, format, val);\n");
  } else {
    fprintf(outfile, "(outfile, format, val);\n");
  }    
  fprintf(outfile, "}\n");
}    


void UserType::codegenIORoutines(FILE* outfile) {
  bool isRead;

  isRead = true;
  codegenIOPrototype(intheadfile, name, isRead);
  fprintf(intheadfile, ";\n");

  isRead = false;
  codegenIOPrototype(intheadfile, name, isRead);
  fprintf(intheadfile, ";\n\n");

  isRead = true;
  codegenIOPrototypeBody(outfile, name, definition, isRead);
  fprintf(outfile, "\n\n");

  isRead = false;
  codegenIOPrototypeBody(outfile, name, definition, isRead);
}


void UserType::codegenDefaultFormat(FILE* outfile, bool isRead) {
  definition->codegenDefaultFormat(outfile, isRead);
}


ClassType::ClassType(bool isValueClass, bool isUnion,
		     ClassType* init_parentClass, Stmt* init_definition,
		     Stmt* init_constructor, SymScope* init_classScope) :
  Type(TYPE_CLASS, nilExpr),
  value(isValueClass),
  union_value(isUnion),
  parentClass(init_parentClass),
  definition(init_definition),
  constructor(init_constructor),
  classScope(init_classScope)
{
  SET_BACK(definition);
  constructor->back = &(Stmt*&)constructor; // UGH --SJD
}


Type* ClassType::copyType(bool clone, CloneCallback* analysis_clone) {
  Type* copy = new ClassType(value,
			     union_value,
			     parentClass,
			     definition->copyList(clone, analysis_clone),
			     constructor->copy(clone, analysis_clone),
			     classScope);
  copy->addName(name);
  return copy;
}


void ClassType::addDefinition(Stmt* init_definition) {
  definition = init_definition;
  SET_BACK(definition); // SJD: Eliminate please.

  if (!isNull() && Symboltable::parsingUserCode()) {
    /* create default constructor */

    char* constructorName = glomstrings(2, "_construct_", name->name);
    FnSymbol* newFunSym = Symboltable::startFnDef(new FnSymbol(constructorName));
    if (value || union_value) {
      VarSymbol* this_insert = new VarSymbol("this", this);
      VarDefStmt* body1 = new VarDefStmt(this_insert, nilExpr);
      this_insert->setDefPoint(body1);
      ReturnStmt* body2 =  new ReturnStmt(new Variable(this_insert));
      body1->append(body2);
      BlockStmt* body = new BlockStmt(body1);
      constructor = Symboltable::finishFnDef(newFunSym, nilSymbol, this, body);
    }
    else {
      BlockStmt* body = new BlockStmt(
			  new ReturnStmt(
			    new CastExpr(this, 
			      new FnCall(
			        new Variable(
				  Symboltable::lookupInternal("malloc")), 
			        new SizeofExpr(this)))
			    )
			  );
      constructor = Symboltable::finishFnDef(newFunSym, nilSymbol, this, body);
    }
    /** Add test tags for unions: This is a little ugly, it should
	insert the enum statement that we generate in codegen before
	the union def stmt.
    **/
    if (union_value) {
      Stmt* tmpStmt = definition;

      while (tmpStmt) {
	if (VarDefStmt* varStmt = dynamic_cast<VarDefStmt*>(tmpStmt)) {
	  VarSymbol* idtag = new VarSymbol(glomstrings(4, "_", name->name, "_union_id_", varStmt->var->name));
	  idtag->setDefPoint(NULL); // SHOULD BE REAL statement for declaring this enum, UGH...short-term
	}
	tmpStmt = nextLink(Stmt, tmpStmt);
      }
    }
  }
  constructor->back = &(Stmt*&)constructor; // UGH --SJD
}


void ClassType::setClassScope(SymScope* init_classScope) {
  classScope = init_classScope;
}


void ClassType::traverseDefType(Traversal* traversal) {
  SymScope* prevScope;
  if (classScope) {
    prevScope = Symboltable::setCurrentScope(classScope);
  }
  TRAVERSE_LS(definition, traversal, false);
  TRAVERSE_LS(constructor, traversal, false);
  TRAVERSE(defaultVal, traversal, false);
  if (classScope) {
    Symboltable::setCurrentScope(prevScope);
  }
}


bool ClassType::isNull(void) {
  return (this == nilClassType);
}


void ClassType::codegen(FILE* outfile) {
  name->codegen(outfile);
}


void ClassType::codegenDef(FILE* outfile) {
  if (union_value) {
    fprintf(outfile, "typedef enum _");
    name->codegen(outfile);
    fprintf(outfile, "_union_id_def {\n");
    definition->codegenVarNames(outfile, 
				glomstrings(3, "_", name->name, "_union_id_"), "");
    fprintf(outfile, "} _");
    name->codegen(outfile);
    fprintf(outfile, "_union_id;\n\n");
  }
  fprintf(outfile, "typedef struct _");
  name->codegen(outfile);
  fprintf(outfile, "_def {\n");
  if (union_value) {
    fprintf(outfile, "_");
    name->codegen(outfile);
    fprintf(outfile, "_union_id _chpl_union_tag;\n");
    fprintf(outfile, "union _chpl_union_def {\n");
  }
  definition->codegenVarDefs(outfile);
  if (union_value) {
    fprintf(outfile, "} _chpl_union;\n");
  }
  fprintf(outfile, "} ");
  if (value || union_value) {
    name->codegen(outfile);
    fprintf(outfile, ";\n\n");
  }
  else {
    fprintf(outfile, "_");
    name->codegen(outfile);
    fprintf(outfile,", *");
    name->codegen(outfile);
    fprintf(outfile, ";\n\n");
  }
}


void ClassType::codegenConstructors(FILE* outfile) {
  constructor->codegenList(outfile, "\n");
}


void ClassType::codegenIORoutines(FILE* outfile) {
  bool isRead;

  isRead = true;
  codegenIOPrototype(intheadfile, name, isRead);
  fprintf(intheadfile, ";\n");

  isRead = false;
  codegenIOPrototype(intheadfile, name, isRead);
  fprintf(intheadfile, ";\n\n");

  isRead = true;
  codegenIOPrototype(outfile, name, isRead);
  fprintf(outfile, "{\n");
  fprintf(outfile, "}\n");

  isRead = false;
  codegenIOPrototype(outfile, name, isRead);
  fprintf(outfile, "{\n");
  fprintf(outfile, "}\n");
}


TupleType::TupleType(Type* firstType) :
  Type(TYPE_TUPLE, nilExpr)
{
  components.add(firstType);
}


void TupleType::addType(Type* additionalType) {
  components.add(additionalType);
}


Type* TupleType::copyType(bool clone, CloneCallback* analysis_clone) {
  TupleType* newTupleType =
    new TupleType(components.v[0]->copy(clone, analysis_clone));
  for (int i=1; i<components.n; i++) {
    newTupleType->addType(components.v[i]->copy(clone, analysis_clone));
  }
  newTupleType->addName(name);
  return newTupleType;
}


void TupleType::traverseDefType(Traversal* traversal) {
  for (int i=0; i<components.n; i++) {
    TRAVERSE(components.v[i], traversal, false);
  }
  TRAVERSE(defaultVal, traversal, false);
}


void TupleType::print(FILE* outfile) {
  fprintf(outfile, "(");
  for (int i=0; i<components.n; i++) {
    if (i) {
      fprintf(outfile, ", ");
    }
    components.v[i]->print(outfile);
  }
  fprintf(outfile, ")");
}


void TupleType::codegen(FILE* outfile) {
  INT_FATAL(this, "Cannot codegen Tuple types yet");
}


VariableType::VariableType() :
  Type(TYPE_VARIABLE, nilExpr)
{}


Type* VariableType::copyType(bool clone, CloneCallback* analysis_clone) {
  return new VariableType();
}


void VariableType::codegen(FILE* outfile) {
  INT_FATAL(this, "ERROR:  Cannot codegen a variable type.");
}


UnresolvedType::UnresolvedType(char* init_name) :
  Type(TYPE_UNRESOLVED, nilExpr) {
  name = new UnresolvedSymbol(init_name);
}


Type* UnresolvedType::copyType(bool clone, CloneCallback* analysis_clone) {
  return new UnresolvedType(copystring(name->name));
}


void UnresolvedType::codegen(FILE* outfile) {
  INT_FATAL(this, "ERROR:  Cannot codegen an unresolved type.");
}


void initTypes(void) {
  // define built-in types
  dtUnknown = Symboltable::defineBuiltinType("???", "???", nilExpr);
  dtVoid = Symboltable::defineBuiltinType("void", "void", nilExpr);

  dtBoolean = Symboltable::defineBuiltinType("boolean", "_boolean",
					     new BoolLiteral("false", false));
  dtInteger = Symboltable::defineBuiltinType("integer", "_integer64",
					     new IntLiteral("0", 0));
  dtFloat = Symboltable::defineBuiltinType("float", "_float64",
					   new FloatLiteral("0.0", 0.0));
  dtComplex = Symboltable::defineBuiltinType("complex", "_complex128",
					     new FloatLiteral("0.0", 0.0));
  dtString = Symboltable::defineBuiltinType("string", "_string", new StringLiteral(""));
  

  dtLocale = Symboltable::defineBuiltinType("locale", "_locale", nilExpr);
}


void findInternalTypes(void) {
  dtTuple = Symboltable::lookupInternalType("Tuple")->type;
  dtIndex = Symboltable::lookupInternalType("Index")->type;
  dtDomain = Symboltable::lookupInternalType("Domain")->type;
  dtArray = Symboltable::lookupInternalType("Array")->type;
}
