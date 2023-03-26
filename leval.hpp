#ifndef LEVAL
#define LEVAL

#include <string>
#include <utility>
#include <vector>
#include <variant>
#include <memory>
#include <map>
#include <stack>
#include <stdexcept>

namespace LE {

namespace _int{
// vector stack with exposed underlying vector
// because we need indexing, insert of vectors and other operations
template<typename T>
class i_stack : public std::stack<T,std::vector<T>>{
    protected: 
        using std::stack<T,std::vector<T>>::c;
    public:
    // reverse indexing
    auto operator[]( int pos ) -> T&{
        return c[c.size()-1-static_cast<size_t>(pos)];
    }

    void insert(std::vector<T>&& to_insert){
        c.insert(c.end(),to_insert.begin(),to_insert.end());
    }
    void insert(std::vector<T>& to_insert){
        c.insert(c.end(),to_insert.begin(),to_insert.end());
    }
    void insert(i_stack<T>&& to_insert){
        c.insert(c.end(),to_insert.c.begin(),to_insert.c.end());
    }
};

enum GmValEnum {
    GLOB, 
    VALUE, 
    ARG, 
    LOCAL
};

using GmVal=std::pair<GmValEnum,int>;

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

enum BINOP_MAPPINGS{
    ADD_M,
    SUB_M,
    DIV_M,
    MUL_M,
    EQU_M,
    IF_M
};

struct GmInstr{
    using GmInstrData=std::variant<int, GmVal, 
                std::pair<std::vector<GmInstr>,std::vector<GmInstr>>,std::monostate>;
    GmInstrEnum instr;
    GmInstrData data;
};

struct SCo{
    int arity=-1;
    i_stack<GmInstr> code={};
    SCo ()=default;
    SCo (int arity, std::vector<GmInstr> code_s) : arity(arity){
        code.insert(code_s);
    }
};

using GmInstrData=std::variant<int, GmVal, 
            std::pair<std::vector<GmInstr>,std::vector<GmInstr>>,std::monostate>;

}

struct Arg;

struct function_prototype{
    int hash=0;
    std::vector<std::variant<function_prototype, int, Arg>> args;
    auto operator()
        (std::vector<std::variant<function_prototype, int, Arg>> args_t) -> function_prototype{
        args=std::move(args_t);
        return (*this);
    }
};

struct Arg {
    int arg;
    explicit Arg(int arg) : arg(arg){}
};

using body_t=std::variant<function_prototype, int, Arg>;

auto operator+ (body_t&& a, body_t&& b) -> function_prototype{
    return {_int::ADD_M,{std::forward<body_t>(a), std::forward<body_t>(b)}};
}

auto operator- (body_t&& a, body_t&& b) -> function_prototype{
    return {_int::SUB_M,{std::forward<body_t>(a), std::forward<body_t>(b)}};
}

auto operator* (body_t&& a, body_t&& b) -> function_prototype{
    return {_int::MUL_M,{std::forward<body_t>(a), std::forward<body_t>(b)}};
}

auto operator/ (body_t&& a, body_t&& b) -> function_prototype{
    return {_int::DIV_M,{std::forward<body_t>(a), std::forward<body_t>(b)}};
}

auto operator== (body_t&& a, body_t&& b) -> function_prototype{
    return {_int::EQU_M,{std::forward<body_t>(a), std::forward<body_t>(b)}};
}

using Arguments = std::vector<body_t>;

using fun_lib=std::map<int, _int::SCo>;

auto initialize_fun_lib() -> fun_lib{
    fun_lib lib={};
    std::vector<_int::GmInstr> binop ={{_int::UNWIND,std::monostate()}, {_int::SLIDE,3}, 
        {_int::ADD, std::monostate()}, {_int::EVAL, std::monostate()},
        {_int::PUSH, std::pair(_int::ARG, 2)}, {_int::EVAL, std::monostate()},
        {_int::PUSH, std::pair(_int::ARG, 0)} };
        std::vector<_int::GmInstr> if_code = {{_int::UNWIND,std::monostate()},
            {_int::SLIDE,4},
            {_int::COND,std::pair(std::vector<_int::GmInstr>{{_int::PUSH,std::pair(_int::ARG,1)}},
                    std::vector<_int::GmInstr>{{_int::PUSH,std::pair(_int::ARG,2)}})},
            {_int::EVAL,std::monostate()}, {_int::PUSH, std::pair(_int::ARG, 0)}};
    lib[_int::ADD_M]={2,binop};
    binop[2].instr=_int::SUB;
    lib[_int::SUB_M]={2,binop};
    binop[2].instr=_int::DIV;
    lib[_int::DIV_M]={2,binop};
    binop[2].instr=_int::MUL;
    lib[_int::MUL_M]={2,binop};
    binop[2].instr=_int::EQU;
    lib[_int::EQU_M]={2,binop};
    lib[_int::IF_M]={3,if_code};
    return lib;
};

function_prototype if_fun{_int::IF_M, {}};

namespace _int{

fun_lib function_library=initialize_fun_lib();

struct SCo;
struct App;

using GmNode=std::variant<int,SCo*,App>;
using stack_i=i_stack<std::shared_ptr<GmNode>>;
using dump_t=std::stack<std::pair<i_stack<GmInstr>,stack_i>>;


struct App{
    std::shared_ptr<GmNode> function;
    std::shared_ptr<GmNode> arg;
    App(auto&& f, auto&& x) : function(std::forward<decltype(f)>(f)), arg(std::forward<decltype(x)>(x)){
        
        
    }
};

enum GmNodeEnum{
    NUME,
    SCOE,
    APPE,
};

class G_machine{
    fun_lib* used_function_library;
    // stack but we need indexing
    stack_i curr_stack{};
    dump_t dump{};
    i_stack<GmInstr> code;

