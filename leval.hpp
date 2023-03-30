#ifndef LEVAL
#define LEVAL

#include <map>
#include <memory>
#include <stack>
#include <stdexcept>
#include <variant>
#include <vector>

namespace LE {

// internal namespace hides unimportant things from the user
namespace _int {
// vector stack with exposed underlying vector
// because we need indexing, insert of vectors and other operations
template <typename T> class i_stack : public std::stack<T, std::vector<T>> {
protected:
  using std::stack<T, std::vector<T>>::c;

public:
  // reverse indexing
  auto operator[](int pos) -> T & {
    return c[c.size() - 1 - static_cast<size_t>(pos)];
  }

  void insert(std::vector<T> &&to_insert) {
    c.insert(c.end(), to_insert.begin(), to_insert.end());
  }
  void insert(std::vector<T> &to_insert) {
    c.insert(c.end(), to_insert.begin(), to_insert.end());
  }
  void insert(i_stack<T> &&to_insert) {
    c.insert(c.end(), to_insert.c.begin(), to_insert.c.end());
  }
};

enum GmValEnum { GLOB, VALUE, ARG, LOCAL };

using GmVal = std::pair<GmValEnum, int>;

enum GmInstrEnum { // Instructions for the Stack machine
  SLIDE,
  PUSH,
  COND,
  MKAP,
  EVAL,
  ADD,
  SUB,
  DIV,
  MUL,
  EQU,
  UNWIND
};

// type representing Instructions for the stackmachine
struct GmInstr {
  using GmInstrData =
      std::variant<int, GmVal,
                   std::pair<std::vector<GmInstr>, std::vector<GmInstr>>,
                   std::monostate>;
  GmInstrEnum instr;
  GmInstrData data;
};

// mapping of basic operations in function libraries
enum BINOP_MAPPINGS { ADD_M, SUB_M, DIV_M, MUL_M, EQU_M, IF_M };

// structure representing supercombinator/functions
// for the stack_machine
struct SCo {
  int arity = -1;
  i_stack<GmInstr> code = {};
  SCo() = default;
  SCo(int arity, std::vector<GmInstr> code_s) : arity(arity) {
    code.insert(code_s);
  }
};

using GmInstrData =
    std::variant<int, GmVal,
                 std::pair<std::vector<GmInstr>, std::vector<GmInstr>>,
                 std::monostate>;

} // namespace _int

struct Arg;

// Symbolic type representing functions for compilation
// we need their mapping in the function library and their arguments
struct function_prototype {
  int hash = 0;
  std::vector<std::variant<function_prototype, int, Arg>> args;
  // same operator as for LEfunctions for if_fun compilation ergonomics
  auto
  operator()(std::vector<std::variant<function_prototype, int, Arg>> args_t)
      -> function_prototype {
    args = std::move(args_t);
    return (*this);
  }
};

// represents argument of currently compiled function (0,arity-1)
struct Arg {
  int arg;
  explicit Arg(int arg) : arg(arg) {}
};

// Possible arguments to a function prototype
using body_t = std::variant<function_prototype, int, Arg>;

// Operators for ergonomic compilation of functions
// we can do {1}.compile({Arg(0) - 1}) for decrement function;
auto operator+(body_t &&a, body_t &&b) -> function_prototype {
  return {_int::ADD_M, {std::forward<body_t>(a), std::forward<body_t>(b)}};
}

auto operator-(body_t &&a, body_t &&b) -> function_prototype {
  return {_int::SUB_M, {std::forward<body_t>(a), std::forward<body_t>(b)}};
}

auto operator*(body_t &&a, body_t &&b) -> function_prototype {
  return {_int::MUL_M, {std::forward<body_t>(a), std::forward<body_t>(b)}};
}

auto operator/(body_t &&a, body_t &&b) -> function_prototype {
  return {_int::DIV_M, {std::forward<body_t>(a), std::forward<body_t>(b)}};
}

auto operator==(body_t &&a, body_t &&b) -> function_prototype {
  return {_int::EQU_M, {std::forward<body_t>(a), std::forward<body_t>(b)}};
}

using Arguments = std::vector<body_t>;

// function library represents functions known during execution
using fun_lib = std::map<int, _int::SCo>;

// initialize adds build-in precompiled operations
// and must be called before compilation
auto initialize_fun_lib() -> fun_lib {
  fun_lib lib = {};
  std::vector<_int::GmInstr> binop = {
      {_int::UNWIND, std::monostate()},      {_int::SLIDE, 3},
      {_int::ADD, std::monostate()},         {_int::EVAL, std::monostate()},
      {_int::PUSH, std::pair(_int::ARG, 2)}, {_int::EVAL, std::monostate()},
      {_int::PUSH, std::pair(_int::ARG, 0)}};
  std::vector<_int::GmInstr> if_code = {
      {_int::UNWIND, std::monostate()},
      {_int::SLIDE, 4},
      {_int::COND,
       std::pair(
           std::vector<_int::GmInstr>{{_int::PUSH, std::pair(_int::ARG, 1)}},
           std::vector<_int::GmInstr>{{_int::PUSH, std::pair(_int::ARG, 2)}})},
      {_int::EVAL, std::monostate()},
      {_int::PUSH, std::pair(_int::ARG, 0)}};
  lib[_int::ADD_M] = {2, binop};
  binop[2].instr = _int::SUB;
  lib[_int::SUB_M] = {2, binop};
  binop[2].instr = _int::DIV;
  lib[_int::DIV_M] = {2, binop};
  binop[2].instr = _int::MUL;
  lib[_int::MUL_M] = {2, binop};
  binop[2].instr = _int::EQU;
  lib[_int::EQU_M] = {2, binop};
  lib[_int::IF_M] = {3, if_code};
  return lib;
};

function_prototype if_fun{_int::IF_M, {}};

namespace _int {

// default function library functions are added to
fun_lib function_library = initialize_fun_lib();

struct SCo;
struct App;

// types representing stack nodes, stack, stack of stacks
using GmNode = std::variant<int, SCo *, App>;
using stack_i = i_stack<std::shared_ptr<GmNode>>;
using dump_t = std::stack<std::pair<i_stack<GmInstr>, stack_i>>;

// enum for GmNodes
enum GmNodeEnum {
  NUME,
  SCOE,
  APPE,
};

// application node - represented in curried form
// if a function of 2 arguments gets 1 argument it returns a function in 1
// argument
struct App {
  std::shared_ptr<GmNode> function;
  std::shared_ptr<GmNode> arg;
  App(auto &&f, auto &&x)
      : function(std::forward<decltype(f)>(f)),
        arg(std::forward<decltype(x)>(x)) {}
};

// the stack machine itself
class G_machine {
  fun_lib *used_function_library;
  stack_i curr_stack{};
  dump_t dump{};
  i_stack<GmInstr> code;

public:
  // executes itself
  auto eval() -> int {
    while (!end()) {
      auto next_i = std::move(code.top());
      code.pop();
      perform_instr(std::move(next_i));
    }
    if (curr_stack.size() == 1 && curr_stack.top()->index() == NUME) {
      return std::get<NUME>(*curr_stack.top());
    }
    throw std::logic_error("Program terminated without result");
  }

