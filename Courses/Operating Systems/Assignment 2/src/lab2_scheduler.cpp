#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>

using namespace std;


// Globals
bool DEBUG = false;
char scheduler;
int ofs = 0, quantum = INT_MAX, maxprio = 4;
vector < int > randvals;
string RAND_FILE_PATH;
enum ProcessState { CREATED, READY, RUNNING, BLOCKED } ;
enum StateTransition { TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT} ;


//void initRandNums(){
//    ifstream randFile()
//}
//
//
//int myrandom(int burst) {
//    ofs++;
//    return 1 + (randvals[(ofs-1)%randvals.size()] % burst);
//}
//
//
//class Scheduler {
//private:
//    int quantum;
//public:
//    virtual add_process() {
//
//    }
//
//    virtual get_next_process() {
//
//    }
//
//    virtual test_preempt(){
//
//    }
//};
//
//
//class Process {
//private:
//    int AT, TC, CB, IO, PID, static_priority;
//public:
//    Process(int AT, int TC, int CB, int IO, int PID)
//            :
//            AT(AT),
//            TC(TC),
//            CB(CB),
//            IO(IO),
//            PID(PID) {
//
//
//    }
//
//};


void parseArgs(int argc, char **argv){
    int c = 0;
    // Disable getopt error output
    opterr = 0;

    while ((c = getopt (argc, argv, "vs:")) != -1)
        switch (c)
        {
            case 's':
                sscanf(optarg, "%c%d:%d", &scheduler, &quantum, &maxprio);
                break;
            case 'v':
                DEBUG = true;
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
        cout << "Scheduler:" << scheduler << ", Quantum: " << quantum << ", Max Priority: " << maxprio << endl;
        cout << "Non-option args: \n";
        for (int i = optind; i < argc; i++)
            cout << argv[i] << endl;
    }
}


int main (int argc, char **argv) {
    parseArgs(argc, argv);
}