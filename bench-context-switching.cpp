#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <benchmark/benchmark.h>
#include <pthread.h>
#include <unistd.h>

#include "libfiber/fiber.h"

#define MSG_TEXT "tst"
#define MSG_TEXT_SIZE (strlen(MSG_TEXT) + 1)

typedef struct pipe_info pipe_info_t;
struct pipe_info {
    int read_fd;
    int write_fd;
};

void* thread_func(void* p) {
    char read_buf[MSG_TEXT_SIZE] = { 0 };
    auto pipe_info = (pipe_info_t*)p;

    while (true) {
        int read_rc = read(pipe_info->read_fd, read_buf, MSG_TEXT_SIZE);

        if (read_rc < 0) {
            perror("read thread");
            break;
        } else if (read_rc != MSG_TEXT_SIZE) {
            break;
        }

        if (write(pipe_info->write_fd, read_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
            perror("write thread");
            break;
        }
    }

    return nullptr;
}

void BM_ContextSwitching_Reference(benchmark::State& state) {
    int fds[2];
    const char write_buf[MSG_TEXT_SIZE] = MSG_TEXT;
    char read_buf[MSG_TEXT_SIZE] = { 0 };

    if (pipe(fds) == -1) {
        perror("pipe");
        return;
    }

    // Self test
    if (write(fds[1], write_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
        perror("write self-test");
        return;
    }

    if (read(fds[0], read_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
        perror("read self-test");
        return;
    }

    if (strcmp(read_buf, write_buf) != 0) {
        fprintf(stderr, "buffers mismatch\n");
        return;
    }

    // Measure ops
    for (auto _ : state) {
        if (write(fds[1], write_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
            perror("write loop");
            break;
        }

        if (read(fds[0], read_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
            perror("read loop");
            break;
        }
    }

    close(fds[0]);
    close(fds[1]);
}

void BM_ContextSwitching_OsOverheadUnpinned(benchmark::State& state) {
    int main_to_child[2], child_to_main[2];
    const char write_buf[MSG_TEXT_SIZE] = MSG_TEXT;
    char read_buf[MSG_TEXT_SIZE] = { 0 };

    if (pipe(main_to_child) == -1) {
        perror("pipe main_to_child");
        return;
    }

    if (pipe(child_to_main) == -1) {
        perror("pipe child_to_main");
        return;
    }

    pipe_info_t main_fds = {
            .read_fd = child_to_main[0],
            .write_fd = main_to_child[1]
    };

    pipe_info_t child_fds = {
            .read_fd = main_to_child[0],
            .write_fd = child_to_main[1]
    };

    pthread_t child_thread;
    pthread_create(&child_thread, nullptr, thread_func, (void*)&child_fds);

    // Self test
    if (write(main_fds.write_fd, write_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
        perror("write self-test");
        return;
    }

    if (read(main_fds.read_fd, read_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
        perror("read self-test");
        return;
    }

    if (strcmp(read_buf, write_buf) != 0) {
        fprintf(stderr,"buffers mismatch\n");
        return;
    }

    // Measure ops
    for (auto _ : state) {
        if (write(main_fds.write_fd, write_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
            perror("write loop");
            break;
        }

        if (read(main_fds.read_fd, read_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
            perror("read loop");
            break;
        }
    }

    close(main_fds.write_fd);
    close(main_fds.read_fd);

    if (pthread_join(child_thread, nullptr)) {
        perror("pthread_join");
    }
}

void BM_ContextSwitching_OsOverheadPinned(benchmark::State& state) {
    int main_to_child[2], child_to_main[2];
    uint core_index;
    const char write_buf[MSG_TEXT_SIZE] = MSG_TEXT;
    char read_buf[MSG_TEXT_SIZE] = {0 };
    cpu_set_t cpuset;

    if (pipe(main_to_child) == -1) {
        perror("pipe main_to_child");
        return;
    }

    if (pipe(child_to_main) == -1) {
        perror("pipe child_to_main");
        return;
    }

    pipe_info_t main_fds = {
            .read_fd = child_to_main[0],
            .write_fd = main_to_child[1]
    };

    pipe_info_t child_fds = {
            .read_fd = main_to_child[0],
            .write_fd = child_to_main[1]
    };

    pthread_t child_thread;
    pthread_create(&child_thread, nullptr, thread_func, (void*)&child_fds);

    getcpu(&core_index, nullptr);
    CPU_ZERO(&cpuset);
    CPU_SET(core_index, &cpuset);
    pthread_setaffinity_np(child_thread, sizeof(cpu_set_t), &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    // Self test
    if (write(main_fds.write_fd, write_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
        perror("write self-test");
        return;
    }

    if (read(main_fds.read_fd, read_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
        perror("read  self-test");
        return;
    }

    if (strcmp(read_buf, write_buf) != 0) {
        fprintf(stderr, "buffers mismatch\n");
        return;
    }

    // Measure ops
    for (auto _ : state) {
        if (write(main_fds.write_fd, write_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
            perror("write loop");
            break;
        }

        if (read(main_fds.read_fd, read_buf, MSG_TEXT_SIZE) != MSG_TEXT_SIZE) {
            perror("read loop");
            break;
        }
    }

    close(main_fds.write_fd);
    close(main_fds.read_fd);

    if (pthread_join(child_thread, nullptr)) {
        perror("pthread_join");
    }
}

[[noreturn]]
void fiber_func(fiber_t* fiber_from, fiber_t* fiber_to) {
    while (true) {
        fiber_context_swap(fiber_to, fiber_from);
    }
}

void BM_ContextSwitching_Fiber2XPinnedOverhead(benchmark::State& state) {
    uint core_index;
    cpu_set_t cpuset;
    fiber_t main_context = { 0 };

    getcpu(&core_index, nullptr);
    CPU_ZERO(&cpuset);
    CPU_SET(core_index, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    fiber_t* child_fiber = fiber_new(getpagesize() * 8, fiber_func, nullptr);

    for (auto _ : state) {
        fiber_context_swap(&main_context, child_fiber);
    }

    fiber_free(child_fiber);
}

static void BenchArguments(benchmark::internal::Benchmark* b) {
    b->Iterations(1000000);
}

BENCHMARK(BM_ContextSwitching_Reference)
        ->Apply(BenchArguments);
BENCHMARK(BM_ContextSwitching_OsOverheadUnpinned)
        ->Apply(BenchArguments);
BENCHMARK(BM_ContextSwitching_OsOverheadPinned)
        ->Apply(BenchArguments);
BENCHMARK(BM_ContextSwitching_Fiber2XPinnedOverhead)
        ->Apply(BenchArguments);