  G_machine(auto &&lib, auto &&curr, auto &&dump, auto &&code)
      : used_function_library(std::forward<decltype(lib)>(lib)),
        curr_stack(std::forward<decltype(curr)>(curr)),
        dump(std::forward<decltype(dump)>(dump)),
        code(std::forward<decltype(code)>(code)) {}

private:
  // performs single instruction and updates its state
  void perform_instr(GmInstr &&next) {
    switch (next.instr) {
    case PUSH: {
      push(std::get<PUSH>(next.data));
    } break;
    case SLIDE: {
      // saves top element and removes n+1 elements from stack
      auto saves = std::move(curr_stack.top());
      int num = std::get<SLIDE>(next.data);
      for (int i = 0; i <= num; ++i) {
        curr_stack.pop();
      }
      curr_stack.push(saves);
    } break;
    case COND: {
      // condition instruction
      auto cond = std::move(curr_stack.top());
      auto code_pair = std::get<COND>(next.data);
      curr_stack.pop();
      if (std::get<NUME>(*cond) != 0) {
        code.insert(code_pair.first);
      } else {
        code.insert(code_pair.second);
      }
      // Add the new code to instructs
    } break;
    case MKAP: {
      // makes an application node from 2 top-most nodes
      auto x = std::move(curr_stack.top());
      curr_stack.pop();
      auto f = std::move(curr_stack.top());
      curr_stack.pop();
      curr_stack.push(
          std::make_shared<GmNode>(App(std::move(f), std::move(x))));
    } break;
    case UNWIND: {
      // Prepares stack for execution
      unwind();
    } break;
    case EVAL: {
      // Actually evaluating the top element
      // we make another stack and save the current state
      auto nd = std::move(curr_stack.top());
      curr_stack.pop();
      dump.push(std::pair(std::move(code), std::move(curr_stack)));
      code = i_stack<GmInstr>();
      code.push({UNWIND, std::monostate()});
      curr_stack = stack_i();
      curr_stack.push(std::move(nd));
    } break;
    case ADD: {
      auto arg = getNums();
      curr_stack.push(
          std::move(std::make_shared<GmNode>(arg.second + arg.first)));
    } break;
    // Trivial evaluation of binary operations
    case SUB: {
      auto arg = getNums();
      curr_stack.push(
          std::move(std::make_shared<GmNode>(arg.second - arg.first)));
    } break;
    case DIV: {
      auto arg = getNums();
      curr_stack.push(
          std::move(std::make_shared<GmNode>(arg.second / arg.first)));
    } break;
    case MUL: {
      auto arg = getNums();
      curr_stack.push(
          std::move(std::make_shared<GmNode>(arg.second * arg.first)));
    } break;
    case EQU: {
      auto arg = getNums();
      curr_stack.push(
          std::move(std::make_shared<GmNode>(arg.second == arg.first)));
    } break;
    }
  }

