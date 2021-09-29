#ifndef FIBER_H
#define FIBER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fiber fiber_t;
typedef void (fiber_start_fp_t)(fiber_t* fiber_from, fiber_t* fiber_to);

struct fiber {
    struct {
        void *rip, *rsp;
        void *rbx, *rbp, *r12, *r13, *r14, *r15;
    } context;
    void* stack_pointer;
    void* stack_base;
    size_t stack_size;
    fiber_start_fp_t* start_fp;
    void* start_fp_user_data;
};

extern void fiber_context_get(
        fiber_t *fiber_context);

extern void fiber_context_set(
        fiber_t *fiber_context);

extern void fiber_context_swap(
        fiber_t *fiber_context_from,
        fiber_t *fiber_context_to);

void fiber_stack_protection(
        fiber_t* fiber,
        bool enable);

fiber_t* fiber_new(
        size_t stack_size,
        fiber_start_fp_t* fiber_start_fp,
        void* user_data);

void fiber_free(
        fiber_t* fiber);

#ifdef __cplusplus
}
#endif

#endif //FIBER_H