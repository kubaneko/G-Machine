#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <map>
#include <stack>
#include <stdexcept>
#include <cassert>

// vector stack with exposed underlying vector
// because we need indexing, insert of vectors and other operations
template<typename T>
class i_stack : public std::stack<T,std::vector<T>>{
    protected: 
        using std::stack<T,std::vector<T>>::c;
    public:
    // reverse indexing
    auto operator[]( int pos ) -> T&{
        assert(c.size()>0);
        return c[c.size()-1-size_t(pos)];
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
    SCo (){}
    SCo (int arity, std::vector<GmInstr> code_s) : arity(arity){
        code.insert(code_s);
    }
};

using GmInstrData=std::variant<int, GmVal, 
            std::pair<std::vector<GmInstr>,std::vector<GmInstr>>,std::monostate>;

struct Arg;

struct function_prototype{
    int hash=0;
    std::vector<std::variant<function_prototype, int, Arg>> args;
    function_prototype operator+ (auto&& a){
        return {ADD_M,{*this,std::forward<decltype(a)>(a)}};
    }

    function_prototype operator- (auto&& a){
        return {SUB_M,{*this,std::forward<decltype(a)>(a)}};
    }

    function_prototype operator* (auto&& a){
        return {MUL_M,{*this,std::forward<decltype(a)>(a)}};
    }

    function_prototype operator/ (auto&& a){
        return {DIV_M,{*this,std::forward<decltype(a)>(a)}};
    }

    function_prototype operator== (auto&& a){
        return {EQU_M,{*this,std::forward<decltype(a)>(a)}};
    }
    function_prototype operator()
        (std::vector<std::variant<function_prototype, int, Arg>> args_t){
        args=args_t;
        return (*this);
    }
};

using body_t=std::variant<function_prototype, int, Arg>;

struct Arg {
    int arg;
    explicit Arg(int arg) : arg(arg){}
    function_prototype operator+ (auto&& a){
        return {ADD_M,{*this,std::forward<decltype(a)>(a)}};
    }

    function_prototype operator- (auto&& a){
        return {SUB_M,{*this,std::forward<decltype(a)>(a)}};
    }

    function_prototype operator* (auto&& a){
        return {MUL_M,{*this,std::forward<decltype(a)>(a)}};
    }

    function_prototype operator/ (auto&& a){
        return {DIV_M,{*this,std::forward<decltype(a)>(a)}};
    }