  void unwind() {
    switch (curr_stack.top()->index()) {
    // we have either ended the computation or evaluated some expression
    case NUME: {
      if (dump.empty()) {
        code = i_stack<GmInstr>();
      } else {
        auto st = std::move(curr_stack);
        curr_stack = std::move(dump.top().second);
        curr_stack.insert(std::move(st));
        code = std::move(dump.top().first);
        dump.pop();
      }
    } break;
    // we load the super combinator code and continue execution
    case SCOE: {
      SCo *sco = std::get<SCOE>(*curr_stack.top());
      code = sco->code;
      if (!(curr_stack.size() + 1 >= static_cast<size_t>(sco->arity))) {
        throw std::logic_error(
            "supercombinator does not have enough arguments");
      }
    } break;
    // we push function of the application on the stack
    case APPE: {
      code = i_stack<GmInstr>();
      code.push({UNWIND, std::monostate()});
      curr_stack.push(std::get<APPE>(*curr_stack.top()).function);
    } break;
    }
  }
  // different pushes
  void push(GmVal val) {
    switch (val.first) {
    // pushes super combinator from fun_lib
    case GLOB: {
      SCo *sco = &(*used_function_library)[val.second];
      curr_stack.push(std::make_shared<GmNode>(sco));
    } break;
    // pushes a constant
    case VALUE: {
      curr_stack.push(std::make_shared<GmNode>(val.second));
    } break;
    // argument of the function at curr_stack[i]
    case ARG: {
      curr_stack.push(std::get<APPE>(*curr_stack[val.second + 1]).arg);
    } break;
    // pointer to curr_stack[i] - used to optimize
    case LOCAL: {
      curr_stack.push(curr_stack[val.second]);
    } break;
    }
  }
  auto getNums() -> std::pair<int, int> {
    auto a = std::move(curr_stack.top());
    curr_stack.pop();
    auto b = std::move(curr_stack.top());
    curr_stack.pop();
    return std::pair(std::get<NUME>(*a), std::get<NUME>(*b));
  }
  // execution ends if there is no more code or
  // there is nothing more to execute and there is only a number on the stack
  auto end() -> bool {
    bool whnf = curr_stack.size() == 1 && curr_stack.top()->index() == NUME;
    return code.empty() || (dump.empty() && whnf);
  }
};

} // namespace _int

// representation of functions for users
class LEfunction {
  // we maintain mapping, arity, instructions, code is only non-empty during
  // compilation and pointer to the function_library we want to use
  int hash = -1;
  int arity = -1;
  std::vector<_int::GmInstr> code{};
  std::map<int, _int::SCo> *globals = &_int::function_library;