    public:
    auto eval() -> int{
        while (!end()){
            auto next_i=std::move(code.top());
            code.pop();
            perform_instr(std::move(next_i));
        }
        if (curr_stack.size()==1 && curr_stack.top()->index()==NUME){
            return std::get<NUME>(*curr_stack.top());
        }
        throw std::logic_error( "Program terminated without result" );
    }
        G_machine(auto&& lib, auto&& curr, auto&& dump, auto&& code) : 
            used_function_library(std::forward<decltype(lib)>(lib)),
            curr_stack(std::forward<decltype(curr)>(curr)),
            dump(std::forward<decltype(dump)>(dump)),
            code(std::forward<decltype(code)>(code))
        { }

    private: 

    void perform_instr(GmInstr&& next){
        switch (next.instr) {
            case PUSH: {
                push(std::get<PUSH>(next.data));
            } break;
            case SLIDE: {
                auto saves=std::move(curr_stack.top());
                int num=std::get<SLIDE>(next.data);
                for (int i=0;i<=num;++i){
                    curr_stack.pop();
                }
                curr_stack.push(saves);
            } break;
            case COND: {
                auto cond=std::move(curr_stack.top());
                auto code_pair=std::get<COND>(next.data);
                curr_stack.pop();
                if (std::get<NUME>(*cond)!=0){
                    code.insert(code_pair.first);
                } else {
                    code.insert(code_pair.second);
                }
                // Add the new code to instructs
            } break;
            case MKAP: {
                auto x=std::move(curr_stack.top());
                curr_stack.pop();
                auto f=std::move(curr_stack.top());
                curr_stack.pop();
                curr_stack.push(std::make_shared<GmNode>(App(std::move(f),std::move(x))));
            } break;
            case UNWIND: {
                unwind();
            } break;
            case EVAL: {
                auto nd=std::move(curr_stack.top());
                curr_stack.pop();
                dump.push(std::pair(std::move(code),std::move(curr_stack)));
                code=i_stack<GmInstr>();
                code.push({UNWIND,std::monostate()});
                curr_stack=stack_i();
                curr_stack.push(std::move(nd));
            } break;
            case ADD: {
                auto arg=getNums();
                curr_stack.push(std::move(std::make_shared<GmNode>(arg.second+arg.first)));
            } break;
            case SUB: {
                auto arg=getNums();
                curr_stack.push(std::move(std::make_shared<GmNode>(arg.second-arg.first)));
            } break;
            case DIV: {
                auto arg=getNums();
                curr_stack.push(std::move(std::make_shared<GmNode>(arg.second/arg.first)));
            } break;
            case MUL: {
                auto arg=getNums();
                curr_stack.push(std::move(std::make_shared<GmNode>(arg.second*arg.first)));
            } break;
            case EQU: {
                auto arg=getNums();
                curr_stack.push(std::move(std::make_shared<GmNode>(arg.second==arg.first)));
            } break;
        }
    }

