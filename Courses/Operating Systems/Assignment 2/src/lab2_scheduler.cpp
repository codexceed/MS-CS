#include <iostream>
#include <list>
#include <iterator>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <map>

using namespace std;


// Globals
bool DEBUG = false, VERBOSE = false;
char scheduler_code;
int ofs = 0, quantum = INT_MAX, maxprio = 4;
vector<int> rand_vals;
string INPUT_FILE_PATH, RAND_FILE_PATH;


// Constants
enum ProcessState {
    CREATED, READY, RUNNING, BLOCKED, TERMINATED
};
static const char *state_string[] = {"CREATED", "READY", "RUNNG", "BLOCK"};
enum StateTransition {
    TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT
};


void initRandNums() {
    ifstream rand_file(RAND_FILE_PATH);
    int num, n;
    rand_file >> n;
    for (int i = 0; i < n; i++) {
        rand_file >> num;
        rand_vals.push_back(num);
    }
}


int myrandom(int burst) {
    ofs++;
    return 1 + (rand_vals[(ofs - 1) % rand_vals.size()] % burst);
}


class Process {
public:
    int AT, TC, CB, IO, PID, burst_left = 0, static_priority, dynamic_priority, remaining_time, finishing_time, io_time = 0, wait_time = 0, ready_stamp;
    bool prioritised = false, can_preempt = false;
    ProcessState state = CREATED;

    Process(int AT, int TC, int CB, int IO, int PID)
            :
            AT(AT),
            TC(TC),
            CB(CB),
            IO(IO),
            PID(PID),
            static_priority(myrandom(maxprio)),
            remaining_time(TC),
            ready_stamp(AT) { dynamic_priority = static_priority; }

};


typedef map<int, list<Process *>> mlqueue;


class Scheduler {
public:
    list<Process *> run_queue;

    virtual void add_process(Process *process) {}

    virtual Process *peek_next_process() {
        return run_queue.front();
    }

    virtual Process *get_next_process() {
        auto *process = run_queue.front();
        if (!run_queue.empty())
            run_queue.pop_front();
        return process;
    }

    virtual void test_preempt() {}

    virtual string get_name() const {}
};


class FCFS : public Scheduler {
public:
    void add_process(Process *process) override {
        run_queue.push_back(process);
    }

    void test_preempt() {}

    string get_name() const override { return "FCFS"; }

};


class LCFS : public Scheduler {
public:
    void add_process(Process *process) final {
        run_queue.push_front(process);
    }

    void test_preempt() {}

    string get_name() const override { return "LCFS"; }
};


class SRTF : public Scheduler {
public:
    void add_process(Process *process) final {
        list<Process *>::iterator it;
        for (it = run_queue.begin(); it != run_queue.end(); it++) {
            if (process->remaining_time < (*it)->remaining_time) {
                run_queue.insert(it, process);
                return;
            }
        }

        run_queue.push_back(process);
    }

    void test_preempt() {}

    string get_name() const final { return "SRTF"; }
};


class RR : public FCFS {
public:
    explicit RR(int quant_arg) { quantum = quant_arg; }

    void test_preempt() {}

    string get_name() const override { return "RR"; }
};


class PRIO : public Scheduler {
    mlqueue expired_queue, run_queue;

    static void insert_into_mlqueue(mlqueue &queue, Process *process) {
        if (queue.find(process->dynamic_priority) == queue.end()) {
            list<Process *> prio_queue;
            queue[process->dynamic_priority] = prio_queue;
        }
        queue[process->dynamic_priority].push_back(process);
    }

    Process *peek_mlqueue() {
        if (run_queue.empty())
            return nullptr;

        return run_queue.rbegin()->second.front();
    }

