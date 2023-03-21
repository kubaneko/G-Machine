#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <map>
#include <stack>
#include <stdexcept>
#include <cassert>

enum GmValEnum {
    GLOB, 
    VALUE, 
    ARG, 
    LOCAL
};

using GmValVal=std::variant<std::string,int,int,int>;
using GmVal=std::pair<GmValEnum,GmValVal>;

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

struct GmInstr{
    using GmInstrData=std::variant<int, GmVal, 
                std::pair<std::vector<GmInstr>,std::vector<GmInstr>>,std::monostate>;
    GmInstrEnum instr;
    GmInstrData data{};
};

using GmInstrData=std::variant<int, GmVal, 
            std::pair<std::vector<GmInstr>,std::vector<GmInstr>>,std::monostate>;

struct Arg{
    int arg;
};

using Argument = std::variant<Arg, int>;

class LEfunction;

class LEfunction {
std::vector<int> args;

std::shared_ptr<LEfunction> body;

explicit LEfunction (int sizeArg){
    args=std::vector<int>(sizeArg);

};

auto getArg(int numArg) -> int{
    return args[numArg-1];
};
};

struct SCo;

struct App;

using GmNode=std::variant<int,SCo*,App>;

struct App{
    std::shared_ptr<GmNode> function{};
    std::shared_ptr<GmNode> arg{};

    App(std::shared_ptr<GmNode>&& func, std::shared_ptr<GmNode>&& arg){
        function=std::move(func);
        arg=std::move(arg);
    }
};

enum GmNodeEnum{
    NUME,
    SCOE,
    APPE,
};

// vector stack with exposed underlying vector
// because we need indexing, insert of vectors and other operations
template<typename T>
class i_stack : public std::stack<T,std::vector<T>>{
    protected: 
        using std::stack<T,std::vector<T>>::c;
    public:
    // reverse indexing
    auto operator[]( size_t pos ) -> T&{
        assert(c.size()>0);
        return c[c.size()-1-pos];
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
using stack_i=i_stack<std::shared_ptr<GmNode>>;

struct SCo{
    int arity{};
    i_stack<GmInstr> code;
};

std::map<std::string, std::shared_ptr<SCo>> function_library;

class G_machine{
    std::map<std::string, SCo> used_function_library;
    // stack but we need indexing
    stack_i curr_stack{};
    std::stack<std::pair<i_stack<GmInstr>,stack_i>> dump{};
    i_stack<GmInstr> code;

    public:
        G_machine(auto&& lib, auto&& curr, auto&& dump, auto&& code) : 
            used_function_library(std::forward<decltype(lib)>(lib)),
            curr_stack(std::forward<decltype(curr)>(curr)),
            dump(std::forward<decltype(dump)>(dump)),
            code(std::forward<decltype(code)>(code))
        { }

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
                assert(cond->index()==NUME);
                if (std::get<NUME>(*cond)==0){
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
                dump.push(std::pair(code,curr_stack));
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
                if (!(curr_stack.size()+1>=sco->arity)) {
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
                SCo* sco = &used_function_library[std::get<GLOB>(val.second)];
                curr_stack.push(std::make_shared<GmNode>(sco));
            } break;
            case VALUE: {
                curr_stack.push(std::make_shared<GmNode>(std::get<VALUE>(val.second)));
            } break; 
            case ARG: {
                curr_stack.push(curr_stack[std::get<ARG>(val.second) + 1]);
            } break; 
            case LOCAL: {
                curr_stack.push(curr_stack[std::get<LOCAL>(val.second)]);
            } break; 
        }
    }
    auto getNums () -> std::pair<int,int>{
        auto a = std::move(curr_stack.top());
        curr_stack.pop();
        auto b = std::move(curr_stack.top());
        curr_stack.pop();
        assert(a->index()==NUME && b->index()==NUME);
        return std::pair(std::get<NUME>(*a),std::get<NUME>(*b));
    }
    auto end() -> bool{
        bool whnf = curr_stack.size()==1 && curr_stack.top()->index()==NUME;
            return code.empty() || (dump.empty() && whnf);
    }
};

    // std::map<std::string, SCo> used_function_library;
    // // stack but we need indexing
    // stack_i curr_stack;
    // std::stack<std::pair<i_stack<GmInstr>,stack_i>> dump;
    // i_stack<GmInstr> code;
auto main () -> int {
     SCo omega={0, {}};
     SCo mul={1, {}};
    ;
     std::vector<GmInstr> vec ={{UNWIND, std::monostate()},{PUSH,
             std::pair(GLOB, std::string("main"))}};
     std::vector<GmInstr> ex2 ={{UNWIND,std::monostate()}, {SLIDE, 1}, {MUL, std::monostate()}, {EVAL, std::monostate()}, 
{PUSH, std::pair(VALUE,
                 GmValVal{std::in_place_index<VALUE>, 10})}, 
                 {EVAL, std::monostate()}, 
{PUSH, std::pair(VALUE,
                 GmValVal{std::in_place_index<VALUE>, 10})}
                 };

     // Push (Arg 0), Eval, Push (Arg 2), Eval, Mul, Slide 3, Unwind 
     omega.code.insert(vec);
     mul.code.insert(ex2);
    std::map<std::string, SCo> lib ={};
    lib["main"]=omega;
    // G_machine mach{lib, {}, {}, {{UNWIND, std::monostate()}, {PUSH,
             // std::pair(GLOB, std::string("main"))}}};
    i_stack<GmInstr> i;
    i.insert({{UNWIND, std::monostate()},
            {PUSH,
             GmVal{std::pair(GLOB, std::string("main"))}}});
    G_machine mach(lib,stack_i{},std::stack<std::pair<i_stack<GmInstr>,stack_i>>{},i);
    printf("res: %d", mach.eval());
}

