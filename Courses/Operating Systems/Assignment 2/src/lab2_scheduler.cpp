#include <iostream>
#include <list>
#include <iterator>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <iomanip>

using namespace std;


// Globals
bool DEBUG = false, VERBOSE = false;
char scheduler_code;
int ofs = 0, quantum = INT_MAX, maxprio = 4;
vector < int > rand_vals;
string INPUT_FILE_PATH, RAND_FILE_PATH;
enum ProcessState { CREATED, READY, RUNNING, BLOCKED } ;
static const char *state_string [] = {"CREATED", "READY", "RUNNG", "BLOCK"};
enum StateTransition { TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT};


void initRandNums(){
    ifstream rand_file(RAND_FILE_PATH);
    int num, n;
    rand_file >> n;
    for(int i=0; i<n; i++){
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
    int AT, TC, CB, IO, PID, static_priority, remaining_time, finishing_time, io_time = 0, wait_time = 0, ready_stamp;
    ProcessState state = CREATED;
    Process(int AT, int TC, int CB, int IO, int PID)
            :
            AT(AT),
            TC(TC),
            CB(CB),
            IO(IO),
            PID(PID),
            static_priority(myrandom(maxprio)),
            remaining_time(TC){}

};


class Scheduler {
public:
    list <Process*> runqueue;
    virtual void add_process(Process* process) {}

    Process* get_next_process() {
        auto* process = runqueue.front();
        runqueue.pop_front();
        return process;
    }

    virtual void test_preempt(){}

    virtual string get_name() const {}
};


class FCFS: public Scheduler{
public:
    void add_process(Process* process) override{
        runqueue.push_back(process);
    }

    void test_preempt(){}

    string get_name() const override {return "FCFS";}

};


class LCFS: public Scheduler{
public:
    void add_process(Process* process){
        runqueue.push_back(process);
    }

    void test_preempt(){}

    string get_name() const override {return "LCFS";}
};


class SRTF: public Scheduler{
public:
    void add_process(Process* process){
        runqueue.push_back(process);
    }

    void test_preempt(){}

    string get_name() const override {return "SRTF";}
};


class RR: public Scheduler{
public:
    void add_process(Process* process){
        runqueue.push_back(process);
    }

    void test_preempt(){}

    string get_name() const override {return "RR";}
};


class PRIO: public Scheduler{
public:
    void add_process(Process* process){
        runqueue.push_back(process);
    }

    void test_preempt(){}

    string get_name() const override {return "PRIO";}
};


class PREPRIO: public Scheduler{
public:
    void add_process(Process* process){
        runqueue.push_back(process);
    }

    void test_preempt(){}

    string get_name() const override {return "PREPRIO";}
};


class Event{
    int timestamp;
    Process* process;
    ProcessState old_state, new_state;
public:
    Event(int timestamp, Process* process, ProcessState old_state, ProcessState new_state)
    :
    timestamp(timestamp),
    process(process),
    old_state(old_state),
    new_state(new_state) {}

    int get_time(){
        return timestamp;
    }

    Process* get_process(){
        return process;
    }

    ProcessState* get_transition_states(){
        auto* states = new ProcessState[2];
        states[0] = old_state;
        states[1] = new_state;
        return states;
    }
};

typedef list <Event> eventQueue_t;


class DiscreteEventSimulator{
    eventQueue_t event_queue;
    list <Process> finished_processes;

    void print_metrics(const Scheduler* scheduler){
        cout << scheduler->get_name() << endl;
        for(auto process: finished_processes){
            printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
                   process.PID, process.AT,process.TC, process.CB,
                   process.IO, process.static_priority,process.finishing_time,
                   process.finishing_time - process.AT,
                   process.static_priority, process.wait_time);
        }
    }

    bool process_queued_for_running(Process* process){
        for(auto event: event_queue){
            if(event.get_process() == process && event.get_transition_states()[1] == RUNNING){
                return true;
            }
        }

        return false;
    }

public:
    void run_simulation(Scheduler* scheduler){
        /*
         * Orchestrate the events.
         */
        int curr_time = 0;
        while(!event_queue.empty()){
            Event curr_event = get_event();
            curr_time = curr_event.get_time();
            Process* curr_process = curr_event.get_process();
            ProcessState* transition_states = curr_event.get_transition_states();
            curr_process->state = transition_states[1];

            switch(curr_process->state){
                case READY:{
                    // Queue process to trigger scheduling.
                    curr_process->ready_stamp = curr_time;
                    scheduler->add_process(curr_process);

                    // Check if next running process is queued in events and create a run event if not.
                    Process* next_running_process = scheduler->get_next_process();
                    if(!process_queued_for_running(next_running_process)){
                        Event next_run_event(curr_time, next_running_process, READY, RUNNING);
                        put_event(next_run_event);
                    }
                    break;
                }

                case RUNNING:{
                    int cpu_burst = myrandom(curr_process->CB);
                    int burst_ends[3] = {cpu_burst, curr_process->remaining_time, quantum};

                    // Determine out the shortest time period between the burst, quantum and remaining time.
                    int burst_time = *min_element(begin(burst_ends), end(burst_ends));

                    // Update process time metrics
                    curr_process->remaining_time -= burst_time;
                    curr_process->wait_time += curr_time - curr_process->ready_stamp;

                    // Create a new event for this process only if the process doesn't end at the end of this one.
                    if(curr_process->remaining_time > 0){
                        Event* new_event;
                        if(burst_time == curr_process->CB)
                            new_event = new Event(curr_time + burst_time, curr_process, RUNNING, BLOCKED);
                        else
                            new_event = new Event(curr_time + burst_time, curr_process, RUNNING, READY);

                        put_event(*new_event);
                    }
                    // Enqueue finished processes.
                    else{
                        curr_process->finishing_time = curr_time + burst_time;
                        finished_processes.push_back(*curr_process);
                    }
                    break;
                }

                case BLOCKED:{
                    int burst_time = myrandom(curr_process->IO);
                    curr_process->io_time+=burst_time;
                    Event new_event(curr_time + burst_time, curr_process, BLOCKED, READY);
                    put_event(new_event);
                    break;
                }
            }
        }

        // Print out the final summary.
        print_metrics(scheduler);
    }

    void put_event(Event event){
        /*
         * Inserts an event to event queue in a time-sorted order.
         */
        int event_time = event.get_time();
        list<Event>::iterator it;

        // Find the time-sorted position in the event queue to insert at.
        for(it = event_queue.begin(); it != event_queue.end(); it++){
            if(event_time < (*it).get_time()) break;
        }

        event_queue.insert(it, event);
    }

    Event get_event(){
        /*
         * Get the first event in queue and pop it from the list.
         */
        Event event = event_queue.front();
        event_queue.pop_front();
        return event;
    }

    eventQueue_t get_event_queue(){
        return event_queue;
    }
};


