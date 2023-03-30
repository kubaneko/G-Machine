# Developer documentation

## Description

Leval is a header only library for numerical computations based on lazy evaluation
specifically the virtual stack machine used by Haskell G-Machine. It has usuall
arithmetic operations and Conditional that executes based on whether its first
argument is 0. The library uses usual lamda calculus style definition of
functions. With the simplification that functions can only call other functions
(without higher order functions).

## Compilation

Compilation uses a simple strategy. We start with something like this:
(for function f with 2 arguments and @ representing application nodes)

```
->     @
      / \
->   @   Arg(1)
    / \
-> f   Arg(0)
```

Where the arrows represent the stack and the top of the stack is at the bottom.
We then push the first function in the body of f, compile its first argument
and make an application node out of them because we use functions in their
curried form. We continue building the tree for the body until we compile all
its arguments and make 1 application node from the whole body.

After that we save the tree and remove the previous tree of the function f.

We then UNWIND the application node and we get similar tree for the next evaluation.

## Representation

The library can be broadly separated into, function, intermediate
representation of the functions and the G-Machine.

### Code

The code is represented as a pair of Enum and a variant for the data for the
instructions. This was chosen because I prefer working with functional style
definitions more than OOP ones.

### G-Machine

G-Machine consists of current stack, dump and stack of code to execute. Current
stack represents data for evaluation of some function, the dump has all the
remaining evaluation still in progress (their code and stack). Code stack
contains instructions to be executed.

### Functions

There are two representation for functions function_prototype and LEfunction. 
- function_prototype is the syntactic representation for functions used to define bodies of other functions that is it contains only mapping to its code and its arguments
- LEfunction represents the whole function the important details about the function are its arity and its methods.

## References

G-Machine is inspired by the Haskell implementation. Which can be found here.

[Implementing lazy functional languages on stock hardoware: the Spineless Tagless G-machine Version 2.5 Simon L Peyton Jones](https://www.microsoft.com/en-us/research/wp-content/uploads/1992/04/spineless-tagless-gmachine.pdf)
