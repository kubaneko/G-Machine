# User documentation

## Description

Leval is a header only library for numerical computations with lazy evaluation and ML style
syntax. The user can add, subtract, divide, multiply numbers and test their
equality. The user can also define conditionals with the if_fun function that
if its first argument evaluates to 1 evaluates its second argument and
otherwise its third argument.

## Usage

- The whole library is in the LE namespace.
- User defined functions are of the LEfunction type
- Constructed functions need arity and can take pointer to a function_library 
  (fun_lib type) as an argument (otherwise they take the global fun_lib)
- If the user wants their own fun_lib it should be initialize with the
  initialize_fun_lib function before constructing functions with it.
- As expected LEfunctions can be compiled (.compile()) and executed (.exec())
    - compile takes body of the function composed of:
        - function_prototypes - we get them by calling the function
            with vector of its arguments e.g. Add({Arg(0),Arg(1)})
            - built-in operations are the same operators as with other types e.g. Arg(0)+1
            - with the exception of conditional that is defined by if_fun function
        - Arguments in Arg structure (Arg(0)) from 0 to arity-1
        - integers
- The execution similarly takes its arguments (integers) e.g. Add.exec({1,2})

## Examples

- Omega-combinator calls itself and enters infinite cycle
```cpp
LEfunction omega{0};
omega.compile(omega({}));
omega.exec();

```

- Evaluates its second argument if its first argument is not 0 otherwise returns 0
```cpp
LEfunction if_exec {2};
if_exec.compile(if_fun({Arg(0), Arg(1), 0}))
LEfunction helper {1};
helper.compile(if_exec({Arg(0), omega({})}));
```
Since we have lazy evaluation helper terminates if and only if its argument is 0

- Definition of factorial
```cpp
LEfunction fact{1};
fact.compile(Arg(0) * if_fun({Arg(0) == 1, 1, fact({Arg(0) - 1})}));
```

## Practical tips

- For bigger computation make your own fun_lib and after it dealocate it. The global fun_lib cannot safely remove functions so it is mostly for smaller computations.

- we can define non-recursive lambdas in function definitions as follows
```cpp
LEfunction plus {2};
plus.compile(
{2}.compile(Arg(0)+Arg(1)) // Lambda
({Arg(0), Arg(1)});        // arguments for the lambda
);
```
