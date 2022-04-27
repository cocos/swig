/* -----------------------------------------------------------------------------
 * This file is part of SWIG, which is licensed as a whole under version 3
 * (or any later version) of the GNU General Public License. Some additional
 * terms also apply to certain portions of SWIG. The full details of the SWIG
 * license and copyrights can be found in the LICENSE and COPYRIGHT files
 * included with the SWIG source code as distributed by the SWIG developers
 * and at http://www.swig.org/legal.html.
 *
 * javascript.cxx
 *
 * Javascript language module for SWIG.
 * ----------------------------------------------------------------------------- */

#include "cparse.h"
#include "swigmod.h"
#include <vector>
#include <string>
#include <stack>
#include <sstream>

namespace cc {

/**
 * Enables extra debugging information in typemaps.
 */
static bool js_template_enable_debug = false;

#define ERR_MSG_ONLY_ONE_ENGINE_PLEASE "Only one engine can be specified at a time."

// keywords used for state variables
#define NAME "name"
#define NEST_CLASS_NAME_LIST "nest_class_name_list"
#define NAME_MANGLED "name_mangled"
#define TYPE "type"
#define TYPE_MANGLED "type_mangled"
#define WRAPPER_NAME "wrapper"
#define IS_IMMUTABLE "is_immutable"
#define IS_WRITE_ONLY "is_write_only"
#define IS_STATIC "is_static"
#define IS_ABSTRACT "is_abstract"
#define GETTER "getter"
#define SETTER "setter"
#define PARENT "parent"
#define PARENT_MANGLED "parent_mangled"
#define CTOR "ctor"
#define CTOR_WRAPPERS "ctor_wrappers"
#define CTOR_DISPATCHERS "ctor_dispatchers"
#define DTOR "dtor"
#define ARGCOUNT "wrap:argc"
#define HAS_TEMPLATES "has_templates"
#define FORCE_CPP "force_cpp"
#define RESET true

// keys for global state variables
#define CREATE_NAMESPACES "create_namespaces"
#define REGISTER_CLASSES "register_classes"
#define REGISTER_NAMESPACES "register_namespaces"
#define REGISTER_GLOBAL "register_global"
#define INITIALIZER "initializer"
#define HEADER_REGISTER_MODULE "header_register_module"
#define HEADER_REGISTER_CLASSES "header_register_classes"

// keys for class scoped state variables
#define MEMBER_VARIABLES "member_variables"
#define MEMBER_FUNCTIONS "member_functions"
#define STATIC_FUNCTIONS "static_functions"
#define STATIC_VARIABLES "static_variables"

static std::string join(const std::vector<std::string>& v, const std::string& c) {
    std::stringstream s;
    for (auto p = v.begin(); p != v.end(); ++p) {
        s << *p;
        if (p != v.end() - 1) {
            s << c;
        }
    }
    return s.str();
}

static Node* getParentNodeByNodeType(Node* n, const char* nodeType) {
    Node *parentNode = parentNode(n);
    if (parentNode != nullptr) {
        if (0 == Cmp(nodeType(parentNode), nodeType)) {
            return parentNode;
        }

        return getParentNodeByNodeType(parentNode, nodeType);
    }

    return nullptr;
}

static Node* getClassNode(Node* n) {
    return getParentNodeByNodeType(n, "class");
}

static Node* getNamespaceNode(Node* n) {
    return getParentNodeByNodeType(n, "namespace");
}

static List* createNestClassSymNameList(Node* n) {
    List* ret = nullptr;
    if (GetFlag(n, "nested")) {
        ret = NewList();
        for (Node *outer_class = Getattr(n, "nested:outer"); outer_class != nullptr; outer_class = Getattr(outer_class, "nested:outer")) {
              if (String* name = Getattr(outer_class, "sym:name")) {
                  Insert(ret, 0, name);
              }
              else {
                  Delete(ret);
                  return nullptr;
              }
        }
        Append(ret, Getattr(n, "sym:name"));
    }

    return ret;
}

static String* joinClassSymNameWithList(List* l, const_String_or_char_ptr concatStr) {
    assert(l != nullptr);
    String* ret = NewStringEmpty();

    int len = Len(l);
    if (len > 0) {

        for (int i = 0; i < len; ++i) {
            auto* name = Getitem(l, i);
            Append(ret, name);
            if (i != (len-1)){
                Append(ret, concatStr);
            }
        }
    }

    return ret;
}

static std::vector<std::string> getNamespaceNameArray(Node* n) {
    std::vector<std::string> namespaceArray;
    Node* curNode = n;
    do {
        Node* nsNode = getNamespaceNode(curNode);
        if (nsNode == nullptr) {
            break;
        }

        namespaceArray.insert(std::begin(namespaceArray), Char(Getattr(nsNode, "sym:name")));
        curNode = nsNode;
    } while (true);

    return namespaceArray;
}

struct PropertyName {
    std::string name;
    std::string symName;

    bool isValid() const {
        return !name.empty() && !symName.empty();
    }
};

static std::string fixCppKeyword(const std::string& s) {
    static const char CPP_KEYWORD_PREFIX[] = "cpp_keyword_";
    if (0 == s.find(CPP_KEYWORD_PREFIX)) {
        return s.substr(sizeof(CPP_KEYWORD_PREFIX)-1);
    }
    return s;
}

static PropertyName getPropertyName(Node* n) {
    if (0 == Cmp(Getattr(n, "kind"), "variable")) {
        String* storage = Getattr(n, "storage");
        String* access = Getattr(n, "access");
        int needIgnore = GetFlag(n, "feature:ignore");
        // ignore properties those name begin with '_'
        if (!needIgnore
            && 0 == Cmp(access, "public")
            && 0 != Cmp(storage, "static")) {
            return {
                std::string(Char(Getattr(n, "name"))),
                fixCppKeyword(Char(Getattr(n, "sym:name")))
            };
        }
    }
    return {};
}

static std::vector<PropertyName> getStructProperties(Node* n) {
    std::vector<PropertyName> ret;
    auto* nodeType = nodeType(n);
    if (0 == Cmp(nodeType, "class")) {
        auto* child = firstChild(n);
        if (child != nullptr) {
            auto name = getPropertyName(child);
            if (name.isValid()) {
                ret.emplace_back(name);
            }

            Node* cur = child;
            Node* next = nullptr;
            do {
                next = nextSibling(cur);
                if (next == nullptr) {
                    break;
                }

                name = getPropertyName(next);
                if (name.isValid()) {
                    ret.emplace_back(name);
                }

                cur = next;
            } while (true);
        }
    }

    return ret;
}

static void convertToMangledName(String* name) {
    Replaceall(name, "::", "_");
    Replaceall(name, "<", "_");
    Replaceall(name, ">", "");
    Replaceall(name, " ", "");
    Replaceall(name, ",", "_");
    Replaceall(name, "*", "_");
}

static int getParamListCount(ParmList* params) {
    int count = 0;
    ParmList* p = params;
    while (p != nullptr) {
        ++count;
        p = nextSibling(p);
    }
    return count;
}

/**
 * A convenience class to manage state variables for emitters.
 * The implementation delegates to SWIG Hash DOHs and provides
 * named sub-hashes for class, variable, and function states.
 */
class JSEmitterState {
public:
    JSEmitterState();
    ~JSEmitterState();
    DOH *globals();
    DOH *globals(const char *key, DOH *initial = 0);
    DOH *clazz(bool reset = false);
    DOH *clazz(const char *key, DOH *initial = 0);
    DOH *function(bool reset = false);
    DOH *function(const char *key, DOH *initial = 0);
    DOH *variable(bool reset = false);
    DOH *variable(const char *key, DOH *initial = 0);
    static int IsSet(DOH *val);

private:
    DOH *getState(const char *key, bool reset = false);
    Hash *globalHash;
};

/**
 * A convenience class that wraps a code snippet used as template
 * for code generation.
 */
class Template {
public:
    Template(const String *code);
    Template(const String *code, const String *templateName);
    Template(const Template &other);
    ~Template();
    String *str();
    Template &replace(const String *pattern, const String *repl);
    Template &print(DOH *doh);
    Template &pretty_print(DOH *doh);
    void operator=(const Template &t);
    Template &trim();

private:
    String *code;
    String *templateName;
};

/**
 * JSEmitter represents an abstraction of javascript code generators
 * for different javascript engines.
 **/
class JSEmitter {
protected:
    typedef JSEmitterState State;

    enum MarshallingMode {
        Setter,
        Getter,
        Ctor,
        Function
    };

public:
    enum JSEngine {
        Cocos
    };

    JSEmitter(JSEngine engine);

    virtual ~JSEmitter();

    /**
   * Opens output files and temporary output DOHs.
   */
    virtual int initialize(Node *n);

    /**
   * Writes all collected code into the output file(s).
   */
    virtual int dump(Node *n) = 0;

    /**
   * Cleans up all open output DOHs.
   */
    virtual int close() = 0;

    /**
   * Switches the context for code generation.
   *
   * Classes, global variables and global functions may need to
   * be registered in certain static tables.
   * This method should be used to switch output DOHs correspondingly.
   */
    virtual int switchNamespace(Node *);

    /**
   * Invoked at the beginning of the classHandler.
   */
    virtual int enterClass(Node *);

    /**
   * Invoked at the end of the classHandler.
   */
    virtual int exitClass(Node *) {
        return SWIG_OK;
    }

    /**
   * Invoked at the beginning of the variableHandler.
   */
    virtual int enterVariable(Node *);

    /**
   * Invoked at the end of the variableHandler.
   */
    virtual int exitVariable(Node *) {
        return SWIG_OK;
    }

    /**
   * Invoked at the beginning of the functionHandler.
   */
    virtual int enterFunction(Node *);

    /**
   * Invoked at the end of the functionHandler.
   */
    virtual int exitFunction(Node *) {
        return SWIG_OK;
    }

    /**
   * Invoked by functionWrapper callback after call to Language::functionWrapper.
   */
    virtual int emitWrapperFunction(Node *n);

    /**
   * Invoked by nativeWrapper callback
   */
    virtual int emitNativeFunction(Node *n);

    /**
   * Invoked from constantWrapper after call to Language::constantWrapper.
   **/
    virtual int emitConstant(Node *n);

    /**
   * Registers a given code snippet for a given key name.
   *
   * This method is called by the fragmentDirective handler
   * of the COCOS language module.
   **/
    int registerTemplate(const String *name, const String *code);

    /**
   * Retrieve the code template registered for a given name.
   */
    Template getTemplate(const String *name);

