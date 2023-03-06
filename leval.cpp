#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <map>
#include <stack>


enum class GmVal {
    GLOB, 
    VALUE, 
    ARG, 
    LOCAL
};

enum class GmInstr { // Instructions for the Stack machine
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

template <GmInstr k, GmVal o>
struct GmInstruction;

template <GmVal k>
struct GmValue;

template <>
struct GmValue<GmVal::GLOB>{
    std::string ll;
};

template <GmVal k>
struct GmInstruction<GmInstr::PUSH, k>{
     GmValue<k>* value;
};

struct Arg{
    int arg;
};

using Argument = std::variant<Arg, int>;

class LEfunction;

class LEfunction {
std::vector<int> args;

std::shared_ptr<LEfunction> body;

// bool setBody(LEfunction bd){ 
//     for (int i=0;i<bd->arity();i++){
//         auto&& arg bd.getArg(i);
// 
//     }
// 
//     if bd LEfunction
//         check params
//     body=mkptr(bd);
// };

LEfunction (int sizeArg){
    args=std::vector<int>(sizeArg);

};

// int arity(){
//     return args.length();
// };

int getArg(int numArg){
    return args[numArg-1];
};

//bool emitInstr(&std::stack<Instr> st){
//    args.
//    visit
//
//    push i
//    body.emit
//}

};

static std::map<std::string, LEfunction> function_library;

class G_machine{
    std::stack<std::stack<GmVal>> dump;
    std::stack<GmVal> curr_stack;
    std::vector<GmInstr> instructs;
    int execute(LEfunction func) {
        return 1;
    };
};
