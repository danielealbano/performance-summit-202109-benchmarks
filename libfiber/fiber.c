/**
 * Copyright (C) 2020-2021 Daniele Salvatore Albano
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include "fiber.h"

void* xalloc_alloc_aligned_zero(
        size_t alignment,
        size_t size) {
    void* memptr;

    memptr = aligned_alloc(alignment, size);

    if (memptr == NULL) {
        fprintf(stderr, "Unable to allocate the requested memory %lu aligned to %lu", size, alignment);
        exit(-1);
    }

    if (memset(memptr, 0, size) != memptr) {
        fprintf(stderr, "Unable to zero the requested memory %lu", size);
        exit(-1);
    }

    return memptr;
}


void fiber_stack_protection(
        fiber_t *fiber,
        bool enable) {
    int stack_usage_flags = enable ? PROT_NONE : PROT_READ | PROT_WRITE;
    size_t page_size = getpagesize();;

    if (mprotect(fiber->stack_base, page_size, stack_usage_flags) != 0) {
        if (errno == ENOMEM) {
            fprintf(stderr, "Unable to protect/unprotect fiber stack, review the value of /proc/sys/vm/max_map_count");
            exit(-1);
        }

        fprintf(stderr, "Unable to protect/unprotect fiber stack");
        exit(-1);
    }
}

fiber_t *fiber_new(
        size_t stack_size,
        fiber_start_fp_t *fiber_start_fp,
        void *user_data) {
    size_t page_size = getpagesize();
    fiber_t *fiber = malloc(sizeof(fiber_t));
    memset(fiber, 0, sizeof(fiber_t));
    void *stack_base = xalloc_alloc_aligned_zero(page_size, stack_size);

    // Align the stack_pointer to 16 bytes and leave the 128 bytes red zone free as per ABI requirements
    void* stack_pointer = (void*)((uintptr_t)(stack_base + stack_size) & -16L) - 128;

    // Need room on the stack as we push/pop a return address to jump to our function
    stack_pointer -= sizeof(void*) * 1;

    fiber->start_fp = fiber_start_fp;
    fiber->start_fp_user_data = user_data;
    fiber->stack_size = stack_size;
    fiber->stack_base = stack_base;
    fiber->stack_pointer = stack_pointer;

    // Set the initial fp and rsp of the fiber
    fiber->context.rip = fiber->start_fp; // this or the stack_base? who knows :|
    fiber->context.rsp = fiber->stack_pointer;

    fiber_stack_protection(fiber, true);

    return fiber;
}

void fiber_free(
        fiber_t *fiber) {
    fiber_stack_protection(fiber, false);

    free(fiber->stack_base);
    free(fiber);
}