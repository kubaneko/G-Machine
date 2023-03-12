#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <map>
#include <stack>
#include <cassert>

enum GmValEnum {
    GLOB, 
    VALUE, 
    ARG, 
    LOCAL
};

using GmVal=std::pair<GmValEnum,std::variant<std::string,int,int,int>>;

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
    UNWIND
};

struct GmInstr{
    using GmInstrData=std::variant<int, GmVal, 
                std::pair<std::vector<GmInstr>,std::vector<GmInstr>>,std::monostate>;
    GmInstrEnum instr;
    GmInstrData data;
    GmInstr(GmInstrEnum instr, GmInstrData&& data){
        instr=instr;
        data=std::move(data);
    }
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

LEfunction (int sizeArg){
    args=std::vector<int>(sizeArg);

};

int getArg(int numArg){
    return args[numArg-1];
};
};

struct SCo{
    int arity;
    std::vector<GmInstr> code;
};

struct App;

using GmNode=std::variant<int,SCo*,App>;

struct App{
    std::shared_ptr<GmNode> function;
    std::shared_ptr<GmNode> arg;

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
    T& operator[]( size_t pos ){
        assert(c.size()>0);
        return c[c.size()-1-pos];
    }

    void insert(std::vector<T>& to_insert){
        c.insert(c.end(),to_insert.begin(),to_insert.end());
    }
    void insert(i_stack<T>&& to_insert){
        c.insert(c.end(),to_insert.c.begin(),to_insert.c.end());
    }
};
using stack_i=i_stack<std::shared_ptr<GmNode>>;


std::map<std::string, std::shared_ptr<SCo>> function_library;

class G_machine{
    std::map<std::string, std::shared_ptr<SCo>> used_function_library;
    // stack but we need indexing
    stack_i curr_stack;
    std::stack<std::pair<i_stack<GmInstr>,stack_i>> dump;
    i_stack<GmInstr> code;
    void perform_instr(GmInstr& next){
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
                code.push(GmInstr(UNWIND,std::monostate()));
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
        }
    }
    int eval(){
        while (!code.empty()){
            perform_instr(code.top());
            code.pop();
        }
        return 1;       
    }
    void unwind(){
    }
    void push(GmVal val){
        switch (val.first) {
            case GLOB: {
                SCo* sco=used_function_library[std::get<GLOB>(val.second)].get();
                curr_stack.push(std::make_shared<GmNode>(sco));
            } break;
            case VALUE: {
            } break; 
            case ARG: {
            } break; 
            case LOCAL: {
            } break; 
        }
    }
    std::pair<int,int> getNums (){
        auto&& a = std::move(curr_stack.top());
        curr_stack.pop();
        auto&& b = std::move(curr_stack.top());
        curr_stack.pop();
        assert(a->index()==NUME && b->index()==NUME);
        return std::pair(std::get<NUME>(*a),std::get<NUME>(*b));
    }
};

