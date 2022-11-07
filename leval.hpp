#include <string>

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


