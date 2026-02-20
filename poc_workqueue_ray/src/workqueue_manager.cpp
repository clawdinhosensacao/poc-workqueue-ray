extern "C" {
#include "work_queue.h"
}

#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    int tasks = 40;
    int n = 1000000;
    int port = 9123;

    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        if (k == "--tasks" && i + 1 < argc) tasks = std::stoi(argv[++i]);
        else if (k == "--n" && i + 1 < argc) n = std::stoi(argv[++i]);
        else if (k == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
        else {
            std::cerr << "unknown arg: " << k << "\n";
            return 2;
        }
    }

    auto q = work_queue_create(port);
    if (!q) {
        std::cerr << "couldn't create queue: " << std::strerror(errno) << "\n";
        return 1;
    }

    std::cout << "manager_port=" << work_queue_port(q) << "\n";
    work_queue_specify_name(q, "wq-zlib-poc");

    for (int i = 0; i < tasks; ++i) {
        std::ostringstream cmd;
        cmd << "./compress_task --seed " << (1000 + i)
            << " --n " << n
            << " --level 3"
            << " --out out_" << i << ".bin";

        auto t = work_queue_task_create(cmd.str().c_str());
        work_queue_task_specify_file(t, "compress_task", "compress_task", WORK_QUEUE_INPUT, WORK_QUEUE_CACHE);

        std::string outname = "out_" + std::to_string(i) + ".bin";
        work_queue_task_specify_file(t, outname.c_str(), outname.c_str(), WORK_QUEUE_OUTPUT, WORK_QUEUE_NOCACHE);

        int taskid = work_queue_submit(q, t);
        if (taskid < 0) {
            std::cerr << "submit failed for task " << i << "\n";
        }
    }

    auto t0 = std::chrono::steady_clock::now();
    int done = 0;
    int failed = 0;

    while (!work_queue_empty(q)) {
        auto t = work_queue_wait(q, 5);
        if (!t) continue;

        done++;
        if (t->result != WORK_QUEUE_RESULT_SUCCESS || t->return_status != 0) {
            failed++;
            std::cerr << "task_fail id=" << t->taskid
                      << " result=" << t->result
                      << " return_status=" << t->return_status << "\n";
        }

        if (t->output) {
            std::cout << "task_id=" << t->taskid << " " << t->output;
        }

        work_queue_task_delete(t);
    }

    auto t1 = std::chrono::steady_clock::now();
    double wall_s = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "wq_summary tasks=" << tasks
              << " done=" << done
              << " failed=" << failed
              << " wall_s=" << wall_s
              << " tasks_per_s=" << (done / wall_s)
              << "\n";

    work_queue_delete(q);
    return failed == 0 ? 0 : 1;
}