    inline State& currentState() { return _stateStack.top(); }
    void pushState();
    void popState();

protected:
    /**
   * Generates code for a constructor function.
   */
    virtual int emitCtor(Node *n);

    /**
   * Generates code for a destructor function.
   */
    virtual int emitDtor(Node *n);

    /**
   * Generates code for a function.
   */
    virtual int emitFunction(Node *n, bool is_member, bool is_static);

    virtual int emitFunctionDispatcher(Node *n, bool /*is_member */);

    /**
   * Generates code for a getter function.
   */
    virtual int emitGetter(Node *n, bool is_member, bool is_static);

    /**
   * Generates code for a setter function.
   */
    virtual int emitSetter(Node *n, bool is_member, bool is_static);

    virtual void marshalInputArgs(Node *n, ParmList *parms, Wrapper *wrapper, MarshallingMode mode, bool is_member, bool is_static) = 0;

    virtual String *emitInputTypemap(Node *n, Parm *params, Wrapper *wrapper, String *arg);

    virtual void marshalOutput(Node *n, ParmList *params, Wrapper *wrapper, String *actioncode, const String *cresult = 0, bool emitReturnVariable = true);

    virtual void emitCleanupCode(Node *n, Wrapper *wrapper, ParmList *params);

    /**
   * Helper function to retrieve the first parent class node.
   */
    Node *getBaseClass(Node *n);

    Parm *skipIgnoredArgs(Parm *p);

    virtual int createNamespace(String *scope);

    virtual Hash *createNamespaceEntry(const char *name, const char *parent, const char *parent_mangled);

    virtual int emitNamespaces() = 0;

protected:
    JSEngine engine;
    Hash *templates;
    std::stack<State> _stateStack;
    State* _rootState{};
    bool hasTemplates{};

    // contains context specific data (DOHs)
    // to allow generation of namespace related code
    // which are switched on namespace change
    Hash *namespaces;
    Hash *current_namespace;
    String *defaultResultName;
    String *s_wrappers;
};

/* factory methods for concrete JSEmitters: */
JSEmitter *swig_javascript_create_CocosEmitter();

/**********************************************************************
 * COCOS: SWIG module implementation
 **********************************************************************/

class COCOS : public Language {
public:
    COCOS() : emitter(NULL) {
    }
    ~COCOS() {
        delete emitter;
    }

    virtual int functionHandler(Node *n);
    virtual int globalfunctionHandler(Node *n);

    virtual int variableHandler(Node *n);
    virtual int globalvariableHandler(Node *n);
    virtual int membervariableHandler(Node *n);
    virtual int staticmembervariableHandler(Node *n);

    virtual int staticmemberfunctionHandler(Node *n);
    virtual int classHandler(Node *n);
    virtual int functionWrapper(Node *n);
    virtual int constantWrapper(Node *n);
    virtual int nativeWrapper(Node *n);
    virtual void main(int argc, char *argv[]);
    virtual int top(Node *n);

    virtual NestedClassSupport nestedClassesSupport() const { return NCS_Full; };

    /**
   *  Registers all %fragments assigned to section "templates".
   **/
    virtual int fragmentDirective(Node *n);

    /* SWIG directives */