  // compiles body to code
  void compile_helper(body_t body, int &pushed) {
    switch (body.index()) {
    // recurse
    case _int::GLOB: {
      function_prototype f = std::get<_int::GLOB>(body);
      code.push_back({_int::PUSH, std::pair(_int::GLOB, f.hash)});
      ++pushed;
      for (auto &&arg : f.args) {
        compile_helper(arg, pushed);
        code.push_back({_int::MKAP, std::monostate()});
        --pushed;
      }
    } break;
    case _int::VALUE: {
      code.push_back(
          {_int::PUSH, std::pair(_int::VALUE, std::get<_int::VALUE>(body))});
      ++pushed;
    } break;
    case _int::ARG: {
      code.push_back(
          {_int::PUSH,
           std::pair(_int::ARG, std::get<_int::ARG>(body).arg + pushed)});
      ++pushed;
    } break;
    }
  }

public:
  // constructing functions adds it to the library
  LEfunction(int arity, auto library) : arity(arity), globals(library) {

    (*globals)[globals->size()] = {};
    hash = globals->size();
  };

  explicit LEfunction(int arity) : arity(arity) {
    (*globals)[globals->size()] = {};
    hash = globals->size();
  };

  // compile compiles body and cleanes up
  // return value allows for non-recursive lamdas
  // {2}.compile(Arg(1) + Arg(2))({1,2})
  auto compile(auto &&body) -> LEfunction {
    int pushed = 0;
    compile_helper(std::forward<decltype(body)>(body), pushed);
    code.push_back({_int::SLIDE, arity + 1});
    code.push_back({_int::UNWIND, std::monostate()});
    std::reverse(code.begin(), code.end());
    (*globals)[hash] = {arity, std::move(code)};
    code = {};
    return *this;
  }

  // execution makes a temporary function for arguments and evaluates
  auto exec(const std::vector<int> &args) -> int {
    std::vector<_int::GmInstr> init_code{};
    init_code.push_back({_int::UNWIND, std::monostate()});
    for (auto &&arg : args) {
      init_code.push_back({_int::MKAP, std::monostate()});
      init_code.push_back({_int::PUSH, std::pair(_int::VALUE, arg)});
    }
    init_code.push_back({_int::PUSH, std::pair(_int::GLOB, hash)});
    _int::i_stack<_int::GmInstr> init{};
    init.insert(std::move(init_code));
    _int::G_machine g = {globals, _int::stack_i{}, _int::dump_t{},
                         std::move(init)};
    return g.eval();
  }

  // allows to set function body for other funcitons
  // by calling the function
  auto operator()(Arguments arguments) -> function_prototype {
    return {hash, std::move(arguments)};
  }
};

} // namespace LE

#endif
