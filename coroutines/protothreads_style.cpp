#include <iostream>

#define BEGIN(ctx) switch(ctx->state){ case 0:{
#define YIELD(ctx) ctx->state = __LINE__; return;} case __LINE__: {
#define WAIT_UNTIL(ctx, pred) do {ctx->state = __LINE__; case __LINE__:; if (!pred()) return;} while(0);
#define END(ctx) ctx->state = -1;}}

struct FooCtx {
    int state = 0;
};

struct BarCtx {
    int state = 0;
};

void foo0(FooCtx* ctx, auto pred){
    BEGIN(ctx)
        std::cout << "Foo0Step\n";
        WAIT_UNTIL(ctx, pred)
        std::cout << "Foo0Done\n";
    END(ctx)
}

void foo1(FooCtx* ctx, auto pred){
    BEGIN(ctx)
        std::cout << "Foo1Step\n";
        WAIT_UNTIL(ctx, pred)
        std::cout << "Foo1Done\n";
    END(ctx)
}

void foo2(FooCtx* ctx, auto pred){
    BEGIN(ctx)
        std::cout << "Foo2Step\n";
        WAIT_UNTIL(ctx, pred)
        std::cout << "Foo2Done\n"; 
    END(ctx)
}

void foo3(FooCtx* ctx, auto pred){
    BEGIN(ctx)
        std::cout << "Foo3Step\n";
        WAIT_UNTIL(ctx, pred)
        std::cout << "Foo3Done\n";
    END(ctx)
}


size_t counter = 0;
void bar(BarCtx* ctx){
    BEGIN(ctx)
        YIELD(ctx)
        ++counter;
        std::cout << "BarStep1\n";
        YIELD(ctx)
        ++counter;
        std::cout << "BarStep2\n";
        YIELD(ctx)
        ++counter;
        std::cout << "BarStep3\n";
        YIELD(ctx)
    END(ctx)
}

int main(){
    BarCtx bar_ctx;
    FooCtx foo_0_ctx, foo_1_ctx, foo_2_ctx, foo_3_ctx;
    for (int i = 0; i < 4; ++i){
        std::cout << "--------------------\n";
        std::cout << "Iter:" << i << '\n';
        bar(&bar_ctx);
        foo0(&foo_0_ctx,[]{return counter == 0;});
        foo1(&foo_1_ctx,[]{return counter == 1;});
        foo2(&foo_2_ctx,[]{return counter == 2;});
        foo3(&foo_3_ctx,[]{return counter == 3;});
        std::cout << "--------------------\n";
    }
}