void parseArgs(int argc, char **argv){
    int c;
    // Disable getopt error output
    opterr = 0;

    while ((c = getopt (argc, argv, "dvs:")) != -1)
        switch (c)
        {
            case 's':
                sscanf(optarg, "%c%d:%d", &scheduler_code, &quantum, &maxprio);
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
                    fprintf (stderr, "Unknown option: '%d'", optopt);
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
}


DiscreteEventSimulator initializeSimulator(){
    DiscreteEventSimulator simulator;
    ifstream input_file(INPUT_FILE_PATH);
    int AT, TC, CB, IO, PID = 0;

    if (!input_file){
        cout << "File at " << INPUT_FILE_PATH <<" not found." << endl;
        exit(1);
    }

    while(input_file >> AT >> TC >> CB >> IO){
        auto* process = new Process(AT, TC, CB, IO, PID);
        Event event(AT, process, CREATED, READY);
        simulator.put_event(event);
        PID++;
    }

    return simulator;
}

void printEvents(DiscreteEventSimulator simulator){
    eventQueue_t queue = simulator.get_event_queue();
    cout << "Printing events..." << endl;
    for(auto event : queue){
        ProcessState* states = event.get_transition_states();
        Process process = *event.get_process();
        int at = process.AT, tc = process.TC, cb = process.CB, io = process.IO, pid = process.PID;
        cout << "Time: " << event.get_time() << endl;
        printf("Process: AT: %d, TC: %d, CB: %d, IO: %d, PID: %d\n", at, tc, cb, io, pid);
        cout << "Transition: " << state_string[states[0]] << "->" << state_string[states[1]] << endl << "###########################\n";
    }
}


int main (int argc, char **argv) {
    parseArgs(argc, argv);

    // Initialize the simulator
    initRandNums();
    DiscreteEventSimulator simulator = initializeSimulator();

    if(DEBUG) printEvents(simulator);

    Scheduler* scheduler;
    switch(scheduler_code){
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
            scheduler = new RR();
            break;
        case 'P':
            scheduler = new PRIO();
            break;
        case 'E':
            scheduler = new PREPRIO();
            break;
        default:
            cout << "Invalid scheduler code: " << scheduler_code;
            exit(1);
    }

    simulator.run_simulation(scheduler);
}