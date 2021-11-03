#include <iostream>
#include <string>
#include <list>
#include <iterator>
#include <vector>
#include <unistd.h>
#include <fstream>

using namespace std;


// Globals
bool DEBUG = false, VERBOSE = false;
char scheduler;
int ofs = 0, quantum = INT_MAX, maxprio = 4;
vector < int > rand_vals;
string INPUT_FILE_PATH, RAND_FILE_PATH;
enum ProcessState { CREATED, READY, RUNNING, BLOCKED } ;
static const char *state_string [] = {"CREATED", "READY", "RUNNG", "BLOCK"};
enum StateTransition { TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT} ;


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


class Scheduler {
private:
    int quantum;
public:
    virtual add_process() {}

    virtual get_next_process() {}

    virtual test_preempt(){}
};


class Process {
public:
    int AT, TC, CB, IO, PID, static_priority;
    Process(int AT, int TC, int CB, int IO, int PID)
            :
            AT(AT),
            TC(TC),
            CB(CB),
            IO(IO),
            PID(PID),
            static_priority(myrandom(maxprio)){}

};


class Event{
    int timestamp;
    Process &process;
    ProcessState old_state, new_state;
public:
    Event(int timestamp, Process& process, ProcessState old_state, ProcessState new_state)
    :
    timestamp(timestamp),
    process(process),
    old_state(old_state),
    new_state(new_state) {}

    int get_time(){
        return timestamp;
    }

    Process get_process(){
        return process;
    }

    string* get_states(){
        string* states = new string[2];
        states[0] = state_string[old_state];
        states[1] = state_string[new_state];
        return states;
    }
};

typedef list <Event> eventQueue_t;


class DiscreteEventSimulator{
    eventQueue_t event_queue;
public:
    void put_event(Event event){
        /*
         * Inserts an event to event queue in a time-sorted order.
         */
        // Simply insert into empty list.

        int event_time = event.get_time();
        list<Event>::iterator it;

        // Find the position in the event queue to insert at.
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


//void Simulation() {
//    Event* evt;
//    while( (evt = get_event()) ) {
//        Process *proc = evt->evtProcess; // this is the process the event works on
//        CURRENT_TIME = evt->evtTimeStamp;
//        timeInPrevState = CURRENT_TIME – proc->state_ts;
//        switch(evt->transition) { // which state to transition to?
//            case TRANS_TO_READY:
//                // must come from BLOCKED or from PREEMPTION
//                // must add to run queue
//                CALL_SCHEDULER = true; // conditional on whether something is run
//                break;
//            case TRANS_TO_RUN:
//                // create event for either preemption or blocking
//                break;
//            case TRANS_TO_BLOCK:
//                //create an event for when process becomes READY again
//                CALL_SCHEDULER = true;
//                break;
//            case TRANS_TO_PREEMPT:
//                // add to runqueue (no event is generated)
//                CALL_SCHEDULER = true;
//                break;
//        }
//        // remove current event object from Memory
//        delete evt; evt = nullptr;
//        if(CALL_SCHEDULER) {
//            if (get_next_event_time() == CURRENT_TIME)
//                continue; //process next event from Event queue
//            CALL_SCHEDULER = false; // reset global flag
//            if (CURRENT_RUNNING_PROCESS == nullptr) {
//                CURRENT_RUNNING_PROCESS = THE_SCHEDULER->get_next_process();
//                if (CURRENT_RUNNING_PROCESS == nullptr)
//                    continue;
//                // create event to make this process runnable for same time.
//            }


void parseArgs(int argc, char **argv){
    int c;
    // Disable getopt error output
    opterr = 0;

    while ((c = getopt (argc, argv, "dvs:")) != -1)
        switch (c)
        {
            case 's':
                sscanf(optarg, "%c%d:%d", &scheduler, &quantum, &maxprio);
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
        cout << "Scheduler: " << scheduler << ", Quantum: " << quantum << ", Max Priority: " << maxprio << endl;
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
        Event event(AT, *process, CREATED, READY);
        simulator.put_event(event);
        PID++;
    }

    return simulator;
}

void printEvents(DiscreteEventSimulator simulator){
    eventQueue_t queue = simulator.get_event_queue();
    cout << "Printing events..." << endl;
    for(auto event : queue){
        string* states = event.get_states();
        Process process = event.get_process();
        int at = process.AT, tc = process.TC, cb = process.CB, io = process.IO, pid = process.PID;
        cout << "Time: " << event.get_time() << endl;
        printf("Process: AT: %d, TC: %d, CB: %d, IO: %d, PID: %d\n", at, tc, cb, io, pid);
        cout << "Transition: " << states[0] << "->" << states[1] << endl << "###########################\n";
    }
}


int main (int argc, char **argv) {
    parseArgs(argc, argv);

    // Initialize the simulator
    initRandNums();
    DiscreteEventSimulator simulator = initializeSimulator();

    if(DEBUG) printEvents(simulator);



}