    Process *pop_mlqueue() {
        /*
         * Pick the first process with the highest priority and pop it out.
         */
        Process *next_proc = nullptr;
        for (auto p_level = run_queue.rbegin(); p_level != run_queue.rend(); ++p_level) {
            list<Process *> p_queue = p_level->second;
            next_proc = p_queue.front();
            p_queue.pop_front();
            run_queue[p_level->first] = p_queue;
            if (p_queue.empty())
                run_queue.erase(p_level->first);
            break;
        }

        return next_proc;
    }

public:
    explicit PRIO(int quant_arg) {
        quantum = quant_arg;
    }

    void swap_queues() {
        mlqueue temp;
        temp = run_queue;
        run_queue = expired_queue;
        expired_queue = temp;
    }

    void add_process(Process *process) override {
        process->prioritised = true;

        // Process being added to queue, decrement dynamic priority.
        process->dynamic_priority--;

        // Insert either into expired or MLQ.
        if (process->dynamic_priority < 0) {
            process->dynamic_priority = process->static_priority - 1;
            insert_into_mlqueue(expired_queue, process);
        } else {
            insert_into_mlqueue(run_queue, process);
        }
    }

    Process *peek_next_process() override {
        return peek_mlqueue();
    }

    Process *get_next_process() override {
        Process *next_proc = pop_mlqueue();

        if (next_proc == nullptr) {
            swap_queues();
            next_proc = pop_mlqueue();
        }

        return next_proc;
    }

    void test_preempt() {}

    string get_name() const override { return "PRIO"; }
};


class PREPRIO : public PRIO {
public:
    explicit PREPRIO(int quant_arg) : PRIO(quant_arg) {}

    void add_process(Process *process) final {
        process->prioritised = true;
        process->can_preempt = true;
        PRIO::add_process(process);
    }

    void test_preempt() {}

    string get_name() const override { return "PREPRIO"; }
};


class Event {
    int timestamp;
    Process *process;
    ProcessState old_state, new_state;
public:
    Event(int timestamp, Process *process, ProcessState old_state, ProcessState new_state)
            :
            timestamp(timestamp),
            process(process),
            old_state(old_state),
            new_state(new_state) {}

    int get_time() const {
        return timestamp;
    }

    void set_time(int time) {
        timestamp = time;
    }

    void set_transition(ProcessState from, ProcessState to) {
        old_state = from;
        new_state = to;
    }

    Process *get_process() {
        return process;
    }

    ProcessState *get_transition_states() {
        auto *states = new ProcessState[2];
        states[0] = old_state;
        states[1] = new_state;
        return states;
    }
};


typedef list<Event *> eventQueue_t;


class DiscreteEventSimulator {
    eventQueue_t event_queue;
    list<Process> finished_processes;

    void print_metrics(const Scheduler *scheduler, int cpu_runtime, int io_time) {
        string print_quant = quantum == INT_MAX ? "" : to_string(quantum);
        cout << scheduler->get_name() << " " << print_quant << endl;
        double total_turnaround = 0, total_cpu_wait = 0, proc_count = 0;

        // Process specific metrics
        for (auto process: finished_processes) {
            int turnaround = process.finishing_time - process.AT;
            printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
                   process.PID, process.AT, process.TC, process.CB,
                   process.IO, process.static_priority, process.finishing_time,
                   turnaround, process.io_time, process.wait_time);

            total_turnaround += turnaround;
            total_cpu_wait += process.wait_time;
            proc_count++;
        }

        // Find the process that ends last.
        Process final_process = finished_processes.front();
        for (auto process: finished_processes) {
            if (process.finishing_time > final_process.finishing_time)
                final_process = process;
        }

        // Summary
        double cpu_utilizetion = 100.0 * (double) cpu_runtime / (double) final_process.finishing_time,
                io_utilization = 100.0 * (double) io_time / (double) final_process.finishing_time,
                avg_turnaround = total_turnaround / proc_count, avg_cpu_wait = total_cpu_wait / proc_count,
                throughput = 100.0 * proc_count / (double) final_process.finishing_time;

        printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", final_process.finishing_time, cpu_utilizetion, io_utilization,
               avg_turnaround, avg_cpu_wait, throughput);
    }

    void update_ready_to_run_events(int time) {
        /*
         * Update ready to run events to trigger at a given time.
         */
        for (auto event: event_queue) {
            if (event->get_transition_states()[1] == RUNNING)
                event->set_time(time);
        }
    }

    Event *peek_next_event() {
        Event *earliest_event = event_queue.front();
        map<ProcessState, int> state_order{{READY,      0},
                                           {BLOCKED,    1},
                                           {TERMINATED, 1},
                                           {RUNNING,    2}};
        for (auto event: event_queue) {
            if (event->get_time() < earliest_event->get_time())
                earliest_event = event;

                // Event order needs to consider state transition and priority as well.
            else if (event->get_time() == earliest_event->get_time()) {
                ProcessState earliest_state = earliest_event->get_transition_states()[1], event_state = event->get_transition_states()[1];
                Process *earliest_process = earliest_event->get_process(), *event_process = event->get_process();

                if (state_order[event_state] < state_order[earliest_state] ||
                    (earliest_state == RUNNING && event_state == RUNNING && event_process->can_preempt &&
                     earliest_process->dynamic_priority < event_process->dynamic_priority))
                    earliest_event = event;
            }
        }

        return earliest_event;
    }

    void schedule_next_ready_process(Scheduler *scheduler, int curr_time) {
        /*
         * Schedule next ready process.
         */
        Process *next_running_process = scheduler->get_next_process();
        if (next_running_process == nullptr)
            return;
        auto *next_run_event = new Event(curr_time, next_running_process, READY, RUNNING);
        put_event(next_run_event);
    }

    Event *get_current_run_end_event() {
        Event *running_event = nullptr;
        for (auto event: event_queue) {
            if (event->get_transition_states()[0] == RUNNING)
                running_event = event;
        }
        return running_event;
    }

    bool is_run_queued() {
        bool is_run_queued = false;

        for(auto event : event_queue){
            if (event->get_transition_states()[1] == RUNNING){
                is_run_queued = true;
                break;
            }
        }

        return is_run_queued;
    }

