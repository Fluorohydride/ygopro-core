#include <stdint.h>
#include <functional>

#ifdef COROUTINE_IMPLEMENT_USINT_THREAD

#include <thread>
#include <mutex>
#include <condition_variable>

template<typename YIELD_TYPE = int32_t, typename RESUME_TYPE = int32_t>
class Coroutine {
public:
    Coroutine(std::function<void(Coroutine*, RESUME_TYPE)> co_fun) {
        coroutine_func = co_fun;
        std::unique_lock<std::mutex> lock(switch_lock);
        std::thread([this]() { coroutine_proc(); }).detach();
        yield_con.wait(lock);
    }
    
    bool resume(RESUME_TYPE v = RESUME_TYPE()) {
        if(finished)
            return false;
        std::unique_lock<std::mutex> lock(switch_lock);
        resume_value = v;
        resume_con.notify_one();
        yield_con.wait(lock);
        return !finished;
    }
    
    RESUME_TYPE yield(YIELD_TYPE v) {
        std::unique_lock<std::mutex> lock(switch_lock);
        yield_value = v;
        yield_con.notify_one();
        resume_con.wait(lock);
        return resume_value;
    }
    
    inline YIELD_TYPE get_yield_value() { return yield_value; }
    inline bool is_finished() { return finished; }
    
protected:
    void coroutine_proc() {
        {
            std::unique_lock<std::mutex> lock(switch_lock);
            yield_con.notify_one();
            resume_con.wait(lock);
        }
        coroutine_func(this, resume_value);
        finished = true;
        yield_con.notify_one();
    }
    
    std::function<void(Coroutine*, RESUME_TYPE)> coroutine_func;
    std::mutex switch_lock;
    std::condition_variable yield_con;
    std::condition_variable resume_con;
    YIELD_TYPE yield_value = YIELD_TYPE();
    RESUME_TYPE resume_value = RESUME_TYPE();
    bool finished = false;
};

#else // use local context system

#ifdef _WIN32
// use fiber library on windows
#include <windows.h>

template<typename YIELD_TYPE = int32_t, typename RESUME_TYPE = int32_t>
class Coroutine {
public:
    Coroutine(std::function<void(Coroutine*, RESUME_TYPE)> co_fun, size_t stack_size = 0) {
        coroutine_func = co_fun;
        parent = GetCurrentFiber();
        if(parent == (LPVOID)0x1E00)
            parent = ConvertThreadToFiber(0);
        if(GetFiberData() == nullptr) {
            static_counter()++;
            is_thread = true;
        }
        child = CreateFiber(stack_size, &coroutine_proc, this);
    }
    
    ~Coroutine() {
        DeleteFiber(child);
        if(is_thread && --static_counter() == 0)
            ConvertFiberToThread();
    }
    
    bool resume(RESUME_TYPE v = RESUME_TYPE()) {
        if(finished)
            return false;
        resume_value = v;
        SwitchToFiber(child);
        return !finished;
    }
    
    RESUME_TYPE yield(YIELD_TYPE v) {
        yield_value = v;
        SwitchToFiber(parent);
        return resume_value;
    }
    
    inline YIELD_TYPE get_yield_value() { return yield_value; }
    inline bool is_finished() { return finished; }
    
protected:
    static void WINAPI coroutine_proc(LPVOID param) {
        Coroutine* co = reinterpret_cast<Coroutine*>(param);
        co->coroutine_func(co, co->resume_value);
        co->finished = true;
        SwitchToFiber(co->parent);
    }
    
    inline int32_t& static_counter() { static thread_local int32_t coroutine_counter; return coroutine_counter; }
    
    std::function<void(Coroutine*, RESUME_TYPE)> coroutine_func;
    LPVOID parent;
    LPVOID child;
    YIELD_TYPE yield_value = YIELD_TYPE();
    RESUME_TYPE resume_value = RESUME_TYPE();
    bool is_thread = false;
    bool finished = false;
};

#else

#ifdef __MACH__
#define _XOPEN_SOURCE
#endif
#include <ucontext.h>

template<typename YIELD_TYPE = int32_t, typename RESUME_TYPE = int32_t>
class Coroutine {
public:
    Coroutine(std::function<void(Coroutine*, RESUME_TYPE)> co_fun, size_t stack_size = 0x10000) {
        coroutine_func = co_fun;
        getcontext(&child);
        child.uc_link = &parent;
        child.uc_stack.ss_sp = malloc(stack_size);
        child.uc_stack.ss_size = stack_size;
        child.uc_stack.ss_flags = 0;
        makecontext(&child, (void(*)(void))coroutine_proc, 1, this);
    }
    
    ~Coroutine() { free(child.uc_stack.ss_sp); }
    
    bool resume(RESUME_TYPE v = RESUME_TYPE()) {
        if(finished)
            return false;
        resume_value = v;
        swapcontext(&parent, &child);
        return !finished;
    }
    
    RESUME_TYPE yield(YIELD_TYPE v) {
        yield_value = v;
        swapcontext(&child, &parent);
        return resume_value;
    }
    
    inline YIELD_TYPE get_yield_value() { return yield_value; }
    inline bool is_finished() { return finished; }
    
protected:
    static void coroutine_proc(Coroutine* ycon) {
        ycon->coroutine_func(ycon, ycon->resume_value);
        ycon->finished = true;
    }
    
    std::function<void(Coroutine*, RESUME_TYPE)> coroutine_func;
    ucontext_t parent;
    ucontext_t child;
    YIELD_TYPE yield_value = YIELD_TYPE();
    RESUME_TYPE resume_value = RESUME_TYPE();
    bool finished = false;
};

#endif // _WIN32
#endif // use thread