    void unwind(){
        switch (curr_stack.top()->index()){
            case NUME: {
                if (dump.empty()){
                    code=i_stack<GmInstr>();
                } else {
                    auto st=std::move(curr_stack);
                    curr_stack=std::move(dump.top().second);
                    curr_stack.insert(std::move(st));
                    code=std::move(dump.top().first);
                    dump.pop();
                }
            } break; 
            case SCOE: {
                SCo* sco=std::get<SCOE>(*curr_stack.top());
                code=sco->code;
                if (!(curr_stack.size()+1>=static_cast<size_t>(sco->arity))) {
                    throw std::logic_error( "supercombinator does not have enough arguments" );
                }
            } break; 
            case APPE: {
                code=i_stack<GmInstr>();
                code.push({UNWIND,std::monostate()});
                curr_stack.push(std::get<APPE>(*curr_stack.top()).function);
            } break;
        }
    }
    void push(GmVal  val){
        switch (val.first) {
            case GLOB: {
                SCo* sco = &(*used_function_library)[val.second];
                curr_stack.push(std::make_shared<GmNode>(sco));
            } break;
            case VALUE: {
                curr_stack.push(std::make_shared<GmNode>(val.second));
            } break; 
            case ARG: {
                curr_stack.push(std::get<APPE>(*curr_stack[val.second + 1]).arg);
            } break; 
            case LOCAL: {
                curr_stack.push(curr_stack[val.second]);
            } break; 
        }
    }
    auto getNums () -> std::pair<int,int>{
        auto a = std::move(curr_stack.top());
        curr_stack.pop();
        auto b = std::move(curr_stack.top());
        curr_stack.pop();
        return std::pair(std::get<NUME>(*a),std::get<NUME>(*b));
    }
    auto end() -> bool{
        bool whnf = curr_stack.size()==1 && curr_stack.top()->index()==NUME;
            return code.empty() || (dump.empty() && whnf);
    }
};

}

class LEfunction {
    int hash=-1;
    int arity=-1;
    std::vector<_int::GmInstr> code{};
    std::map<int, _int::SCo>* globals=&_int::function_library;

void compile_helper(body_t body, int &pushed){
    switch (body.index()){
        case _int::GLOB : {
            function_prototype f=std::get<_int::GLOB>(body);
            code.push_back({_int::PUSH,std::pair(_int::GLOB,f.hash)});
            ++pushed;
            for (auto&& arg : f.args) {
                compile_helper(arg, pushed);
                code.push_back({_int::MKAP,std::monostate()});
                --pushed;
            }
        } break;
        case _int::VALUE : {
            code.push_back({_int::PUSH,std::pair(_int::VALUE,std::get<_int::VALUE>(body))});
            ++pushed;
        } break;
        case _int::ARG : {
            code.push_back({_int::PUSH,std::pair(_int::ARG,std::get<_int::ARG>(body).arg+pushed)});
            ++pushed;
        } break;
    }
}

    public:
LEfunction (int arity, auto library) : arity(arity), globals(library){
    
    (*globals)[globals->size()]={};
    hash=globals->size();
};

explicit LEfunction (int arity) : arity(arity){
    (*globals)[globals->size()]={};
    hash=globals->size();
};

// allows for non-recursive lamdas
// {2}.compile(Arg(1) + Arg(2))({1,2})
auto compile(auto&& body) -> LEfunction{
    int pushed=0;
    compile_helper(std::forward<decltype(body)>(body), pushed);
    code.push_back({_int::SLIDE,arity+1});
    code.push_back({_int::UNWIND, std::monostate()});
    std::reverse(code.begin(),code.end());
    (*globals)[hash]={arity,code};
    return *this;
}

auto exec(const std::vector<int>& args) -> int{
    std::vector<_int::GmInstr> init_code{};
    init_code.push_back({_int::UNWIND,std::monostate()});
    for (auto&& arg : args){
        init_code.push_back({_int::MKAP,std::monostate()});
        init_code.push_back({_int::PUSH,std::pair(_int::VALUE,arg)});
    }
    init_code.push_back({_int::PUSH,std::pair(_int::GLOB,hash)});
    _int::i_stack<_int::GmInstr> init{};
    init.insert(std::move(init_code));
    _int::G_machine g={globals, _int::stack_i{}, _int::dump_t{}, std::move(init)};
    return g.eval();
}

auto operator() (Arguments arguments) -> function_prototype{
    return {hash,std::move(arguments)};
}

};

}

# endif