public:
    void run_simulation(Scheduler *scheduler) {
        /*
         * Orchestrate the events.
         */
        int curr_time = 0, cpu_runtime = 0, io_end = -1, io_time = 0;
        while (!event_queue.empty()) {
            Event curr_event = get_event();
            curr_time = curr_event.get_time();
            Process *curr_process = curr_event.get_process();
            ProcessState *transition_states = curr_event.get_transition_states();
            curr_process->state = transition_states[1];

            switch (curr_process->state) {
                case READY: {
                    // Update.
                    curr_process->ready_stamp = curr_time;

                    if (VERBOSE)
                        printf("%d %d: %s -> READY prio=%d\n", curr_time, curr_process->PID,
                               state_string[transition_states[0]],
                               curr_process->dynamic_priority);

                    // Queue process to trigger scheduling.
                    scheduler->add_process(curr_process);

                    // Do not schedule next running event until all transitions to ready at current timestamp are done.
                    Event *next_event = peek_next_event();
                    if (next_event != nullptr && next_event->get_transition_states()[1] == READY &&
                        next_event->get_time() == curr_time)
                        continue;

                    // Schedule next process to run only if it's higher priority or if nothing's running.
                    Process *next_process_in_line = scheduler->peek_next_process();
                    Event *run_end_event = get_current_run_end_event();
                    bool queue_process = false;

                    // Only check
                    if (next_process_in_line != nullptr && run_end_event && transition_states[0] != READY) {
                        // Check if we can preempt.
                        Process *running_process = run_end_event->get_process();

                        if (VERBOSE)
                            printf("---> PRIO preemption %d by %d ? TS=%d now=%d)\n", running_process->PID,
                                   curr_process->PID, run_end_event->get_time(), curr_time);

                        if (next_process_in_line->can_preempt &&
                            running_process->dynamic_priority <
                            next_process_in_line->dynamic_priority && run_end_event->get_time() > curr_time) {
                            // Preempt the current running process if higher priority proc waiting.
                            run_end_event->set_transition(RUNNING, READY);

                            // Since the running process is being preempted, we need to update processing time.
                            int time_adjustment = run_end_event->get_time() - curr_time;
                            running_process->remaining_time += time_adjustment;
                            running_process->burst_left += time_adjustment;
                            cpu_runtime -= time_adjustment;

                            run_end_event->set_time(curr_time);

                            // Scheduler the higher priority process to run.
                            schedule_next_ready_process(scheduler, curr_time);
                        }
                    }

                    // Call scheduler if there's no more running processes or event triggers.
                    else if (run_end_event == nullptr && !is_run_queued())
                        schedule_next_ready_process(scheduler, curr_time);

                    break;
                }

                case RUNNING: {
                    // Check if there's residual CPU burst_left time from prior preemption.
                    int cpu_burst;
                    if (curr_process->burst_left == 0) {
                        cpu_burst = myrandom(curr_process->CB);
                        curr_process->burst_left = cpu_burst;
                    } else
                        cpu_burst = curr_process->burst_left;

                    int burst_candidates[3] = {cpu_burst, curr_process->remaining_time, quantum};

                    // Determine the shortest time period between the burst_left, quantum and remaining time.
                    int burst_time = *min_element(begin(burst_candidates), end(burst_candidates));
                    curr_process->burst_left -= burst_time;
                    int burst_end_time = curr_time + burst_time;

                    if (VERBOSE)
                        printf("%d %d: READY -> RUNNG cb=%d rem=%d prio=%d\n", curr_time, curr_process->PID, cpu_burst,
                               curr_process->remaining_time, curr_process->dynamic_priority);

                    // Update process time metrics
                    curr_process->remaining_time -= burst_time;
                    curr_process->wait_time += curr_time - curr_process->ready_stamp;

                    // Create a new event for this process only if the process doesn't end at the end of this one.
                    Event *new_event;
                    if (curr_process->remaining_time > 0) {
                        if (burst_time == cpu_burst)
                            new_event = new Event(burst_end_time, curr_process, RUNNING, BLOCKED);
                        else
                            new_event = new Event(burst_end_time, curr_process, RUNNING, READY);
                    }

                        // Enqueue process termination event.
                    else new_event = new Event(burst_end_time, curr_process, RUNNING, TERMINATED);

                    put_event(new_event);

                    // Update ready->running transition events
                    update_ready_to_run_events(burst_end_time);

                    cpu_runtime += burst_time;
                    break;
                }

                case BLOCKED: {

                    int burst_time = myrandom(curr_process->IO);
                    int curr_end = curr_time + burst_time;

                    // Calculate last io_burst time metrics
                    if (curr_time >= io_end) {
                        io_time += burst_time;
                    } else if (curr_end > io_end) {
                        io_time += curr_end - io_end;
                    }

                    // Update IO period.
                    io_end = curr_end > io_end ? curr_end : io_end;

                    if (VERBOSE)
                        printf("%d %d: RUNNG -> BLOCK ib=%d rem=%d\n", curr_time, curr_process->PID, burst_time,
                               curr_process->remaining_time);

                    // Reset dynamic priority
                    curr_process->dynamic_priority = curr_process->static_priority;

                    curr_process->io_time += burst_time;
                    auto *new_event = new Event(curr_end, curr_process, BLOCKED, READY);
                    put_event(new_event);

                    if (!is_run_queued())
                        schedule_next_ready_process(scheduler, curr_time);
                    break;
                }

                case TERMINATED: {
                    curr_process->finishing_time = curr_time;
                    list<Process>::iterator proc_it;
                    for (proc_it = finished_processes.begin(); proc_it != finished_processes.end(); proc_it++) {
                        if ((*proc_it).PID > curr_process->PID)
                            break;
                    }
                    if (proc_it == finished_processes.end())
                        finished_processes.push_back(*curr_process);
                    else
                        finished_processes.insert(proc_it, *curr_process);

                    if (VERBOSE)
                        printf("%d %d Done\n", curr_process->finishing_time, curr_process->PID);

                    if(!is_run_queued())
                        schedule_next_ready_process(scheduler, curr_time);
                }
            }
        }

        // Print out the final summary.
        print_metrics(scheduler, cpu_runtime, io_time);
    }

    void put_event(Event *event) {
        event_queue.push_back(event);
    }

    Event get_event() {
        /*
         * Get the earliest event in queue and pop it from the list.
         */
        Event *earliest_event = peek_next_event();
        event_queue.remove(earliest_event);
        return *earliest_event;
    }

    eventQueue_t get_event_queue() {
        return event_queue;
    }
};