    function_prototype operator== (auto&& a){
        return {EQU_M,{*this,std::forward<decltype(a)>(a)}};
    }
};


using Arguments = std::vector<body_t>;

using fun_lib=std::map<int, SCo>;

fun_lib initialize_fun_lib(){
    fun_lib lib={};
    std::vector<GmInstr> binop ={{UNWIND,std::monostate()}, {SLIDE, 3}, {ADD,
         std::monostate()}, {EVAL, std::monostate()}, {PUSH, std::pair(ARG, 2)}, {EVAL,
             std::monostate()}, {PUSH, std::pair(ARG, 0)} };
    GmInstr cond={COND,std::pair(std::vector<GmInstr>{{PUSH,std::pair(ARG,1)}},
                std::vector<GmInstr>{{PUSH,std::pair(ARG,2)}})};
        std::vector<GmInstr> if_code = {{UNWIND,std::monostate()}, {SLIDE,4}, cond, 
        {EVAL,std::monostate()}, {PUSH, std::pair(ARG, 0)}};
    lib[ADD_M]={2,binop};
    binop[2].instr=SUB;
    lib[SUB_M]={2,binop};
    binop[2].instr=DIV;
    lib[DIV_M]={2,binop};
    binop[2].instr=MUL;
    lib[MUL_M]={2,binop};
    binop[2].instr=EQU;
    lib[EQU_M]={2,binop};
    lib[IF_M]={3,if_code};
    return lib;
};

function_prototype if_fun{IF_M, {}};

fun_lib function_library=initialize_fun_lib();

struct SCo;


struct App;

using GmNode=std::variant<int,SCo*,App>;
using stack_i=i_stack<std::shared_ptr<GmNode>>;
using dump_t=std::stack<std::pair<i_stack<GmInstr>,stack_i>>;


struct App{
    std::shared_ptr<GmNode> function;
    std::shared_ptr<GmNode> arg;
    App(auto&& f, auto&& x){
        function=std::forward<decltype(f)>(f);
        arg=std::forward<decltype(x)>(x);
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
        G_machine(auto&& lib, auto&& curr, auto&& dump, auto&& code) : 
            used_function_library(std::forward<decltype(lib)>(lib)),
            curr_stack(std::forward<decltype(curr)>(curr)),
            dump(std::forward<decltype(dump)>(dump)),
            code(std::forward<decltype(code)>(code))
        { }

    void perform_instr(GmInstr&& next){
        printf("%d %d\n",next.instr, curr_stack.size());
        fflush(stdout);
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
                printf("ev %d\n", nd->index());
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
        printf("unw %d\n", curr_stack.top()->index());
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
                if (!(curr_stack.size()+1>=size_t(sco->arity))) {
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
        assert(a->index()==NUME && b->index()==NUME);
        return std::pair(std::get<NUME>(*a),std::get<NUME>(*b));
    }
    auto end() -> bool{
        bool whnf = curr_stack.size()==1 && curr_stack.top()->index()==NUME;
            return code.empty() || (dump.empty() && whnf);
    }
};

class LEfunction {
    int hash=-1;
    int arity=-1;
    std::vector<GmInstr> code{};
    std::map<int, SCo>* globals=&function_library;

    public:
LEfunction (int arity, auto library) : arity(arity){
    globals=library;
    (*globals)[globals->size()]={};
    hash=globals->size();
};

LEfunction (int arity) : arity(arity){
    (*globals)[globals->size()]={};
    hash=globals->size();
};

// allows for non-recursive lamdas
// {2}.compile(plus(Arg(1), Arg(2)))({1,2})
LEfunction compile(auto&& body){
    int pushed=0;
    compile_helper(std::forward<decltype(body)>(body), pushed);
    code.push_back({SLIDE,arity+1});
    code.push_back({UNWIND, std::monostate()});
    std::reverse(code.begin(),code.end());
    (*globals)[hash]={arity,code};
    return *this;
}

void compile_helper(body_t body, int &pushed){
    switch (body.index()){
        case GLOB : {
            function_prototype f=std::get<GLOB>(body);
            code.push_back({PUSH,std::pair(GLOB,f.hash)});
            ++pushed;
            for (auto&& arg : f.args) {
                compile_helper(arg, pushed);
                code.push_back({MKAP,std::monostate()});
                --pushed;
            }
        } break;
        case VALUE : {
            printf("val %d\n",std::get<VALUE>(body));
            code.push_back({PUSH,std::pair(VALUE,std::get<VALUE>(body))});
            ++pushed;
        } break;
        case ARG : {
            code.push_back({PUSH,std::pair(ARG,std::get<ARG>(body).arg+pushed)});
            ++pushed;
        } break;
    }
}

int exec(std::vector<int> args){
    std::vector<GmInstr> init_code{};
    init_code.push_back({UNWIND,std::monostate()});
    for (auto&& arg : args){
        init_code.push_back({MKAP,std::monostate()});
        init_code.push_back({PUSH,std::pair(VALUE,arg)});
    }
    init_code.push_back({PUSH,std::pair(GLOB,hash)});
    i_stack<GmInstr> init{};
    init.insert(std::move(init_code));
    G_machine g={globals, stack_i{}, dump_t{}, std::move(init)};
    return g.eval();
}

function_prototype operator() (Arguments arguments){
    return {hash,arguments};
}

};


auto main () -> int {
     SCo omega={0, {}};
     SCo mul={1, {}};
    ;
     std::vector<GmInstr> vec ={{UNWIND, std::monostate()},{PUSH,
             std::pair(GLOB, 1)}};
     std::vector<GmInstr> ex2 ={{UNWIND,std::monostate()}, {SLIDE, 3}, {MUL,
         std::monostate()}, {EVAL, std::monostate()}, {PUSH, std::pair(ARG, 1)}, {EVAL,
             std::monostate()}, {PUSH, std::pair(ARG, 0)} };

     // Push (Arg 0), Eval, Push (Arg 2), Eval, Mul, Slide 3, Unwind 
     omega.code.insert(vec);
     mul.code.insert(ex2);
    fun_lib lib ={};
    lib[1]=omega;
    LEfunction a={3, &function_library};
    LEfunction b={3, &function_library};
    a({b({1,2,3}),2,3});
    i_stack<GmInstr> i;
    i.insert({{UNWIND, std::monostate()},
            {PUSH,
             GmVal{std::pair(GLOB, 1)}}});
    G_machine mach(&lib,stack_i{},dump_t{},i);
    LEfunction fact{1};
    fact.compile(Arg(0)*if_fun({Arg(0)==1,1,fact({Arg(0)-1})}));
    // mul arg mkap if eq arg mkap 1 mkap mkap 1 mkap fact minus arg mkap 1
    // mkap mkap mkap mkap SLIDE2 UNWIND
    int l=fact.exec({10});
    printf("res: %d", l);
}

