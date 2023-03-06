#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <map>
#include <stack>


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
    std::pair<GmInstrEnum,std::variant<std::monostate,int, GmVal, 
                std::pair<std::vector<GmInstr>,std::vector<GmInstr>>>> intr;
};

using GmInstrT=std::pair<GmInstrEnum,std::variant<std::monostate,int, GmVal, 
        std::pair<std::vector<GmInstr>,std::vector<GmInstr>>>>;

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
};

static std::map<std::string, LEfunction> function_library;

class G_machine{
    std::map<std::string, LEfunction> used_function_library;
    std::stack<std::stack<GmVal>> dump;
    std::stack<GmVal> curr_stack;
    std::vector<GmInstr> instructs;
};