int parseArgs(int argc, char **argv) {
    int c, quant_arg;
    // Disable getopt error output
    opterr = 0;

    while ((c = getopt(argc, argv, "dvs:")) != -1)
        switch (c) {
            case 's':
                sscanf(optarg, "%c%d:%d", &scheduler_code, &quant_arg, &maxprio);
                break;
            case 'd':
                DEBUG = true;
                break;
            case 'v':
                VERBOSE = true;
                break;
            case '?':
                if (optopt == 's')
                    fprintf(stderr, "Option -s is mandatory. Please provide the necesssary arguments.");
                else
                    fprintf(stderr, "Unknown option: '%d'", optopt);
                exit(1);
            default:
                exit(1);
        }
    if (DEBUG) {
        cout << "Scheduler: " << scheduler_code << ", Quantum: " << quantum << ", Max Priority: " << maxprio << endl;
        cout << "Non-option args: \n";
        for (int i = optind; i < argc; i++)
            cout << argv[i] << endl;
    }

    INPUT_FILE_PATH = argv[optind];
    RAND_FILE_PATH = argv[++optind];

    return quant_arg;
}


DiscreteEventSimulator initializeSimulator() {
    DiscreteEventSimulator simulator;
    ifstream input_file(INPUT_FILE_PATH);
    int AT, TC, CB, IO, PID = 0;

    if (!input_file) {
        cout << "File at " << INPUT_FILE_PATH << " not found." << endl;
        exit(1);
    }

    while (input_file >> AT >> TC >> CB >> IO) {
        auto *process = new Process(AT, TC, CB, IO, PID);
        auto *event = new Event(AT, process, CREATED, READY);
        simulator.put_event(event);
        PID++;
    }

    return simulator;
}


void printEvents(DiscreteEventSimulator simulator) {
    eventQueue_t queue = simulator.get_event_queue();
    cout << "Printing events..." << endl;
    for (auto event: queue) {
        ProcessState *states = event->get_transition_states();
        Process process = *event->get_process();
        int at = process.AT, tc = process.TC, cb = process.CB, io = process.IO, pid = process.PID;
        cout << "Time: " << event->get_time() << endl;
        printf("Process: AT: %d, TC: %d, CB: %d, IO: %d, PID: %d\n", at, tc, cb, io, pid);
        cout << "Transition: " << state_string[states[0]] << "->" << state_string[states[1]] << endl
             << "###########################\n";
    }
}


int main(int argc, char **argv) {
    int quant_arg = parseArgs(argc, argv);

    // Initialize the simulator
    initRandNums();
    DiscreteEventSimulator simulator = initializeSimulator();

    if (DEBUG) printEvents(simulator);

    Scheduler *scheduler;
    switch (scheduler_code) {
        case 'F':
            scheduler = new FCFS();
            break;
        case 'L':
            scheduler = new LCFS();
            break;
        case 'S':
            scheduler = new SRTF();
            break;
        case 'R':
            scheduler = new RR(quant_arg);
            break;
        case 'P':
            scheduler = new PRIO(quant_arg);
            break;
        case 'E':
            scheduler = new PREPRIO(quant_arg);
            break;
        default:
            cout << "Invalid scheduler code: " << scheduler_code;
            exit(1);
    }

    simulator.run_simulation(scheduler);
}