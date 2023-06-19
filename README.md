# SOCC
SOCC means Simple but Okay C Compiler. Which is an embedded compiler for a C-like language.

## How to use it
Add this repo as subdirectory in your CMake project.
Link the library ***libsocc***.
```cmake
add_subdirectory(SOCC)
target_link_libraries(yourprogram libsocc)
```
## How to write program
SOCC is an embedded compiler which means it has no parser. It use c++ compiler as its parser, so you should write your program in .cpp file then compile, link, and run it to get machine code.

```cpp
#include <socc/library.hpp>
#include <soasm/asm.hpp>
#include <soasm/soisv1.hpp>
#include <iostream>
#include <iomanip>

using namespace SOCC;

int main(){
    Label main;
    Fn<u8(u8)> fib{"fib(i)"};
    Code program{
        fib(6_u8),
	    SOASM::SOISv1::Halt{}(),
	    fib.impl([&](auto& _,auto i)->Stmt{return {
	        if_(i <= 1_u8).then({
	            _.return_(i),
	        }).else_({
                _.return_(fib(i - 1_u8) + fib(i - 2_u8)),
            }),
        };}),
    };
    
    auto data=program.assemble();
    
	std::cout<<std::setfill('0')<<std::hex;
    for(auto [i,v]:std::views::zip(std::views::iota(0),data)){
        std::cout<<std::setw(2)<<(int)v<<" ";
        if(i%8==7){
	        std::cout<<std::endl;
		}
    }
    
    return 0;
}
```
## Language Reference

### Value and Variable
Value just like rvalue, can produce code to push data to stack.
Var just like lvalue, can produce code to push/pop data to/from stack.
RegVar is Var which store data on Register.
LocalVar is Var which store data on Function Stack Frame.
StaticVar is Var which store store data in RAM, need initialize on start.
ReadOnlyVar is Var which store store data in ROM.
```cpp

//variable allocator
StaticVars svar;
ReadOnlyVars rovar;

//variable declaration
u8 static_var =svar= 5_u8;
u8 const_var =rovar= 6_u8;

Code program{
    svar.init,//initialize StaticVars (should run at start)
    //...
    //halt here,
    rovar,//space for ReadOnlyVars (should not run and in ROM)
    
    //ram start
    svar,//space for StaticVars (should not run and in RAM)
};
```

### Type System
`Type<Size>` is some data which may load()/save() Size of bytes data on stack.
Stmt is will force to clear data and left nothing on stack.
Stmts is lots of Stmt.

#### Builtin Types
```
void_ = Type<0>;
bool_ = Type<1>;
Int<Size,Signed> = Type<Size>;
u8  = Int<1,false>;// uint8_t
u16 = Int<2,false>;// uint16_t
i8  = Int<1,true >;//  int8_t
i16 = Int<2,true >;//  int16_t
usize  = Int<PTR_SIZE,fasle>;
isize  = Int<PTR_SIZE,true >;
ptr<T> = usize;// T*
Fn<Ret(Arg1,Arg2,...)> fn;// Ret fn(Arg1,Arg2,...);
```
#### Builtin Literal
```
Val::none   -> void_
Val::true_  -> bool_
Val::false_ -> bool_
123_u8      -> u8
112_i16     -> i16
```

### Custom Types
```cpp

//u8 A[3];
using A=Array<u8,3>;

//struct Vec{u8 x,y,z;};
struct Vec:Struct<u8,u8,u8>{
    #define THIS Vec
    #define BASE Struct<u8,u8,u8>
    #define members M(x) M(y) M(z)
    #include "socc/define_members.inc"
};

//Union C{u8 a;i16 b;};
struct C:Union<u8,i16>{
    #define THIS C
    #define BASE Struct<u8,i16>
    #define members M(a) M(b)
    #include "socc/define_members.inc"
};
```

### if-elif-else

```cpp
Code program{
    if_( bool_ ).then({
        Stmts
    }).end(),

    if_( bool_ ).then({
        Stmts
    }).elif( bool_ ).then({
        Stmts
    }).else_({
        Stmts
    }), // else_ no need .end()
    
    //just like ?: operator
    if_< T >( bool_ ).then( T ).else_( T ),
};
```
### Loop

```cpp
Code program{
    while_( bool_ ).do_({
        Stmts
    }).end(),
    do_({
        Stmts
    }).while_( bool_ ),
    for_( void_ , bool_ , void_ ).do_({
        Stmts
    }).end(),
};
```

### Loop with break and continue
```cpp
Code program{
    while_( bool_ ).do_([](auto _){return Stmt{
        while_( bool_ ).do_([&](auto _1){return Stmt{
            _.break_,//will break outer loop
            _1.continue_,//will continue inner loop
        };}).end(),
    };}).end(),
};
```

### Function
```cpp
//function declare name is optional
Fn<Ret(Arg1,Arg2,...)> fib{"fib(i)"};
Code program{

    fib(Expr<Arg1>,Expr<Arg2>,...),//->same as Expr<Ret>
    
    //function implmention
    //type of arg1 is LocalVar<Arg1>, etc.
    fib.impl([&](auto& _,auto arg1,auto arg2,...)->Stmt{
        T local0{_};//declare LocalVar<T>
        return {
            _.ret,//direct access return value as LocalVar
            _.return_( Ret ),
        };
    }),
};
```