---
summary: "Workspace template for HEARTBEAT.md"
read_when:
  - Bootstrapping a workspace manually
---

# HEARTBEAT.md

# Keep this file empty (or with only comments) to skip heartbeat API calls.

# Add tasks below when you want the agent to check something periodically.

## Active tasks

- If there is no urgent user request, spend heartbeat cycles improving `rtm3d-cli` incrementally:
  - prioritize code organization/refactor before synthetic-data expansion:
    - split large RTM functions by responsibility (source propagation, receiver backpropagation, imaging condition)
    - improve file/module structure for discoverability and testability
    - run static analysis (prefer clang-tidy, fallback cppcheck) and fix issues incrementally
  - then continue with benchmark realism (synthetic scenarios, multi-shot, stronger metrics)
  - for each cycle: implement + run relevant tests + commit with clear message
  - at the start of each cycle, review prioritized backlog in memory and execute the top actionable item
  - if a strong new idea appears, append it to backlog memory with clear priority placement
  - avoid idle cycles
