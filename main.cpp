#include "leval.hpp"
#include <iostream>

using namespace LE;

int main() {
    LEfunction omega{0};
    omega.compile(omega({}));
    // omega.exec({}); // would cycle until memory runs out
    
    // Evaluates second argument if the first is not 0
    LEfunction if_exec {2};
    if_exec.compile(if_fun({Arg(0), Arg(1), 0}));
    LEfunction helper {1};
    helper.compile(if_exec({Arg(0), omega({})}));
    // helper.exec({1}); // would cycle to infinity
    // helper.exec({0}); // does not cycle and returns 0;
    
    LEfunction fact{1};
    fact.compile(Arg(0) * if_fun({Arg(0) == 1, 1, fact({Arg(0) - 1})}));
    // fact.exec({10}); // calculates factorial of 10

    LEfunction weird_plus {2};
    weird_plus.compile(
        (LEfunction(2).compile(Arg(0)+Arg(1)) // Lambda
        ({Arg(0)*Arg(1), Arg(1)}))            // arguments for the lambda
    );
    // weird_plus.exec({2,1}); // returns 4=(1*2)+2

    // Fibonacci function
    LEfunction fibo {1};
    fibo.compile(
        if_fun({Arg(0), 
            if_fun({Arg(0)-1,
                fibo({Arg(0)-1})+fibo({Arg(0)-2}),
                1}),
            0 
    }));

    // Argument sharing
    LEfunction mul2 {1};
    mul2.compile(Arg(0)*Arg(0));
    // LEfunction share{1}; (only one multiplication is performed)
}
