#include "leval.hpp"

using namespace LE;

int main(){
    LEfunction fact{1};
    fact.compile(Arg(0)*if_fun({Arg(0)==1,1,fact({Arg(0)-1})}));
    // mul arg mkap if eq arg mkap 1 mkap mkap 1 mkap fact minus arg mkap 1
    // mkap mkap mkap mkap SLIDE2 UNWIND
    int l=fact.exec({10});
    LEfunction omega{0};
    omega.compile(omega({}));
    LEfunction id_om{1};
    Arg a=Arg(0);
    id_om.compile(if_fun({1,1-a,omega({})}));
    int i=id_om.exec({10});
    auto r=1-3;
    printf("res: %d %d", i, 3);
}
