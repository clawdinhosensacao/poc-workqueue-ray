extern "C" {
#include "work_queue.h"
}

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    int port = 9123;
    int tasks = 40;
    int bytes = 4 * 1024 * 1024;
    int level = 6;
    std::string outdir = "wq_results";

    if (argc > 1) tasks = std::stoi(argv[1]);
    if (argc > 2) bytes = std::stoi(argv[2]);
    if (argc > 3) port = std::stoi(argv[3]);
    if (argc > 4) outdir = argv[4];

    std::filesystem::create_directories(outdir);

    work_queue* q = work_queue_create(port);
    if (!q) {
        std::cerr << "Failed to create queue on port " << port << "\n";
        return 1;
    }

    std::cout << "WORK_QUEUE_PORT=" << work_queue_port(q) << "\n";
    std::cout << "Submitting " << tasks << " tasks\n";

    auto t0 = std::chrono::steady_clock::now();

    for (int i = 0; i < tasks; ++i) {
        std::string bin_name = "out_" + std::to_string(i) + ".bin";
        std::string stats_name = "stats_" + std::to_string(i) + ".json";
        std::string cmd = "./compress_task --seed " + std::to_string(i) +
                          " --bytes " + std::to_string(bytes) +
                          " --level " + std::to_string(level) +
                          " --output " + bin_name +
                          " --stats " + stats_name;

        work_queue_task* t = work_queue_task_create(cmd.c_str());
        work_queue_task_specify_file(t, "./compress_task", "compress_task", WORK_QUEUE_INPUT, WORK_QUEUE_CACHE);

        std::string local_bin = outdir + "/" + bin_name;
        std::string local_stats = outdir + "/" + stats_name;
        work_queue_task_specify_file(t, local_bin.c_str(), bin_name.c_str(), WORK_QUEUE_OUTPUT, WORK_QUEUE_NOCACHE);
        work_queue_task_specify_file(t, local_stats.c_str(), stats_name.c_str(), WORK_QUEUE_OUTPUT, WORK_QUEUE_NOCACHE);

        int id = work_queue_submit(q, t);
        std::cout << "SUBMITTED task_id=" << id << " seed=" << i << "\n";
    }

    int done = 0;
    int failed = 0;
    while (!work_queue_empty(q)) {
        work_queue_task* t = work_queue_wait(q, 5);
        if (!t) continue;
        ++done;
        if (t->return_status != 0) ++failed;
        std::cout << "DONE task_id=" << t->taskid << " return_status=" << t->return_status
                  << " host=" << (t->host ? t->host : "unknown") << "\n";
        work_queue_task_delete(t);
    }

    auto t1 = std::chrono::steady_clock::now();
    auto wall_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cout << "SUMMARY tasks=" << tasks << " done=" << done << " failed=" << failed
              << " wall_ms=" << wall_ms << "\n";

    work_queue_delete(q);
    return failed == 0 ? 0 : 2;
}