    virtual int applyDirective(Node *n);
    virtual int clearDirective(Node *n);
    virtual int constantDirective(Node *n);
    virtual int extendDirective(Node *n);
    virtual int importDirective(Node *n);
    virtual int includeDirective(Node *n);
    virtual int insertDirective(Node *n);
    virtual int moduleDirective(Node *n);
    virtual int nativeDirective(Node *n);
    virtual int pragmaDirective(Node *n);
    virtual int typemapDirective(Node *n);
    virtual int typemapcopyDirective(Node *n);
    virtual int typesDirective(Node *n);

public:
    virtual String *getNSpace() const;

private:
    JSEmitter *emitter;
};

/* ---------------------------------------------------------------------
 * functionWrapper()
 *
 * Low level code generator for functions
 * --------------------------------------------------------------------- */

int COCOS::functionWrapper(Node *n) {
    // note: the default implementation only prints a message
    // Language::functionWrapper(n);
    emitter->emitWrapperFunction(n);

    return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * functionHandler()
 *
 * Function handler for generating wrappers for functions
 * --------------------------------------------------------------------- */
int COCOS::functionHandler(Node *n) {
    if (GetFlag(n, "isextension") == 1) {
        SetFlag(n, "ismember");
    }

    emitter->enterFunction(n);
    Language::functionHandler(n);
    emitter->exitFunction(n);

    return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * globalfunctionHandler()
 *
 * Function handler for generating wrappers for functions
 * --------------------------------------------------------------------- */

int COCOS::globalfunctionHandler(Node *n) {
    emitter->switchNamespace(n);
    Language::globalfunctionHandler(n);

    return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * staticmemberfunctionHandler()
 *
 * Function handler for generating wrappers for static member functions
 * --------------------------------------------------------------------- */

int COCOS::staticmemberfunctionHandler(Node *n) {
    /*
   *  Note: storage=static is removed by Language::staticmemberfunctionHandler.
   *    So, don't rely on that after here. Instead use the state variable which is
   *    set by JSEmitter::enterFunction().
   */
    Language::staticmemberfunctionHandler(n);
    return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * variableHandler()
 *
 * Function handler for generating wrappers for variables
 * --------------------------------------------------------------------- */

int COCOS::variableHandler(Node *n) {
    // Add feature:dont_convert_var_to_ptr
    Setattr(n, "feature:dont_convert_var_to_ptr", "1");

    emitter->enterVariable(n);
    Language::variableHandler(n);
    emitter->exitVariable(n);

    return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * globalvariableHandler()
 *
 * Function handler for generating wrappers for global variables
 * --------------------------------------------------------------------- */

int COCOS::globalvariableHandler(Node *n) {
    // Add feature:dont_convert_var_to_ptr
    Setattr(n, "feature:dont_convert_var_to_ptr", "1");
    emitter->switchNamespace(n);
    return Language::globalvariableHandler(n);
}

int COCOS::membervariableHandler(Node *n) {
    // Add feature:dont_convert_var_to_ptr
    Setattr(n, "feature:dont_convert_var_to_ptr", "1");
    return Language::membervariableHandler(n);
}

int COCOS::staticmembervariableHandler(Node *n) {
    // Add feature:dont_convert_var_to_ptr
    Setattr(n, "feature:dont_convert_var_to_ptr", "1");
    return Language::staticmembervariableHandler(n);
}

/* ---------------------------------------------------------------------
 * constantHandler()
 *
 * Function handler for generating wrappers for constants
 * --------------------------------------------------------------------- */

int COCOS::constantWrapper(Node *n) {
    emitter->switchNamespace(n);

    // Note: callbacks trigger this wrapper handler
    // TODO: handle callback declarations
    if (Equal(Getattr(n, "kind"), "function")) {
        return SWIG_OK;
    }
    // TODO: the emitter for constants must be implemented in a cleaner way
    // currently we treat it like a read-only variable
    // however, there is a remaining bug with function pointer constants
    // which could be fixed with a cleaner approach
    emitter->emitConstant(n);

    return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * nativeWrapper()
 *
 * Function wrapper for generating placeholders for native functions
 * --------------------------------------------------------------------- */

int COCOS::nativeWrapper(Node *n) {
    emitter->emitNativeFunction(n);

    return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * classHandler()
 *
 * Function handler for generating wrappers for class
 * --------------------------------------------------------------------- */

int COCOS::classHandler(Node *n) {
    emitter->switchNamespace(n);

    emitter->enterClass(n);
    Language::classHandler(n);
    emitter->exitClass(n);

    return SWIG_OK;
}

int COCOS::fragmentDirective(Node *n) {
    // catch all fragment directives that have "templates" as location
    // and register them at the emitter.
    String *section = Getattr(n, "section");

    if (Equal(section, "templates") && !ImportMode) {
        emitter->registerTemplate(Getattr(n, "value"), Getattr(n, "code"));
    } else {
        return Language::fragmentDirective(n);
    }

    return SWIG_OK;
}

int COCOS::applyDirective(Node *n) {
    return Language::applyDirective(n);
}

int COCOS::clearDirective(Node *n) {
    return Language::clearDirective(n);
}

int COCOS::constantDirective(Node *n) {
    return Language::constantDirective(n);
}

int COCOS::extendDirective(Node *n) {
    return Language::extendDirective(n);
}

int COCOS::importDirective(Node *n) {
    return Language::importDirective(n);
}

int COCOS::includeDirective(Node *n) {
    return Language::includeDirective(n);
}

int COCOS::insertDirective(Node *n) {
    return Language::insertDirective(n);
}

int COCOS::moduleDirective(Node *n) {
    return Language::moduleDirective(n);
}

int COCOS::nativeDirective(Node *n) {
    return Language::nativeDirective(n);
}

int COCOS::pragmaDirective(Node *n) {
    return Language::pragmaDirective(n);
}

int COCOS::typemapDirective(Node *n) {
    return Language::typemapDirective(n);
}

int COCOS::typemapcopyDirective(Node *n) {
    return Language::typemapcopyDirective(n);
}

int COCOS::typesDirective(Node *n) {
    return Language::typesDirective(n);
}


String *COCOS::getNSpace() const {
    return Language::getNSpace();
}

/* ---------------------------------------------------------------------
 * top()
 *
 * Function handler for processing top node of the parse tree
 * Wrapper code generation essentially starts from here
 * --------------------------------------------------------------------- */

int COCOS::top(Node *n) {
    emitter->initialize(n);

    Language::top(n);

    emitter->dump(n);
    emitter->close();

    return SWIG_OK;
}

static const char *usage = (char *)
    "\
Javascript Options (available with -cocos)\n\
     -debug-codetemplates   - generates information about the origin of code templates\n";

/* ---------------------------------------------------------------------
 * main()
 *
 * Entry point for the COCOS module
 * --------------------------------------------------------------------- */

void COCOS::main(int argc, char *argv[]) {
    // Set javascript subdirectory in SWIG library
    SWIG_library_directory("javascript");

    int engine = JSEmitter::Cocos;

    for (int i = 1; i < argc; i++) {
        if (argv[i]) {
            if (strcmp(argv[i], "-debug-codetemplates") == 0) {
                Swig_mark_arg(i);
                js_template_enable_debug = true;
            } else if (strcmp(argv[i], "-help") == 0) {
                fputs(usage, stdout);
                return;
            }
        }
    }

    switch (engine) {
        case JSEmitter::Cocos: {
            emitter = swig_javascript_create_CocosEmitter();
            Preprocessor_define("SWIG_JAVASCRIPT_COCOS 1", 0);
            SWIG_library_directory("javascript/cocos");
            break;
        }
        default: {
            Printf(stderr, "SWIG Javascript: Unknown engine. Please specify one of '-jsc', '-v8' or '-node'.\n");
            Exit(EXIT_FAILURE);
            break;
        }
    }

    // Add a symbol to the parser for conditional compilation
    Preprocessor_define("SWIGCOCOS 1", 0);

    // Add typemap definitions
    SWIG_typemap_lang("javascript");

    // Set configuration file
    SWIG_config_file("javascript.swg");

    allow_overloading();
}

/* -----------------------------------------------------------------------------
 * swig_javascript()    - Instantiate module
 * ----------------------------------------------------------------------------- */

static Language *new_swig_javascript() {
    return new COCOS();
}

extern "C" Language *swig_cocos(void) {
    return new_swig_javascript();
}

/**********************************************************************
 * Emitter implementations
 **********************************************************************/

/* -----------------------------------------------------------------------------
 * JSEmitter()
 * ----------------------------------------------------------------------------- */

JSEmitter::JSEmitter(JSEmitter::JSEngine engine)
: engine(engine), templates(NewHash()), namespaces(NULL), current_namespace(NULL), defaultResultName(NewString("result")), s_wrappers(NULL) {
    pushState();
    _rootState = &currentState();
}

/* -----------------------------------------------------------------------------
 * ~JSEmitter()
 * ----------------------------------------------------------------------------- */

JSEmitter::~JSEmitter() {
    Delete(templates);
}

/* -----------------------------------------------------------------------------
 * JSEmitter::RegisterTemplate() :  Registers a code template
 *
 *  Note: this is used only by COCOS::fragmentDirective().
 * ----------------------------------------------------------------------------- */

int JSEmitter::registerTemplate(const String *name, const String *code) {
    hasTemplates = true;
    return Setattr(templates, name, code);
}

/* -----------------------------------------------------------------------------
 * JSEmitter::getTemplate() :  Provides a registered code template
 * ----------------------------------------------------------------------------- */

Template JSEmitter::getTemplate(const String *name) {
    String *templ = Getattr(templates, name);

    if (!templ) {
        Printf(stderr, "Could not find template %s\n.", name);
        Exit(EXIT_FAILURE);
    }

    Template t(templ, name);
    return t;
}

int JSEmitter::initialize(Node * /*n */) {
    if (namespaces != NULL) {
        Delete(namespaces);
    }
    namespaces = NewHash();
    Hash *global_namespace = createNamespaceEntry("exports", 0, 0);

    Setattr(namespaces, "::", global_namespace);
    current_namespace = global_namespace;

    s_wrappers = NewString("");

    return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * skipIgnoredArgs()
 * --------------------------------------------------------------------- */

Parm *JSEmitter::skipIgnoredArgs(Parm *p) {
    while (checkAttribute(p, "tmap:in:numinputs", "0")) {
        p = Getattr(p, "tmap:in:next");
    }
    return p;
}

/* -----------------------------------------------------------------------------
 * JSEmitter::getBaseClass() :  the node of the base class or NULL
 *
 * Note: the first base class is provided. Multiple inheritance is not
 *       supported.
 * ----------------------------------------------------------------------------- */

Node *JSEmitter::getBaseClass(Node *n) {
    // retrieve the first base class that is not %ignored
    List *baselist = Getattr(n, "bases");
    if (baselist) {
        Iterator base = First(baselist);
        while (base.item && GetFlag(base.item, "feature:ignore")) {
            base = Next(base);
        }
        return base.item;
    }
    return NULL;
}

/* -----------------------------------------------------------------------------
  * JSEmitter::emitWrapperFunction() :  dispatches emitter functions.
  *
  * This allows having small sized, dedicated emitting functions.
  * All state dependent branching is done here.
  * ----------------------------------------------------------------------------- */

int JSEmitter::emitWrapperFunction(Node *n) {
    auto& state = currentState();
    int ret = SWIG_OK;

    String *kind = Getattr(n, "kind");

    if (kind) {
        if (Equal(kind, "function")
            // HACK: sneaky.ctest revealed that typedef'd (global) functions must be
            // detected via the 'view' attribute.
            || (Equal(kind, "variable") && Equal(Getattr(n, "view"), "globalfunctionHandler"))) {
            bool is_member = GetFlag(n, "ismember") != 0 || GetFlag(n, "feature:extend") != 0;
            bool is_static = GetFlag(state.function(), IS_STATIC) != 0;
            ret = emitFunction(n, is_member, is_static);
        } else if (Cmp(kind, "variable") == 0) {
            bool is_static = GetFlag(state.variable(), IS_STATIC) != 0;
            // HACK: smartpointeraccessed static variables are not treated as statics
            if (GetFlag(n, "allocate:smartpointeraccess")) {
                is_static = false;
            }

            bool is_member = GetFlag(n, "ismember") != 0;
            bool is_setter = GetFlag(n, "memberset") != 0 || GetFlag(n, "varset") != 0;
            bool is_getter = GetFlag(n, "memberget") != 0 || GetFlag(n, "varget") != 0;
            if (is_setter) {
                ret = emitSetter(n, is_member, is_static);
            } else if (is_getter) {
                ret = emitGetter(n, is_member, is_static);
            }

        } else {
            Printf(stderr, "Warning: unsupported wrapper function type\n");
            ret = SWIG_ERROR;
        }
    } else {
        String *view = Getattr(n, "view");

        if (Cmp(view, "constructorHandler") == 0) {
            ret = emitCtor(n);
        } else if (Cmp(view, "destructorHandler") == 0) {
            ret = emitDtor(n);
        } else {
            Printf(stderr, "Warning: unsupported wrapper function type");
            ret = SWIG_ERROR;
        }
    }

    return ret;
}

int JSEmitter::emitNativeFunction(Node *n) {
    auto& state = currentState();
    String *wrapname = Getattr(n, "wrap:name");
    enterFunction(n);
    state.function(WRAPPER_NAME, wrapname);
    exitFunction(n);
    return SWIG_OK;
}

int JSEmitter::enterClass(Node *n) {
    auto& state = currentState();
    state.clazz(RESET);
    state.clazz(NAME, Getattr(n, "sym:name"));

    auto* nestClassNameList = createNestClassSymNameList(n);
    if (nestClassNameList != nullptr) {
        state.clazz(NEST_CLASS_NAME_LIST, nestClassNameList);
        Delete(nestClassNameList);
        nestClassNameList = nullptr;
    }

    state.clazz("nspace", current_namespace);

    // Creating a mangled name using the current namespace and the symbol name
    String *mangled_name = NewString("");
    //cjh
    Printf(mangled_name, "%s", Getattr(n, "classtype"));
    convertToMangledName(mangled_name);
    //



    // Printf(mangled_name, "%s_%s", Getattr(current_namespace, NAME_MANGLED), Getattr(n, "sym:name"));

    state.clazz(NAME_MANGLED, mangled_name);
    Delete(mangled_name);

    state.clazz(TYPE, NewString(Getattr(n, "classtype")));

    String *type = SwigType_manglestr(Getattr(n, "classtypeobj"));
    String *classtype_mangled = NewString("");
    Printf(classtype_mangled, "p%s", type);
    state.clazz(TYPE_MANGLED, classtype_mangled);
    Delete(type);

//    String *ctor_wrapper = NewString("js_new_veto_");
//    Append(ctor_wrapper, state.clazz(NAME));
    auto* ctorStr = NewString("nullptr");
    state.clazz(CTOR, ctorStr);
    Delete(ctorStr);

    state.clazz(CTOR_DISPATCHERS, NewString(""));

    // HACK: assume that a class is abstract
    // this is resolved by emitCtor (which is only called for non abstract classes)
    SetFlag(state.clazz(), IS_ABSTRACT);

    /* adds a class registration statement to initializer function */
    Template t_registerclass(getTemplate("jsc_class_registration"));
    t_registerclass.replace("$jsname", state.clazz(NAME))
        .replace("$jsmangledname", state.clazz(NAME_MANGLED))
        .replace("$jsnspace", Getattr(state.clazz("nspace"), NAME_MANGLED))
        .pretty_print(_rootState->globals(REGISTER_CLASSES));

    return SWIG_OK;
}

int JSEmitter::enterFunction(Node *n) {
    auto& state = currentState();
    state.function(RESET);
    state.function(NAME, Getattr(n, "sym:name"));
    if (Equal(Getattr(n, "storage"), "static")) {
        SetFlag(state.function(), IS_STATIC);
    }
    return SWIG_OK;
}

int JSEmitter::enterVariable(Node *n) {
    auto& state = currentState();
    // reset the state information for variables.
    state.variable(RESET);

    // Retrieve a pure symbol name. Using 'sym:name' as a basis, as it considers %renamings.
    if (Equal(Getattr(n, "view"), "memberconstantHandler")) {
        // Note: this is kind of hacky/experimental
        // For constants/enums 'sym:name' contains e.g., 'Foo_Hello' instead of 'Hello'
        state.variable(NAME, Getattr(n, "memberconstantHandler:sym:name"));
    } else {
        state.variable(NAME, Swig_scopename_last(Getattr(n, "sym:name")));
    }

    if (Equal(Getattr(n, "storage"), "static")) {
        SetFlag(state.variable(), IS_STATIC);
    }

    if (!Language::instance()->is_assignable(n)) {
        SetFlag(state.variable(), IS_IMMUTABLE);
    }

    if (GetFlag(n, "feature:writeonly")) {
        SetFlag(state.variable(), IS_WRITE_ONLY);
    }

    // FIXME: test "arrays_global" does not compile with that as it is not allowed to assign to char[]
    if (Equal(Getattr(n, "type"), "a().char")) {
        SetFlag(state.variable(), IS_IMMUTABLE);
    }

    return SWIG_OK;
}

void JSEmitter::pushState() {
    _stateStack.emplace();

    auto& state = currentState();
    state.globals(CREATE_NAMESPACES, NewString(""));
    state.globals(REGISTER_NAMESPACES, NewString(""));
    state.globals(INITIALIZER, NewString(""));
    state.globals(REGISTER_CLASSES, NewString(""));
    state.globals(REGISTER_GLOBAL, NewString(""));
    state.globals(HEADER_REGISTER_MODULE, NewString(""));
    state.globals(HEADER_REGISTER_CLASSES, NewString(""));
}

void JSEmitter::popState() {
    _stateStack.pop();
}

int JSEmitter::emitCtor(Node *n) {
    auto& state = currentState();
    Wrapper *wrapper = NewWrapper();

    bool is_overloaded = GetFlag(n, "sym:overloaded") != 0;

    Template t_ctor(getTemplate("js_ctor"));

    auto * classNode = getClassNode(n);
    auto* symName = Copy(Getattr(classNode, "classtype"));
    convertToMangledName(symName);
    auto* dtorSymName = Copy(symName);
    Insert(symName, 0, "new_");

    String *wrap_name = Swig_name_wrapper(symName);
    Delete(symName);
    symName = nullptr;
    
    if (is_overloaded) {
        t_ctor = getTemplate("js_overloaded_ctor");
        Append(wrap_name, Getattr(n, "sym:overname"));
    }
    Setattr(n, "wrap:name", wrap_name);
    // note: we can remove the is_abstract flag now, as this
    //       is called for non-abstract classes only.
    Setattr(state.clazz(), IS_ABSTRACT, 0);

    ParmList *params = Getattr(n, "parms");
    emit_parameter_variables(params, wrapper);
    emit_attach_parmmaps(params, wrapper);
    // HACK: in test-case `ignore_parameter` emit_attach_parmmaps generated an extra line of applied typemaps.
    // Deleting wrapper->code here, to reset, and as it seemed to have no side effect elsewhere
    Delete(wrapper->code);
    wrapper->code = NewString("");

    Printf(wrapper->locals, "%sresult;", SwigType_str(Getattr(n, "type"), 0));

    marshalInputArgs(n, params, wrapper, Ctor, true, false);
    String *action = emit_action(n);
    Printv(wrapper->code, action, "\n", 0);

    emitCleanupCode(n, wrapper, params);


    t_ctor.replace("$jswrapper", wrap_name)
        .replace("$jsmangledtype", state.clazz(TYPE_MANGLED))
        .replace("$jsmangledname", state.clazz(NAME_MANGLED))
        .replace("$jsname", state.clazz(NAME))
        .replace("$jsdtor", dtorSymName)
        .replace("$jslocals", wrapper->locals)
        .replace("$jscode", wrapper->code)
        .replace("$jsargcount", Getattr(n, ARGCOUNT))
        .pretty_print(s_wrappers);

    Template t_ctor_case(getTemplate("js_ctor_dispatch_case"));
    t_ctor_case.replace("$jswrapper", wrap_name)
        .replace("$jsargcount", Getattr(n, ARGCOUNT));
    Append(state.clazz(CTOR_DISPATCHERS), t_ctor_case.str());

    DelWrapper(wrapper);

    Clear(state.clazz(CTOR));
    // create a dispatching ctor
    if (is_overloaded) {
        if (!Getattr(n, "sym:nextSibling")) {
            String *wrap_name = Swig_name_wrapper(Getattr(n, "sym:name"));
            Template t_mainctor(getTemplate("js_ctor_dispatcher"));
            t_mainctor.replace("$jswrapper", wrap_name)
                .replace("$jsmangledname", state.clazz(NAME_MANGLED))
                .replace("$jsname", state.clazz(NAME))
                .replace("$jsdtor", dtorSymName)
                .replace("$jsdispatchcases", state.clazz(CTOR_DISPATCHERS))
                .pretty_print(s_wrappers);

            Printf(state.clazz(CTOR), "_SE(%s)", wrap_name);
        }
    } else {
        Printf(state.clazz(CTOR), "_SE(%s)", wrap_name);
    }

    Delete(dtorSymName);

    return SWIG_OK;
}

int JSEmitter::emitDtor(Node *n) {
    auto& state = currentState();

    auto * classNode = getClassNode(n);
    auto* symName = Copy(Getattr(classNode, "classtype"));

    convertToMangledName(symName);
    Insert(symName, 0, "delete_");

    String *wrap_name = Swig_name_wrapper(symName);
    Delete(symName);
    symName = nullptr;

    SwigType *type = state.clazz(TYPE);
    String *p_classtype = SwigType_add_pointer(state.clazz(TYPE));
    String *ctype = SwigType_lstr(p_classtype, "");
    String *jsfree = NewString("");

    // (Taken from JSCore implementation.)
    /* The if (Extend) block was taken from the Ruby implementation.
   * The problem is that in the case of an %extend to create a destructor for a struct to coordinate automatic memory cleanup with the Javascript collector,
   * the SWIG function was not being generated. More specifically:
   struct MyData {
   %extend {
   ~MyData() {
   FreeData($self);
   }
   }
   };
   %newobject CreateData;
   struct MyData* CreateData(void);
   %delobject FreeData;
   void FreeData(struct MyData* the_data);

   where the use case is something like:
   var my_data = example.CreateData();
   my_data = null;

   This function was not being generated:
   SWIGINTERN void delete_MyData(struct MyData *self){
   FreeData(self);
   }

   I don't understand fully why it wasn't being generated. It just seems to happen in the Lua generator.
   There is a comment about staticmemberfunctionHandler having an inconsistency and I tracked down dome of the SWIGINTERN void delete_*
   code to that function in the Language base class.
   The Ruby implementation seems to have an explicit check for if(Extend) and explicitly generates the code, so that's what I'm doing here.
   The Ruby implementation does other stuff which I omit.
   */
    if (Extend) {
        String *wrap = Getattr(n, "wrap:code");
        if (wrap) {
            Printv(s_wrappers, wrap, NIL);
        }
    }
    // HACK: this is only for the v8 emitter. maybe set an attribute wrap:action of node
    // TODO: generate dtors more similar to other wrappers
    // EW: I think this is wrong. delete should only be used when new was used to create. If malloc was used, free needs to be used.
    if (SwigType_isarray(type)) {
        Printf(jsfree, "delete [] (%s)", ctype);
    } else {
        Printf(jsfree, "delete (%s)", ctype);
    }

    String *destructor_action = Getattr(n, "wrap:action");
    // Adapted from the JSCore implementation.
    /* The next challenge is to generate the correct finalize function for JavaScriptCore to call.
     Originally, it would use this fragment from javascriptcode.swg
     %fragment ("JS_destructordefn", "templates")
     %{
     void _wrap_${classname_mangled}_finalize(JSObjectRef thisObject)
     {
     SWIG_PRV_DATA* t = (SWIG_PRV_DATA*)JSObjectGetPrivate(thisObject);
     if(t && t->swigCMemOwn) free ((${type}*)t->swigCObject);
     free(t);
     }
     %}

     But for the above example case of %extend to define a destructor on a struct, we need to override the system to not call
     free ((${type}*)t->swigCObject);
     and substitute it with what the user has provided.
     To solve this, I created a variation fragment called JS_destructoroverridedefn:
     SWIG_PRV_DATA* t = (SWIG_PRV_DATA*)JSObjectGetPrivate(thisObject);
     if(t && t->swigCMemOwn) {
     ${type}* arg1 = (${type}*)t->swigCObject;
     ${destructor_action}
     }
     free(t);

     Based on what I saw in the Lua and Ruby modules, I use Getattr(n, "wrap:action")
     to decide if the user has a preferred destructor action.
     Based on that, I decide which fragment to use.
     And in the case of the custom action, I substitute that action in.
     I noticed that destructor_action has the form
     delete_MyData(arg1);
     The explicit arg1 is a little funny, so I structured the fragment to create a temporary variable called arg1 to make the generation easier.
     This might suggest this solution misunderstands a more complex case.

     Also, there is a problem where destructor_action is always true for me, even when not requesting %extend as above.
     So this code doesn't actually quite work as I expect. The end result is that the code still works because
     destructor_action calls free like the original template. The one caveat is the string in destructor_action casts to char* which is weird.
     I think there is a deeper underlying SWIG issue because I don't think it should be char*. However, it doesn't really matter for free.

     Maybe the fix for the destructor_action always true problem is that this is supposed to be embedded in the if(Extend) block above.
     But I don't fully understand the conditions of any of these things, and since it works for the moment, I don't want to break more stuff.
   */
    if (destructor_action) {
        Template t_dtor = getTemplate("js_dtoroverride");
        state.clazz(DTOR, wrap_name);
        t_dtor.replace("${classname_mangled}", state.clazz(NAME_MANGLED))
            .replace("$jswrapper", wrap_name)
            .replace("$jsfree", jsfree)
            .replace("$jstype", ctype);

        t_dtor.replace("${destructor_action}", destructor_action);
        Wrapper_pretty_print(t_dtor.str(), s_wrappers);
    } else {
        Template t_dtor = getTemplate("js_dtor");
        state.clazz(DTOR, wrap_name);
        t_dtor.replace("$jsmangledname", state.clazz(NAME_MANGLED))
            .replace("$jswrapper", wrap_name)
            .replace("$jsfree", jsfree)
            .replace("$jstype", ctype)
            .pretty_print(s_wrappers);
    }

    Delete(p_classtype);
    Delete(ctype);
    Delete(jsfree);

    return SWIG_OK;
}

static String* getVariableWrapName(Node* n, bool isGetter) {
    String *wrap_name = nullptr;
    auto* varWrapperName = Copy(Getattr(n, "variableWrapper:name"));
    if (varWrapperName) {
        convertToMangledName(varWrapperName);
        wrap_name = Swig_name_wrapper(varWrapperName);
        if (isGetter) {
            Append(wrap_name, "_get");
        }
        else {
            Append(wrap_name, "_set");
        }

        Delete(varWrapperName);
        varWrapperName = nullptr;
    }
    else {
        auto* symName = Copy(Getattr(n, "sym:name"));
        auto namespaceNameArray = getNamespaceNameArray(n);
        if (!namespaceNameArray.empty()) {
            std::string namespaceName = join(namespaceNameArray, "_");
            namespaceName.append("_");
            Insert(symName, 0, namespaceName.c_str());
        }

        wrap_name = Swig_name_wrapper(symName);

        Delete(symName);
    }

    return wrap_name;
}

int JSEmitter::emitGetter(Node *n, bool is_member, bool is_static) {
    auto& state = currentState();
    // skip variables that are writeonly
    if (State::IsSet(state.variable(IS_WRITE_ONLY))) {
        return SWIG_OK;
    }

    int isextendmember = GetFlag(n, "isextendmember");
    int is_global = !is_member && !is_static;

    Wrapper *wrapper = NewWrapper();
    Template t_getter(getTemplate("js_getter"));

    // prepare wrapper name
    String* wrap_name = getVariableWrapName(n, true);
    Setattr(n, "wrap:name", wrap_name);

    Clear(state.variable(GETTER));
    Printf(state.variable(GETTER), "_SE(%s)", wrap_name);

    // prepare local variables
    ParmList *params = Getattr(n, "parms");

    emit_parameter_variables(params, wrapper);
    emit_attach_parmmaps(params, wrapper);

    // prepare code part
    String *action = (isextendmember || is_static || is_global) ? emit_action(n) : NewStringEmpty();
    marshalInputArgs(n, params, wrapper, Getter, is_member, is_static);
    String* prop = (!isextendmember && !is_static && !is_global) ? NewStringf("arg1->%s", Getattr(n, "name")) : nullptr;

    marshalOutput(n, params, wrapper, action, prop, isextendmember || is_static || is_global); // don't emit result value
    Delete(action);
    Delete(prop);

    emitCleanupCode(n, wrapper, params);

    t_getter.replace("$jswrapper", wrap_name)
        .replace("$jslocals", wrapper->locals)
        .replace("$jscode", wrapper->code)
        .pretty_print(s_wrappers);

    DelWrapper(wrapper);
    Delete(wrap_name);

    return SWIG_OK;
}

int JSEmitter::emitSetter(Node *n, bool is_member, bool is_static) {
    auto& state = currentState();
    // skip variables that are immutable
    if (State::IsSet(state.variable(IS_IMMUTABLE))) {
        return SWIG_OK;
    }

    int isextendmember = GetFlag(n, "isextendmember");

    Wrapper *wrapper = NewWrapper();

    Template t_setter(getTemplate("js_setter"));

    // prepare wrapper name
    String* wrap_name = getVariableWrapName(n, false);
    Setattr(n, "wrap:name", wrap_name);
    Clear(state.variable(SETTER));
    Printf(state.variable(SETTER), "_SE(%s)", wrap_name);

    // prepare local variables
    ParmList *params = Getattr(n, "parms");
    int paramCount = getParamListCount(params);

    ParmList *value = nullptr;
    if (paramCount > 1) {
        value = nextSibling(params);
        Swig_print(Getattr(value, "type"));
    }

    if (!isextendmember && value != nullptr) {
        auto* prop = NewStringf("arg1->%s", Getattr(value, "name"));
        Setattr(value, "lname", prop);
        Delete(prop);
        DohIncref(value);
        set_nextSibling(params, nullptr);
    }

    emit_parameter_variables(params, wrapper);

    if (!isextendmember && value != nullptr) {
        set_nextSibling(params, value);
        Delete(value);
    }
    emit_attach_parmmaps(params, wrapper);

    // prepare code part
    String *action = emit_action(n);
    marshalInputArgs(n, params, wrapper, Setter, is_member, is_static);
    if (isextendmember || value == nullptr) { //cjh added
        Append(wrapper->code, action);
    }

    emitCleanupCode(n, wrapper, params);

    t_setter.replace("$jswrapper", wrap_name)
        .replace("$jslocals", wrapper->locals)
        .replace("$jscode", wrapper->code)
        .pretty_print(s_wrappers);

    DelWrapper(wrapper);

    return SWIG_OK;
}

/* -----------------------------------------------------------------------------
 * JSEmitter::emitConstant() :  triggers code generation for constants
 * ----------------------------------------------------------------------------- */

int JSEmitter::emitConstant(Node *n) {
    auto& state = currentState();
    // HACK: somehow it happened under Mac OS X that before everything started
    // a lot of SWIG internal constants were emitted
    // This didn't happen on other platforms yet...
    // we ignore those premature definitions
    if (!State::IsSet(state.globals(HAS_TEMPLATES))) {
        return SWIG_ERROR;
    }

    Wrapper *wrapper = NewWrapper();
    SwigType *type = Getattr(n, "type");
    String *name = Getattr(n, "name");
    String *iname = Getattr(n, "sym:name");
    String *wname = Swig_name_wrapper(name);
    String *rawval = Getattr(n, "rawval");
    String *value = rawval ? rawval : Getattr(n, "value");

    // HACK: forcing usage of cppvalue for v8 (which turned out to fix typedef_struct.i, et. al)
    if (State::IsSet(state.globals(FORCE_CPP)) && Getattr(n, "cppvalue") != NULL) {
        value = Getattr(n, "cppvalue");
    }

    Template t_getter(getTemplate("js_getter"));

    // call the variable methods as a constants are
    // registered in same way
    enterVariable(n);
    Clear(state.variable(GETTER));
    Swig_print(wname, -1);
    Printf(state.variable(GETTER), "_SE(%s)", wname);
    // TODO: why do we need this?
    Setattr(n, "wrap:name", wname);

    // special treatment of member pointers
    if (SwigType_type(type) == T_MPOINTER) {
        // TODO: this could go into a code-template
        String *mpointer_wname = NewString("");
        Printf(mpointer_wname, "_wrapConstant_%s", iname);
        Setattr(n, "memberpointer:constant:wrap:name", mpointer_wname);
        String *str = SwigType_str(type, mpointer_wname);
        Printf(s_wrappers, "static %s = %s;\n", str, value);
        Delete(str);
        value = mpointer_wname;
    }

    marshalOutput(n, 0, wrapper, NewString(""), value, false);

    t_getter.replace("$jswrapper", wname)
        .replace("$jslocals", wrapper->locals)
        .replace("$jscode", wrapper->code)
        .pretty_print(s_wrappers);

    exitVariable(n);

    DelWrapper(wrapper);

    return SWIG_OK;
}

int JSEmitter::emitFunction(Node *n, bool is_member, bool is_static) {
    auto& state = currentState();
    Wrapper *wrapper = NewWrapper();
    Template t_function(getTemplate("js_function"));

    bool is_overloaded = GetFlag(n, "sym:overloaded") != 0;

    // prepare the function wrapper name
    String *iname = Copy(Getattr(n, "sym:name"));

    auto namespaceNameArray = getNamespaceNameArray(n);
    if (!namespaceNameArray.empty()) {
        std::string namespaceName = join(namespaceNameArray, "_");
        Insert(iname, 0, "_");
        Insert(iname, 0, namespaceName.c_str());
    }

    if (is_member) {
        if (is_static) {
            Append(iname, "_static");
        }
    }

    String *wrap_name = Swig_name_wrapper(iname);

    if (is_overloaded) {
        t_function = getTemplate("js_overloaded_function");
        Append(wrap_name, Getattr(n, "sym:overname"));
    }

    Setattr(n, "wrap:name", wrap_name);
    state.function(WRAPPER_NAME, wrap_name);

    // prepare local variables
    ParmList *params = Getattr(n, "parms");
    emit_parameter_variables(params, wrapper);
    emit_attach_parmmaps(params, wrapper);

    // HACK: in test-case `ignore_parameter` emit_attach_parmmaps generates an extra line of applied typemap.
    // Deleting wrapper->code here fixes the problem, and seems to have no side effect elsewhere
    Delete(wrapper->code);
    wrapper->code = NewString("");

    marshalInputArgs(n, params, wrapper, Function, is_member, is_static);
    String *action = emit_action(n);
    marshalOutput(n, params, wrapper, action);
    emitCleanupCode(n, wrapper, params);
    Replaceall(wrapper->code, "$symname", iname);

    t_function.replace("$jswrapper", wrap_name)
        .replace("$jslocals", wrapper->locals)
        .replace("$jscode", wrapper->code)
        .replace("$jsargcount", Getattr(n, ARGCOUNT))
        .pretty_print(s_wrappers);

    DelWrapper(wrapper);

    Delete(iname);
    Delete(wrap_name);

    return SWIG_OK;
}

int JSEmitter::emitFunctionDispatcher(Node *n, bool /*is_member */) {
    auto& state = currentState();
    Wrapper *wrapper = NewWrapper();

    // Generate call list, go to first node
    Node *sibl = n;

    while (Getattr(sibl, "sym:previousSibling"))
        sibl = Getattr(sibl, "sym:previousSibling"); // go all the way up

    do {
        String *siblname = Getattr(sibl, "wrap:name");

        if (siblname) {
            // handle function overloading
            Template t_dispatch_case = getTemplate("js_function_dispatch_case");
            t_dispatch_case.replace("$jswrapper", siblname)
                .replace("$jsargcount", Getattr(sibl, ARGCOUNT));

            Append(wrapper->code, t_dispatch_case.str());
        }

    } while ((sibl = Getattr(sibl, "sym:nextSibling")));

    Template t_function(getTemplate("js_function_dispatcher"));

    // Note: this dispatcher function gets called after the last overloaded function has been created.
    // At this time, n.wrap:name contains the name of the last wrapper function.
    // To get a valid function name for the dispatcher function we take the last wrapper name and
    // subtract the extension "sym:overname",
    String *wrap_name = NewString(Getattr(n, "wrap:name"));
    String *overname = Getattr(n, "sym:overname");

    Node *methodclass = Swig_methodclass(n);
    String *class_name = Getattr(methodclass, "sym:name");

    int l1 = Len(wrap_name);
    int l2 = Len(overname);
    Delslice(wrap_name, l1 - l2, l1);

    Setattr(n, "wrap:name", wrap_name);
    state.function(WRAPPER_NAME, wrap_name);

    t_function.replace("$jslocals", wrapper->locals)
        .replace("$jscode", wrapper->code);

    // call this here, to replace all variables
    t_function.replace("$jswrapper", wrap_name)
        .replace("$jsname", state.function(NAME))
        .pretty_print(s_wrappers);

    // Delete the state variable
    DelWrapper(wrapper);
    Delete(wrap_name);

    return SWIG_OK;
}

String *JSEmitter::emitInputTypemap(Node *n, Parm *p, Wrapper *wrapper, String *arg) {
    // Get input typemap for current param
    String *tm = Getattr(p, "tmap:in");
    SwigType *type = Getattr(p, "type");

    if (tm != NULL) {
        Replaceall(tm, "$input", arg);
        Setattr(p, "emit:input", arg);
        // do replacements for built-in variables
        if (Getattr(p, "wrap:disown") || (Getattr(p, "tmap:in:disown"))) {
            Replaceall(tm, "$disown", "SWIG_POINTER_DISOWN");
        } else {
            Replaceall(tm, "$disown", "0");
        }
        Replaceall(tm, "$symname", Getattr(n, "sym:name"));
        Printf(wrapper->code, "%s\n", tm);
    } else {
        Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, "Unable to use type %s as a function argument.\n", SwigType_str(type, 0));
    }

    return tm;
}

void JSEmitter::marshalOutput(Node *n, ParmList *params, Wrapper *wrapper, String *actioncode, const String *cresult, bool emitReturnVariable) {
    SwigType *type = Getattr(n, "type");
    String *tm;
    Parm *p;

    // adds a declaration for the result variable
    if (emitReturnVariable)
        emit_return_variable(n, type, wrapper);
    // if not given, use default result identifier ('result') for output typemap
    if (cresult == 0)
        cresult = defaultResultName;

    tm = Swig_typemap_lookup_out("out", n, cresult, wrapper, actioncode);
    bool should_own = GetFlag(n, "feature:new") != 0;

    if (tm) {
        Replaceall(tm, "$objecttype", Swig_scopename_last(SwigType_str(SwigType_strip_qualifiers(type), 0)));

        if (should_own) {
            Replaceall(tm, "$owner", "SWIG_POINTER_OWN");
        } else {
            Replaceall(tm, "$owner", "0");
        }
        Append(wrapper->code, tm);

        if (Len(tm) > 0) {
            Printf(wrapper->code, "\n");
        }
    } else {
        Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to use return type %s in function %s.\n", SwigType_str(type, 0), Getattr(n, "name"));
    }

    if (params) {
        for (p = params; p;) {
            if ((tm = Getattr(p, "tmap:argout"))) {
                Replaceall(tm, "$input", Getattr(p, "emit:input"));
                Printv(wrapper->code, tm, "\n", NIL);
                p = Getattr(p, "tmap:argout:next");
            } else {
                p = nextSibling(p);
            }
        }
    }

    Replaceall(wrapper->code, "$result", "jsresult");
}

void JSEmitter::emitCleanupCode(Node *n, Wrapper *wrapper, ParmList *params) {
    Parm *p;
    String *tm;

    for (p = params; p;) {
        if ((tm = Getattr(p, "tmap:freearg"))) {
            //addThrows(n, "tmap:freearg", p);
            Replaceall(tm, "$input", Getattr(p, "emit:input"));
            Printv(wrapper->code, tm, "\n", NIL);
            p = Getattr(p, "tmap:freearg:next");
        } else {
            p = nextSibling(p);
        }
    }

    if (GetFlag(n, "feature:new")) {
        tm = Swig_typemap_lookup("newfree", n, Swig_cresult_name(), 0);
        if (tm != NIL) {
            //addThrows(throws_hash, "newfree", n);
            Printv(wrapper->code, tm, "\n", NIL);
        }
    }

    /* See if there is any return cleanup code */
    if ((tm = Swig_typemap_lookup("ret", n, Swig_cresult_name(), 0))) {
        Printf(wrapper->code, "%s\n", tm);
        Delete(tm);
    }
}

int JSEmitter::switchNamespace(Node *n) {
    // HACK: somehow this gets called for member functions.
    // We can safely ignore them, as members are not associated to a namespace (only their class)
    if (GetFlag(n, "ismember")) {
        return SWIG_OK;
    }

    // if nspace is deactivated, everything goes into the global scope
    if (!GetFlag(n, "feature:nspace")) {
        current_namespace = Getattr(namespaces, "::");
        return SWIG_OK;
    }

// EXPERIMENTAL: we want to use Language::getNSpace() here
// However, it is not working yet.
// For namespace functions Language::getNSpace() does not give a valid result
#if 0
  COCOS *lang = static_cast<COCOS*>(Language::instance());
  String *_nspace = lang->getNSpace();
  if (!Equal(nspace, _nspace)) {
    Printf(stdout, "##### Custom vs Language::getNSpace(): %s | %s\n", nspace, _nspace);
  }
#endif

    String *nspace = Getattr(n, "sym:nspace");

    if (nspace == NULL) {
        // It seems that only classes have 'sym:nspace' set.
        // We try to get the namespace from the qualified name (i.e., everything before the last '::')
        nspace = Swig_scopename_prefix(Getattr(n, "name"));
    }

    // If there is not even a scopename prefix then it must be global scope
    if (nspace == NULL) {
        current_namespace = Getattr(namespaces, "::");
        return SWIG_OK;
    }

    String *scope = NewString(nspace);
    // replace "." with "::" that we can use Swig_scopename_last
    Replaceall(scope, ".", "::");

    // if the scope is not yet registered
    // create (parent) namespaces recursively
    if (!Getattr(namespaces, scope)) {
        createNamespace(scope);
    }
    current_namespace = Getattr(namespaces, scope);

    return SWIG_OK;
}

int JSEmitter::createNamespace(String *scope) {
    String *parent_scope = Swig_scopename_prefix(scope);
    Hash *parent_namespace;
    if (parent_scope == 0) {
        parent_namespace = Getattr(namespaces, "::");
    } else if (!Getattr(namespaces, parent_scope)) {
        createNamespace(parent_scope);
        parent_namespace = Getattr(namespaces, parent_scope);
    } else {
        parent_namespace = Getattr(namespaces, parent_scope);
    }
    assert(parent_namespace != 0);

    Hash *new_namespace = createNamespaceEntry(Char(scope), Char(Getattr(parent_namespace, "name")), Char(Getattr(parent_namespace, "name_mangled")));
    Setattr(namespaces, scope, new_namespace);

    Delete(parent_scope);
    return SWIG_OK;
}

Hash *JSEmitter::createNamespaceEntry(const char *_name, const char *parent, const char *parent_mangled) {
    Hash *entry = NewHash();
    String *name = NewString(_name);
    Setattr(entry, NAME, Swig_scopename_last(name));
    Setattr(entry, NAME_MANGLED, Swig_name_mangle(name));
    Setattr(entry, PARENT, NewString(parent));
    Setattr(entry, PARENT_MANGLED, NewString(parent_mangled));

    Delete(name);
    return entry;
}

/**********************************************************************
 * Cocos: JSEmitter implementation for Cocos script engine
 **********************************************************************/

class CocosEmitter : public JSEmitter {
public:
    CocosEmitter();
    virtual ~CocosEmitter();
    virtual int initialize(Node *n);
    virtual int dump(Node *n);
    virtual int close();

protected:
    virtual int enterVariable(Node *n);
    virtual int exitVariable(Node *n);
    virtual int enterFunction(Node *n);
    virtual int exitFunction(Node *n);
    virtual int enterClass(Node *n);
    virtual int exitClass(Node *n);
    virtual void marshalInputArgs(Node *n, ParmList *parms, Wrapper *wrapper, MarshallingMode mode, bool is_member, bool is_static);
    virtual Hash *createNamespaceEntry(const char *name, const char *parent, const char *parent_mangled);
    virtual int emitNamespaces();

private:
    String *VETO_SET{};
    String *VETO_GET{};

    // output file and major code parts
    File *f_wrap_cpp{};
    File *f_wrap_h{};
    String *s_runtime{};
    String *s_header{};
    String *s_header_file{};
    String *s_init{};
    String *s_targetNamespace{};
};

CocosEmitter::CocosEmitter()
: JSEmitter(JSEmitter::Cocos), VETO_GET(NewString("nullptr")), VETO_SET(NewString("nullptr")) {
}

CocosEmitter::~CocosEmitter() {
    Delete(VETO_GET);
    Delete(VETO_SET);
}

/* ---------------------------------------------------------------------
 * marshalInputArgs()
 *
 * Process all of the arguments passed into the argv array
 * and convert them into C/C++ function arguments using the
 * supplied typemaps.
 * --------------------------------------------------------------------- */

void CocosEmitter::marshalInputArgs(Node *n, ParmList *parms, Wrapper *wrapper, MarshallingMode mode, bool is_member, bool is_static) {
    Parm *p;
    String *tm;

    // determine an offset index, as members have an extra 'this' argument
    // except: static members and ctors.
    int startIdx = 0;
    if (is_member && !is_static && mode != Ctor) {
        startIdx = 1;
    }
    // store number of arguments for argument checks
    int num_args = emit_num_arguments(parms) - startIdx;
    String *argcount = NewString("");
    Printf(argcount, "%d", num_args);
    Setattr(n, ARGCOUNT, argcount);

    // process arguments
    int i = 0;
    for (p = parms; p; i++) {
        String *arg = NewString("");
        String *type = Getattr(p, "type");

        // ignore varargs
        if (SwigType_isvarargs(type))
            break;

        switch (mode) {
            case Getter:
            case Function:
                if (is_member && !is_static && i == 0) {
                    Printv(arg, "args[0]", 0);
                } else {
                    Printf(arg, "args[%d]", i - startIdx);
                }
                break;
            case Setter:
                if (is_member && !is_static && i == 0) {
                    Printv(arg, "args[0]", 0);
                } else {
                    Printv(arg, "args[0]", 0);
                }
                break;
            case Ctor:
                Printf(arg, "args[%d]", i);
                break;
            default:
                Printf(stderr, "Illegal MarshallingMode.");
                Exit(EXIT_FAILURE);
        }
        tm = emitInputTypemap(n, p, wrapper, arg);
        Delete(arg);
        if (tm) {
            p = Getattr(p, "tmap:in:next");
        } else {
            p = nextSibling(p);
        }
    }

    /* Insert constraint checking code */
    for (p = parms; p;) {
        if ((tm = Getattr(p, "tmap:check"))) {
            Printv(wrapper->code, tm, "\n", NIL);
            p = Getattr(p, "tmap:check:next");
        } else {
            p = nextSibling(p);
        }
    }
}

int CocosEmitter::initialize(Node *n) {
    JSEmitter::initialize(n);

    // Get any options set in the module directive
    Node *moduleNode = Getattr(n, "module");
    Node *optionsnode = Getattr(moduleNode, "options");
    if (optionsnode != nullptr) {
        if (Getattr(optionsnode, "target_namespace")) {
            s_targetNamespace = Copy(Getattr(optionsnode, "target_namespace"));
        }
    }

    if (s_targetNamespace == nullptr) {
        s_targetNamespace = Copy(Getattr(moduleNode, "name"));
    }

    Swig_name_register("wrapper", "js_%f");

    /* Get the output file name */
    String *outfile = Getattr(n, "outfile");
    String *outfile_h = Getattr(n, "outfile_h");

    /* Initialize I/O */
    f_wrap_cpp = NewFile(outfile, "w", SWIG_output_files());
    if (!f_wrap_cpp) {
        FileErrorDisplay(outfile);
        Exit(EXIT_FAILURE);
    }

    f_wrap_h = NewFile(outfile_h, "w", SWIG_output_files());
    if (!f_wrap_h) {
        FileErrorDisplay(outfile_h);
        Exit(EXIT_FAILURE);
    }

    /* Initialization of members */
    s_runtime = NewString("");
    s_init = NewString("");
    s_header = NewString("");
    s_header_file = NewString("");

    /* Register file targets with the SWIG file handler */
    Swig_register_filebyname("begin", f_wrap_cpp);
    Swig_register_filebyname("header", s_header);
    Swig_register_filebyname("wrapper", s_wrappers);
    Swig_register_filebyname("runtime", s_runtime);
    Swig_register_filebyname("init", s_init);
    Swig_register_filebyname("header_file", s_header_file);

    Swig_banner(f_wrap_cpp);
    Swig_banner(f_wrap_h);

    return SWIG_OK;
}

int CocosEmitter::dump(Node *n) {
    auto& state = currentState();
    /* Get the module name */
    String *module = Getattr(n, "name");

    //  Template initializer_define(getTemplate("js_initializer_define"));
    //  initializer_define.replace("$jsname", module).pretty_print(s_header);

    //cjh  SwigType_emit_type_table(s_runtime, s_wrappers);

    Printv(f_wrap_cpp, s_runtime, "\n", 0);
    Printv(f_wrap_cpp, s_header, "\n", 0);
    Printv(f_wrap_cpp, s_wrappers, "\n", 0);
    Printv(f_wrap_h, s_header_file, "\n", 0);

    emitNamespaces();

    // compose the initializer function using a template
    Template initializer(getTemplate("js_initializer"));
    initializer.replace("$js_module_name", module)
        .replace("$js_namespace", s_targetNamespace)
        .replace("$jsregisterclasses", state.globals(REGISTER_CLASSES))
        .replace("$jsregisternamespaces", state.globals(REGISTER_NAMESPACES))
        .pretty_print(s_init);

    Printv(f_wrap_cpp, s_init, 0);

    {
        Template template_module_declare(getTemplate("js_register_module_declare"));
        template_module_declare.replace("$jsmodulename", module)
            .pretty_print(f_wrap_h);
    }

    Printv(f_wrap_h, state.globals(HEADER_REGISTER_CLASSES), 0);

    return SWIG_OK;
}

int CocosEmitter::close() {
    Delete(s_runtime);
    Delete(s_header);
    Delete(s_header_file);
    Delete(s_wrappers);
    Delete(s_init);
    Delete(namespaces);
    Delete(f_wrap_cpp);
    Delete(f_wrap_h);
    Delete(s_targetNamespace);
    return SWIG_OK;
}

int CocosEmitter::enterFunction(Node *n) {
    JSEmitter::enterFunction(n);

    return SWIG_OK;
}

int CocosEmitter::exitFunction(Node *n) {
    auto& state = currentState();
    bool is_member = GetFlag(n, "ismember") != 0 || GetFlag(n, "feature:extend") != 0;
    bool is_overloaded = GetFlag(n, "sym:overloaded") != 0;

    // handle overloaded functions
    if (is_overloaded) {
        if (!Getattr(n, "sym:nextSibling")) {
            //state.function(WRAPPER_NAME, Swig_name_wrapper(Getattr(n, "name")));
            // create dispatcher
            emitFunctionDispatcher(n, is_member);
        } else {
            //don't register wrappers of overloaded functions in function tables
            return SWIG_OK;
        }
    }

    std::string jsname = fixCppKeyword(Char(state.function(NAME)));

    if (is_member) {
        if (GetFlag(state.function(), IS_STATIC)) {
            Template t_static_function = getTemplate("jsc_static_function_declaration");
            t_static_function.replace("$jsname", jsname.c_str())
                .replace("$jswrapper", state.function(WRAPPER_NAME));

            t_static_function.pretty_print(state.clazz(STATIC_FUNCTIONS));
        } else {
            Template t_function = getTemplate("jsc_function_declaration");
            t_function.replace("$jsname", jsname.c_str())
                .replace("$jswrapper", state.function(WRAPPER_NAME));
            t_function.pretty_print(state.clazz(MEMBER_FUNCTIONS));
        }
    } else {
        Template t_function = getTemplate("jsc_global_function_declaration");
        t_function.replace("$jsname", jsname.c_str())
            .replace("$jswrapper", state.function(WRAPPER_NAME));
        t_function.pretty_print(Getattr(current_namespace, "functions"));
    }

    return SWIG_OK;
}

int CocosEmitter::enterVariable(Node *n) {
    JSEmitter::enterVariable(n);

    auto& state = currentState();
    auto* vetoGet = Copy(VETO_GET);
    state.variable(GETTER, vetoGet);
    Delete(vetoGet);

    auto* vetoSet = Copy(VETO_SET);
    state.variable(SETTER, vetoSet);
    Delete(vetoSet);

//cjh    Swig_print(NewStringf("~~~~ \nenterVariable: %s\ngetter: %s\nsetter: %s\n~~~~", Getattr(n, "name"), state.variable(GETTER), state.variable(SETTER)));

    return SWIG_OK;
}

int CocosEmitter::exitVariable(Node *n) {
    auto& state = currentState();
//cjh    Swig_print(NewStringf("-----\nname: %s\ngetter: %s\nsetter: %s\n-------", state.variable(NAME), state.variable(GETTER), state.variable(SETTER)));

    std::string jsname = fixCppKeyword(Char(state.variable(NAME)));

    if (GetFlag(n, "ismember")) {
        if (GetFlag(state.variable(), IS_STATIC) || Equal(Getattr(n, "nodeType"), "enumitem")) {
            Template t_static_variable(getTemplate("jsc_static_variable_declaration"));
            t_static_variable.replace("$jsname", jsname.c_str())
                .replace("$jsgetter", state.variable(GETTER))
                .replace("$jssetter", state.variable(SETTER));

            t_static_variable.pretty_print(state.clazz(STATIC_VARIABLES));
        } else {

            Template t_variable(getTemplate("jsc_variable_declaration"));
            t_variable.replace("$jsname", jsname.c_str())
                .replace("$jsgetter", state.variable(GETTER))
                .replace("$jssetter", state.variable(SETTER));

            t_variable.pretty_print(state.clazz(MEMBER_VARIABLES));
        }
    } else {
        Template t_variable(getTemplate("jsc_global_variable_declaration"));
        t_variable.replace("$jsname", jsname.c_str())
            .replace("$jsgetter", state.variable(GETTER))
            .replace("$jssetter", state.variable(SETTER));

        t_variable.pretty_print(Getattr(current_namespace, "values"));
    }

    return SWIG_OK;
}

int CocosEmitter::enterClass(Node *n) {
    pushState();

    JSEmitter::enterClass(n);
    auto& state = currentState();
    state.clazz(MEMBER_VARIABLES, NewStringEmpty());
    state.clazz(MEMBER_FUNCTIONS, NewStringEmpty());
    state.clazz(STATIC_VARIABLES, NewStringEmpty());
    state.clazz(STATIC_FUNCTIONS, NewStringEmpty());

    String* finalizerFunction = nullptr;
    auto* nestClassNameList = state.clazz(NEST_CLASS_NAME_LIST);
    if (nestClassNameList != nullptr){
        finalizerFunction = joinClassSymNameWithList(nestClassNameList, "_");
        auto namespaceNameArray = getNamespaceNameArray(n);
        if (!namespaceNameArray.empty()) {
            std::string namespaceName = join(namespaceNameArray, "_");
            namespaceName.append("_");
            Insert(finalizerFunction, 0, namespaceName.c_str());
        }
    }
    else {
        finalizerFunction = state.clazz(NAME_MANGLED);
    }

    Template t_class_decl = getTemplate("jsc_class_declaration");
    String *type = SwigType_manglestr(Getattr(n, "classtypeobj"));
    t_class_decl.replace("$jsmangledname", state.clazz(NAME_MANGLED))
        .replace("$jsdtor", finalizerFunction)
        .pretty_print(s_wrappers);

    if (nestClassNameList != nullptr) {
        Delete(finalizerFunction);
    }

    return SWIG_OK;
}

int CocosEmitter::exitClass(Node *n) {
    auto& state = currentState();
    Clear(state.globals(INITIALIZER));

    /* adds the ctor wrappers at this position */
    // Note: this is necessary to avoid extra forward declarations.
    //Append(s_wrappers, state.clazz(CTOR_WRAPPERS));

    // for abstract classes add a vetoing ctor
//    if (GetFlag(state.clazz(), IS_ABSTRACT)) {
//        Template t_veto_ctor(getTemplate("js_veto_ctor"));
//        t_veto_ctor.replace("$jswrapper", state.clazz(CTOR))
//            .replace("$jsmangledname", state.clazz(NAME_MANGLED))
//            .replace("$jsname", state.clazz(NAME))
//            .pretty_print(s_wrappers);
//    }



    /* prepare registration of base class */
    String *jsclass_inheritance = NewStringEmpty();
    Node *base_class = getBaseClass(n);
    auto* nestClassNameList = state.clazz(NEST_CLASS_NAME_LIST);
    String* jsname = NewStringEmpty();
    if (nestClassNameList) {
        Append(jsname, "###cc");
        int sz = Len(nestClassNameList);
        for (int i = 0; i < sz; i++) {
            String *s = Getitem(nestClassNameList, i);
            Printf(jsname, "\"%s\"", s);
            if (i != (sz-1)) {
                Append(jsname, ", ");
            }
        }
        Append(jsname, "cc###");
    }
    else {
        Printf(jsname, "\"%s\"", state.clazz(NAME));
    }

    if (base_class != NULL) {
        String *baseClassNameMangled = NewString("");
        Printf(baseClassNameMangled, "%s", Getattr(base_class, "classtype"));
        convertToMangledName(baseClassNameMangled);

        Template t_inherit(getTemplate("jsc_class_inherit"));
        t_inherit.replace("$jsmangledname", state.clazz(NAME_MANGLED))
            .replace("$jsname", jsname)
            .replace("$jsctor", state.clazz(CTOR))
            .replace("$jsbaseclassmangled", baseClassNameMangled)
            .pretty_print(jsclass_inheritance);

        Delete(baseClassNameMangled);

    } else {
        Template t_inherit(getTemplate("jsc_class_noinherit"));
        t_inherit.replace("$jsmangledname", state.clazz(NAME_MANGLED))
            .replace("$jsname", jsname)
            .replace("$jsctor", state.clazz(CTOR))
            .pretty_print(jsclass_inheritance);
    }

    Delete(jsname);
    jsname = nullptr;

    String* jsclassname = Copy(Getattr(n, "classtype"));
    Replaceall(jsclassname, "(", "");
    Replaceall(jsclassname, ")", "");

    String* s_jsc_finalize_function = NewString("");
    if (state.clazz(DTOR)) {
        Template t_finalize_function(getTemplate("jsc_finalize_function"));
        t_finalize_function.replace("$jsdtor", state.clazz(DTOR))
            .pretty_print(s_jsc_finalize_function);
    }

    /* adds a class template statement to initializer function */
    Template t_classtemplate(getTemplate("jsc_class_definition"));
    t_classtemplate.replace("$jsmangledname", state.clazz(NAME_MANGLED))
        .replace("$jsclassname", jsclassname)
        .replace("$jsname", state.clazz(NAME))
        .replace("$jsmangledtype", state.clazz(TYPE_MANGLED))
        .replace("$jsclass_inheritance", jsclass_inheritance)
        .replace("$jsctor", state.clazz(CTOR))
        .replace("$jsfinalizefunction", s_jsc_finalize_function)
        .replace("$jsnspace", Getattr(state.clazz("nspace"), NAME_MANGLED))

        .replace("$jsclassvariables", state.clazz(MEMBER_VARIABLES))
        .replace("$jsclassfunctions", state.clazz(MEMBER_FUNCTIONS))
        .replace("$jsstaticclassfunctions", state.clazz(STATIC_FUNCTIONS))
        .replace("$jsstaticclassvariables", state.clazz(STATIC_VARIABLES));

    t_classtemplate.pretty_print(state.globals(INITIALIZER));

    //NOTE: pretty_print will format '{' & '}' with new lines, but that is not we want.
    // We use ###cc to replace '{' and cc### to replace '}' in the above code,
    // Replace it back to '{' and '}' after pretty_print is invoked.
    Replace(state.globals(INITIALIZER), "###cc", "{", DOH_REPLACE_ANY);
    Replace(state.globals(INITIALIZER), "cc###", "}", DOH_REPLACE_ANY);

    Delete(jsclass_inheritance);

    /* Note: this makes sure that there is a swig_type added for this class */
    SwigType_remember_clientdata(state.clazz(TYPE_MANGLED), NewString("0"));

    bool isStruct = 0 == Cmp(Getattr(n, "kind"), "struct");
    if (isStruct) {
        String* propertyConversionCode = NewString("");
        auto propertyNames = getStructProperties(n);
        for (const auto& propertyName : propertyNames) {
            Template t_jsc_struct_prop_snippet(getTemplate("jsc_struct_prop_snippet"));
            t_jsc_struct_prop_snippet.replace("$field_name", propertyName.name.c_str())
                .replace("$field_symname", propertyName.symName.c_str())
                .pretty_print(propertyConversionCode);
        }

        Template jsc_struct_prop_conversion(getTemplate("jsc_struct_prop_conversion"));
        jsc_struct_prop_conversion.replace("$jsclassname", jsclassname)
            .replace("$jscode", propertyConversionCode)
            .pretty_print(s_wrappers);

        Delete(propertyConversionCode);
    }

    Printv(s_wrappers, state.globals(INITIALIZER), 0); //cjh added



    Template t_headerRegisterClass(getTemplate("se_global_variables"));
    t_headerRegisterClass.replace("$jsclassname", jsclassname)
        .replace("$jsmangledname", state.clazz(NAME_MANGLED))
        .pretty_print(_rootState->globals(HEADER_REGISTER_CLASSES));

    if (isStruct) {
        Template jsc_struct_prop_conversion_declare(getTemplate("jsc_struct_prop_conversion_declare"));
        jsc_struct_prop_conversion_declare.replace("$jsclassname", jsclassname)
            .pretty_print(_rootState->globals(HEADER_REGISTER_CLASSES));
    }

    Delete(jsclassname);

    popState();
    return SWIG_OK;
}

Hash *CocosEmitter::createNamespaceEntry(const char *name, const char *parent, const char *parent_mangled) {
    Hash *entry = JSEmitter::createNamespaceEntry(name, parent, parent_mangled);
    Setattr(entry, "functions", NewString(""));
    Setattr(entry, "values", NewString(""));
    return entry;
}

int CocosEmitter::emitNamespaces() {
    auto& state = currentState();
    Iterator it;
    for (it = First(namespaces); it.item; it = Next(it)) {
        Hash *entry = it.item;
        String *name = Getattr(entry, NAME);
        String *name_mangled = Getattr(entry, NAME_MANGLED);
        String *parent_mangled = Getattr(entry, PARENT_MANGLED);
        String *functions = Getattr(entry, "functions");
        String *variables = Getattr(entry, "values");

        // skip the global namespace which is given by the application
        Template t_createNamespace(getTemplate("jsc_global_registration"));
        t_createNamespace.replace("$jsmangledname", name_mangled)
            .replace("$jsglobalvariables", variables)
            .replace("$jsglobalfunctions", functions)
            .replace("$jsname", name)
            .replace("$jsparent", parent_mangled)
            .pretty_print(state.globals(REGISTER_NAMESPACES));

        //    Append(state.globals(CREATE_NAMESPACES), t_createNamespace.str());
    }

    return SWIG_OK;
}

JSEmitter *swig_javascript_create_CocosEmitter() {
    return new CocosEmitter();
}

/**********************************************************************
 * Helper implementations
 **********************************************************************/

JSEmitterState::JSEmitterState()
: globalHash(NewHash()) {
    // initialize sub-hashes
    Setattr(globalHash, "class", NewHash());
    Setattr(globalHash, "function", NewHash());
    Setattr(globalHash, "variable", NewHash());
}

JSEmitterState::~JSEmitterState() {
    Clear(globalHash);
    Delete(globalHash);
}

DOH *JSEmitterState::getState(const char *key, bool new_key) {
    if (new_key) {
        Hash *hash = NewHash();
        Setattr(globalHash, key, hash);
    }
    return Getattr(globalHash, key);
}

DOH *JSEmitterState::globals() {
    return globalHash;
}

DOH *JSEmitterState::globals(const char *key, DOH *initial) {
    if (initial != 0) {
        Setattr(globalHash, key, initial);
    }
    return Getattr(globalHash, key);
}

DOH *JSEmitterState::clazz(bool new_key) {
    return getState("class", new_key);
}

DOH *JSEmitterState::clazz(const char *key, DOH *initial) {
    DOH *c = clazz();
    if (initial != 0) {
        Setattr(c, key, initial);
    }
    return Getattr(c, key);
}

DOH *JSEmitterState::function(bool new_key) {
    return getState("function", new_key);
}

DOH *JSEmitterState::function(const char *key, DOH *initial) {
    DOH *f = function();
    if (initial != 0) {
        Setattr(f, key, initial);
    }
    return Getattr(f, key);
}

DOH *JSEmitterState::variable(bool new_key) {
    return getState("variable", new_key);
}

DOH *JSEmitterState::variable(const char *key, DOH *initial) {
    DOH *v = variable();
    if (initial != 0) {
        Setattr(v, key, initial);
    }
    return Getattr(v, key);
}

/*static*/
int JSEmitterState::IsSet(DOH *val) {
    if (!val) {
        return 0;
    } else {
        const char *cval = Char(val);
        if (!cval)
            return 0;
        return (strcmp(cval, "0") != 0) ? 1 : 0;
    }
}

/* -----------------------------------------------------------------------------
 * Template::Template() :  creates a Template class for given template code
 * ----------------------------------------------------------------------------- */

Template::Template(const String *code_) {
    if (!code_) {
        Printf(stdout, "Template code was null. Illegal input for template.");
        Exit(EXIT_FAILURE);
    }
    code = NewString(code_);
    templateName = NewString("");
}

Template::Template(const String *code_, const String *templateName_) {
    if (!code_) {
        Printf(stdout, "Template code was null. Illegal input for template.");
        Exit(EXIT_FAILURE);
    }

    code = NewString(code_);
    templateName = NewString(templateName_);
}

/* -----------------------------------------------------------------------------
 * Template::~Template() :  cleans up of Template.
 * ----------------------------------------------------------------------------- */

Template::~Template() {
    Delete(code);
    Delete(templateName);
}

/* -----------------------------------------------------------------------------
 * String* Template::str() :  retrieves the current content of the template.
 * ----------------------------------------------------------------------------- */

String *Template::str() {
    if (js_template_enable_debug) {
        String *pre_code = NewString("");
        String *post_code = NewString("");
        String *debug_code = NewString("");
        Printf(pre_code, "/* begin fragment(\"%s\") */", templateName);
        Printf(post_code, "/* end fragment(\"%s\") */", templateName);
        Printf(debug_code, "%s\n%s\n%s\n", pre_code, code, post_code);

        Delete(code);
        Delete(pre_code);
        Delete(post_code);

        code = debug_code;
    }
    return code;
}

Template &Template::trim() {
    const char *str = Char(code);
    if (str == 0)
        return *this;

    int length = Len(code);
    if (length == 0)
        return *this;

    int idx;
    for (idx = 0; idx < length; ++idx) {
        if (str[idx] != ' ' && str[idx] != '\t' && str[idx] != '\r' && str[idx] != '\n')
            break;
    }
    int start_pos = idx;

    for (idx = length - 1; idx >= start_pos; --idx) {
        if (str[idx] != ' ' && str[idx] != '\t' && str[idx] != '\r' && str[idx] != '\n')
            break;
    }
    int end_pos = idx;

    int new_length = end_pos - start_pos + 1;
    char *newstr = new char[new_length + 1];
    memcpy(newstr, str + start_pos, new_length);
    newstr[new_length] = 0;

    Delete(code);
    code = NewString(newstr);
    delete[] newstr;

    return *this;
}

/* -----------------------------------------------------------------------------
 * Template&  Template::replace(const String* pattern, const String* repl) :
 *
 *  replaces all occurrences of a given pattern with a given replacement.
 *
 *  - pattern:  the pattern to be replaced
 *  - repl:     the replacement string
 *  - returns a reference to the Template to allow chaining of methods.
 * ----------------------------------------------------------------------------- */

Template &Template::replace(const String *pattern, const String *repl) {
    Replaceall(code, pattern, repl);
    return *this;
}

Template &Template::print(DOH *doh) {
    Printv(doh, str(), 0);
    return *this;
}

Template &Template::pretty_print(DOH *doh) {
    Wrapper_pretty_print(str(), doh);
    return *this;
}

Template::Template(const Template &t) {
    code = NewString(t.code);
    templateName = NewString(t.templateName);
}

void Template::operator=(const Template &t) {
    Delete(code);
    Delete(templateName);
    code = NewString(t.code);
    templateName = NewString(t.templateName);
}

} // namespace cc
