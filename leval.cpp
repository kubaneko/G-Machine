#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <map>
#include <stack>
#include <memory>
#include <vector>

enum class GmValEnum {
    GLOB, 
    VALUE, 
    ARG, 
    LOCAL
};

using GmVal=std::pair<GmValEnum,std::variant<int,std::string>>;

enum class GmInstrEnum { // Instructions for the Stack machine
    PUSH, 
    SLIDE, 
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
    GmInstrEnum instr;
    std::variant<std::monostate,int, GmVal, 
                std::pair<std::vector<GmInstr>,std::vector<GmInstr>>> data;
};

using GmInstrData=<std::monostate,int, GmVal, 
                std::pair<std::vector<GmInstr>,std::vector<GmInstr>>>;

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

using GmNode=std::variant<int,SCo,App>>;

// vector stack with exposed underlying vector
// because we need indexing, insert of vectors and other operations
class i_stack : public std::stack<std::shared_pointer<GmNode>,
                        std::vector<std::shared_pointer<GmNode>>>{
    using std::stack<std::shared_pointer<GmNode>>::c;
}


static std::map<std::string, LEfunction> function_library;

class G_machine{
    std::map<std::string, SCo> used_function_library;
    // stack but we need indexing
    using curr_stack_t = std::vector<>;
    curr_stack_t curr_stack;
    std::stack<std::pair<std::stack<GmInstr>,curr_stack_t>> dump;
    std::stack<GmInstr> instructs;
    int eval(){
        while (!instucts.empty()){
            GmInstr* next=instructs.top();
            instructs.pop();
            switch (next->intr) {
                case PUSH
                    GmVal* save=curr_stack.top();
                    switch (save->first) {
                        case GmValEnum.GLOB
                            std::string name=std::get<GLOB>(save->second);
                            
                        break;
                        case GmValEnum.VALUE, 
                        break;
                        case GmValEnum.ARG, 
                        break;
                        case GmValEnum.LOCAL
                        break;
                    }
                break;
                case SLIDE
                    GmVal* save=curr_stack.top();
                    int num=std::get<SLIDE>(next->data);
                    for (int i=0;i<=num;++i){
                        curr_stack.pop();
                    }
                    curr_stack.push(save);
                break;
                case COND
                    GmVal* cond=curr_stack.top();
                    auto code_pair=std::get<COND>(next->data);
                    curr_stack.pop();
                    assert(cond==GmValEnum.Num);
                    if (cond.second!=0){
                        for(int i=code_pair.first.length()-1; i>-1;--i){
                            curr_stack.push(code_pair.first[i]);
                        }
                    } else {
                        for(int i=code_pair.second.length()-1; i>-1;--i){
                            curr_stack.push(code_pair.second[i]);
                        }
                    }
                    // Add the new code to instructs
                break;
                case MKAP 
                break;
                case UNWIND
                break;
                case EVAL 
                    GmVal* nd=curr_stack.top();
                    curr_stack.pop();
                    dump.push(pair(instucts,curr_stack));
                    instructs=std::pair(GmInstrEnum.UNWIND,std::monostate);
                    curr_stack=std::stack(nd);
                break;
                case ADD 
                    auto arg=getNums();
                    curr_stack.push(arg.second+arg.first);
                break;
                case SUB 
                    auto arg=getNums();
                    curr_stack.push(arg.second-arg.first);
                break;
                case DIV 
                    auto arg=getNums();
                    curr_stack.push(arg.second/arg.first);
                break;
                case MUL 
                    auto arg=getNums();
                    curr_stack.push(arg.second*arg.first);
                break;
            }
        }
        return 1       
    }
    std::pair<int,int> getNums (){
        GmVal* a = curr_stack.top();
        curr_stack.pop();
        GmVal* b = curr_stack.top();
        curr_stack.pop();
        assert(a.first==GmValEnum.Num && b.first==GmValEnum.Num);
        return pair(std::get<1>(a.second),std::get<1>(b.second));
    }
};

struct SCo{
    int arity;
    std::vector<GmInstr> code;
}

struct App{
    void* function
    void* arg
}
