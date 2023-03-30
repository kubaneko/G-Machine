#include "leval.hpp"
#include <iostream>

using namespace LE;

int main() {
    LEfunction omega{0};
    omega.compile(omega({}));
    // omega.exec(); // would cycle until memory runs out
    
    // Evaluates second argument if the first is not 0
    LEfunction if_exec {2};
    if_exec.compile(if_fun({Arg(0), Arg(1), 0}));
    LEfunction helper {1};
    helper.compile(if_exec({Arg(0), omega({})}));
    // helper.exec(1); // would cycle to infinity
    // helper.exec(0); // does not cycle and returns 0;
    
    LEfunction fact{1};
    fact.compile(Arg(0) * if_fun({Arg(0) == 1, 1, fact({Arg(0) - 1})}));
    // fact.exec({10}); // calculates factorial of 10

    LEfunction weird_plus {2};
    weird_plus.compile(
        {2}.compile(Arg(0)+Arg(1))        // Lambda
        ({Arg(0)*Arg(1), Arg(1)})        // arguments for the lambda
    );
    // weird_plus.exec({1,2}) // returns 4=(1*2)+2
}
