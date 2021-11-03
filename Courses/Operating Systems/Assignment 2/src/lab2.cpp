#include <iostream>
#include <stdio.h>
#include <queue>
#include <string>
#include <vector>
#include <stdlib.h>    
#include <time.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <list>

using namespace std;

static string FILE_PATH, R_PATH;

typedef enum { STATE_CREATED, STATE_READY, STATE_RUNNING, STATE_BLOCKED } process_state_t;
typedef enum { TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT} state_transition;

vector<int> randvals; //for generating random values

int ofs = 0, CURRENT_TIME, PREV_TIME, TOTAL_TIME, IO_TIME = 0; // some global variables used to store important parameters
bool CALL_SCHEDULER = false; //
static bool DEBUG_FLAG = false;

unordered_set<int> ioStates;
char sched;
int qnum = INT_MAX, maxprio = 4;



static string state_to_string(process_state_t state){
  if(state==STATE_CREATED)
    return "CREATED";
  if(state==STATE_READY)
    return "READY";
  if(state==STATE_RUNNING)
    return "RUNNG";
  if(state==STATE_BLOCKED)
    return "BLOCK";
  return "UNIDENTIFIED STATE";
}

int myrandom(int burst) {
  ofs++;
  // cout<<"rand called: "<<1 + (randvals[(ofs-1)%randvals.size()] % burst)<<"\n";
  return 1 + (randvals[(ofs-1)%randvals.size()] % burst); 
}

class Process { 
  int AT, TC, CB, IO, PID;
  int dynamicCB, remainingTime, dynamicIO, staticPriority, dynamicPriority, lastUpdateTime, FT, IT, CW;
  process_state_t state;
  public:  

  Process(int AT, int TC, int CB, int IO, int PID, int maxprio = 4)
  : AT(AT),
    TC(TC),
    CB(CB),
    IO(IO),
    PID(PID) {
      remainingTime = TC;
      IT = 0;
      CW = 0;
      dynamicCB = 0;
      dynamicIO = 0;
      staticPriority = myrandom(maxprio);
      dynamicPriority = staticPriority-1;
      lastUpdateTime = AT;
      state = STATE_CREATED;
    }

  int getDynamicCB(){
    return dynamicCB;
  }

  int getDynamicIO(){
    return dynamicIO;
  }

  process_state_t getState(){
    return state;
  }

  void updateDynamicPrio(){
    dynamicPriority = dynamicPriority-1;
  }

  void resetDynamicPrio(){
    dynamicPriority = staticPriority-1;
  }

  void updateProcessState(process_state_t state, int time){
    if(this->state == STATE_RUNNING){
      updateEventRemainingTime(time);
    }
    else if(this->state == STATE_BLOCKED){
      IT += time-lastUpdateTime;
    }else{
      CW += time - lastUpdateTime;
    }

    if(state==STATE_RUNNING && dynamicCB==0){ //todo: maybe check for edge cases here
      dynamicCB = min(myrandom(CB), remainingTime);
    }else if(state==STATE_BLOCKED && remainingTime!=0){
      dynamicIO = myrandom(IO);
    }


    if(DEBUG_FLAG){
      if(remainingTime==0) cout<<time<<" "<<PID<<" "<<time-lastUpdateTime<<": Done";
      else cout<<time<<" "<<PID<<" "<<time-lastUpdateTime
        <<": "<<state_to_string(this->state)<<" -> "<<state_to_string(state);
      if(state==STATE_RUNNING){
        cout<<" cb="<<dynamicCB<<" rem="<<remainingTime<<" prio="<<dynamicPriority;
      }else if (state==STATE_READY && this->state==STATE_RUNNING){
        cout<<"  cb="<<dynamicCB<<" rem="<<remainingTime<<" prio="<<dynamicPriority;
      }else if(state==STATE_BLOCKED && remainingTime!=0){
        cout<<"  ib="<<dynamicIO<<" rem="<<remainingTime;
      }
      cout<<"\n";
    }
    if(state==STATE_READY && this->state==STATE_RUNNING){
      updateDynamicPrio();
    }

    this->state = state;
    lastUpdateTime = time;
  }

  void updateEventRemainingTime(int time){
    remainingTime -= (time-lastUpdateTime);
    dynamicCB-= (time-lastUpdateTime);
    if(remainingTime==0){
      FT = time;
    }
  }
  int getProcessStateDuration(int time){return time-lastUpdateTime;}
  int getPID(){ return PID;}
  int getAT(){ return AT;}
  int getTC(){ return TC;}
  int getCB(){ return CB;}
  int getIO(){ return IO;}
  int getFT(){ return FT;}
  int getTT(){ return FT-AT;}
  int getIT(){ return IT;}
  int getCW(){ return CW;}
  int getStaticPriority(){ return staticPriority;}
  int getDynamicPriority(){ return dynamicPriority;}
  int getRemainingTime(){return remainingTime;}
  bool complete(){return remainingTime==0;}
};

Process * CURRENT_RUNNING_PROCESS = nullptr;
static vector<Process*> processes;


class Event{
  private:

  int creationTime, occourenceTime;
  Process * process;
  process_state_t oldState, newState;

  public:
  Event(int creationTime, int occourenceTime, Process * process, process_state_t oldState, process_state_t newState) 
  : creationTime(creationTime), 
    occourenceTime(occourenceTime), 
    process(process), oldState(oldState), 
    newState(newState) {}


  int getCreationTime(){
    return creationTime;
  }

  state_transition getTransition(){
    if(newState==STATE_RUNNING){
      return TRANS_TO_RUN;
    }else if((oldState==STATE_BLOCKED || oldState==STATE_CREATED) && newState==STATE_READY){
      return TRANS_TO_READY;
    }else if(newState==STATE_BLOCKED){
      return TRANS_TO_BLOCK;
    }else if(oldState==STATE_RUNNING && newState==STATE_READY){
      return TRANS_TO_PREEMPT;
    }else{
      printf("error in transition state with\n");
      cout<<process->getPID()<<" "<<oldState<<" "<<newState<<"\n";
      return TRANS_TO_READY;
    }
  }

  int getOccourenceTime(){
    return occourenceTime;
  }

  Process * getProcess(){
    return process;
  }
};

class DiscreteEventSimulator {
  private:

    template<typename Event, class Container=std::vector<Event *>, class Compare=std::less<typename Container::value_type>> 
    class custom_priority_queue : public std::priority_queue<Event *, Container, Compare>
    {
      public:
        void remove(Process * process) {
          auto it = this->c.begin();
          while(it!=this->c.end()){
            if((*it)->getProcess()==process){
              this->c.erase(it);
              make_heap(this->c.begin(), this->c.end(), this->comp);
              return;
            }
            it++;
          }
        }
        int getEvenetTime(Process * process) {
          auto it = this->c.begin();
          while(it!=this->c.end()){
            if((*it)->getProcess()==process){
              return (*it)->getOccourenceTime();
            }
            it++;
          }
          return -1;
        }
    };

  struct CompareEvents{
    bool operator() (Event * e1, Event * e2){
      if(e1->getOccourenceTime()==e2->getOccourenceTime()){
        return e1->getCreationTime()>e2->getCreationTime();
      }
      return e1->getOccourenceTime()>e2->getOccourenceTime();
    }
  };

  custom_priority_queue<Event*, vector<Event*>, CompareEvents> events;

  public:

  void putEvent(Event *event){
    events.push(event);
  }

  Event * getEvent(){
    Event * event = events.top();
    events.pop();
    return event;
  }

  void removeEventWithProc(Process *process){
    events.remove(process);
  }

  int getNextEventTime(){
    if(events.empty()) return INT_MAX;
    return events.top()->getOccourenceTime();
  }
  int getNoOfEvents(){
    return events.size();
  }
  bool finished(){
    return events.empty();
  }
  int getProcessEventTime(Process *process){
    return events.getEvenetTime(process);
  }
};

DiscreteEventSimulator discreteEventSimulator;


class SchedulerInterface { 
  private:
    int timeQuantum = INT_MAX;
  public: 
    virtual void addProcess(Process * process) = 0;
    virtual Process * getNextProcess() = 0;
    virtual bool processInQueue() = 0;
    virtual bool test_preempt(Process *p, int curtime) = 0;

    int getTimeQuantum(){return timeQuantum;}
    void setTimeQuantum(int t){timeQuantum = t;}

};



class FCFS: public SchedulerInterface {
  private:
  queue<Process*> startQueue;
  public:
  void addProcess(Process * process){
    startQueue.push(process);
  }

  Process * getNextProcess(){
    Process * process = startQueue.front();
    startQueue.pop();
    return process;
  }

  bool processInQueue(){
    return !startQueue.empty();
  }
  bool test_preempt(Process *p, int curtime){return false;}
};

class LCFS: public SchedulerInterface {
  private:
  list<Process*> startQueue;
  public:
  void addProcess(Process * process){
    startQueue.push_back(process);
  }

  Process * getNextProcess(){
    Process * process = startQueue.back();
    startQueue.pop_back();
    return process;
  }

  bool processInQueue(){
    return !startQueue.empty();
  }
  bool test_preempt(Process *p, int curtime){return false;}
};

class SRTF: public SchedulerInterface {
  private:

  bool greater(Process * p1, Process * p2){
    if(p1->getRemainingTime()==p2->getRemainingTime()) return p1->getPID()>p2->getPID();
    return p1->getRemainingTime()>p2->getRemainingTime();
  }

  list<Process*> startQueue;
  public:
  void addProcess(Process * process){
    if(startQueue.size()==0){
      startQueue.push_back(process);
      return;
    }
    for(list<Process*>::iterator i = startQueue.begin(); i != startQueue.end(); i++){
      if(!greater(process, *i)){
        startQueue.insert(i, process);
        return;
      }
    }
    startQueue.push_back(process);
    
  }

  Process * getNextProcess(){
    Process * process = startQueue.front();
    startQueue.pop_front();
    return process;
  }

  bool processInQueue(){
    return !startQueue.empty();
  }
  bool test_preempt(Process *p, int curtime){return false;}
};

class PRIO: public SchedulerInterface {
  protected:
  int priority_index;
  vector<queue<Process*>> *activeQueue;
  vector<queue<Process*>> *expiredQueue;
  public:

  PRIO(){
    activeQueue = new vector<queue<Process*>>(maxprio, queue<Process*>());
    expiredQueue = new vector<queue<Process*>>(maxprio, queue<Process*>());
    priority_index = -1;
  }

  void addProcess(Process * process){
    // cout<<process->getDynamicPriority()<<" "<<priority_index<<"\n";
    if(process->getDynamicPriority()==-1){
      process->resetDynamicPrio();
      expiredQueue->at(process->getDynamicPriority()).push(process);
    }else{
      activeQueue->at(process->getDynamicPriority()).push(process);
      priority_index = max(priority_index, process->getDynamicPriority());
    }
  }

  Process * getNextProcess(){
    Process * process = activeQueue->at(priority_index).front();
    activeQueue->at(priority_index).pop();
    while(priority_index>=0 && activeQueue->at(priority_index).size()==0){
      priority_index--;
    }
    // cout<<process->getDynamicPriority()<<" "<<priority_index<<"\n";
    return process;
  }

  bool processInQueue(){
    if(priority_index==-1){
      // cout<<"inside\n";
      // cout<<"switch \n";
      vector<queue<Process*>> *temp = activeQueue;
      activeQueue = expiredQueue;
      expiredQueue = temp;
      priority_index = maxprio-1;
      while(priority_index>=0 && activeQueue->at(priority_index).size()==0){
        priority_index--;
      }
    }
    return priority_index!=-1;
  }
  bool test_preempt(Process *p, int curtime){return false;}
};

class PREPRIO: public PRIO{
  void addProcess(Process * process){

    if(process->getDynamicPriority()==-1){
      process->resetDynamicPrio();
      expiredQueue->at(process->getDynamicPriority()).push(process);
    }else{
      activeQueue->at(process->getDynamicPriority()).push(process);
      priority_index = max(priority_index, process->getDynamicPriority());

      if(CURRENT_RUNNING_PROCESS!=nullptr && CURRENT_RUNNING_PROCESS!=process && test_preempt(process, CURRENT_TIME)){
        discreteEventSimulator.removeEventWithProc(CURRENT_RUNNING_PROCESS);
        Event *premptEvent = new Event(CURRENT_TIME, CURRENT_TIME, CURRENT_RUNNING_PROCESS, STATE_RUNNING, STATE_READY);
        discreteEventSimulator.putEvent(premptEvent);
      }

    }
  }

  bool test_preempt(Process *process, int curtime){

    int t = discreteEventSimulator.getProcessEventTime(CURRENT_RUNNING_PROCESS);
    if(DEBUG_FLAG){
      printf("---> PRIO preemption %d by %d ? %d TS=%d now=%d) --> ",
        CURRENT_RUNNING_PROCESS->getPID(), process->getPID(), process->getDynamicPriority()-CURRENT_RUNNING_PROCESS->getDynamicPriority(),
        t, curtime);
    }

    if(process->getDynamicPriority()>CURRENT_RUNNING_PROCESS->getDynamicPriority()){
      if(t>curtime){
        if(DEBUG_FLAG) cout<<"YES\n";
        return true;
      }
    }
    if(DEBUG_FLAG) cout<<"NO\n";
    return false;
  }
};

void initializeRandom(){
  ifstream infile(R_PATH);
  int rand, n;
  infile >> n;
  for(int i=0; i<n; i++){
    infile>>rand;
    randvals.push_back(rand);
  }
}

void initializeSimulator(){
  ifstream infile(FILE_PATH);
  int AT, TC, CB, IO, PID = 0;
  while(infile >> AT >> TC >> CB >> IO){
    processes.push_back(new Process(AT, TC, CB, IO, PID, maxprio));
    Event * event = new Event(INT_MIN+PID, AT, processes[processes.size()-1], STATE_CREATED, STATE_READY);
    discreteEventSimulator.putEvent(event);
    PID++;
  }
}

void Simulation(SchedulerInterface *scheduler) {
  Event* event;
  int n=0;
  while(!discreteEventSimulator.finished()) {
    
    event = discreteEventSimulator.getEvent();
    Process *process = event->getProcess(); // this is the process the event works on
    PREV_TIME = CURRENT_TIME;
    CURRENT_TIME = event->getOccourenceTime();
    if(ioStates.size()!=0){
      IO_TIME+=CURRENT_TIME-PREV_TIME;
    }

    switch(event->getTransition()) { // which state to transition to?
      case TRANS_TO_READY:{
      // must come from BLOCKED or from PREEMPTION
      // must add to run queue
      // cout<<CURRENT_TIME<<"_ _"<<process->getPID()<<" "<< state_to_string(process->getState())<<"\n";
        ioStates.erase(process->getPID());
        if(process->getState()==STATE_BLOCKED){
          process->resetDynamicPrio();
        }
        process->updateProcessState(STATE_READY, CURRENT_TIME);
        
        scheduler->addProcess(process);
        CALL_SCHEDULER = true; // conditional on whether something is run
        break;
      }
      case TRANS_TO_RUN:{
      // create event for either preemption or blocking

      //todo check for blocking vs preempt
        process->updateProcessState(STATE_RUNNING, CURRENT_TIME);
        int dynamicCB = process->getDynamicCB();
        int timeQuantum = scheduler->getTimeQuantum();
        Event * stopEvent;
        if(timeQuantum<dynamicCB){
          stopEvent = new Event(CURRENT_TIME, CURRENT_TIME+timeQuantum, process, STATE_RUNNING, STATE_READY);
        }else{
          stopEvent = new Event(CURRENT_TIME, CURRENT_TIME+dynamicCB, process, STATE_RUNNING, STATE_BLOCKED);
        }
        discreteEventSimulator.putEvent(stopEvent);
        break;
      }
      case TRANS_TO_BLOCK:{
      //create an event for when process becomes READY again
        CURRENT_RUNNING_PROCESS = nullptr;
        process->updateProcessState(STATE_BLOCKED, CURRENT_TIME);
        CALL_SCHEDULER = true;

        if(!process->complete()){
          ioStates.insert(process->getPID());
          int dynamicIO = process->getDynamicIO();
          Event * readyEvent = new Event(CURRENT_TIME, CURRENT_TIME+dynamicIO, process, STATE_BLOCKED, STATE_READY);
          discreteEventSimulator.putEvent(readyEvent);
        }

        break;
      }
      case TRANS_TO_PREEMPT:{
      // add to runqueue (no event is generated)
        // if(process->getProcessStateDuration(CURRENT_TIME)==scheduler->getTimeQuantum()){
        //   process->updateDynamicPrio();
        // }
        process->updateProcessState(STATE_READY, CURRENT_TIME);
        
        scheduler->addProcess(CURRENT_RUNNING_PROCESS);
        CURRENT_RUNNING_PROCESS = nullptr;
        CALL_SCHEDULER = true;
        break;
      }
    }
    // remove current event object from Memory
    delete event; event = nullptr;
    if(CALL_SCHEDULER) {
      // cout<<"inside ";
      if (discreteEventSimulator.getNextEventTime() == CURRENT_TIME)
        continue; //process next event from Event queue
      CALL_SCHEDULER = false; // reset global flag
      // cout<<"condt "<<CURRENT_RUNNING_PROCESS<<" "<<scheduler->processInQueue()<<"\n";
      if (CURRENT_RUNNING_PROCESS == nullptr && scheduler->processInQueue()) {
        CURRENT_RUNNING_PROCESS = scheduler->getNextProcess();
        // cout<<"inside\n";
        // create event to make this process runnable for same time.
        Event * runEvent = new Event(CURRENT_TIME, CURRENT_TIME, CURRENT_RUNNING_PROCESS, STATE_READY, STATE_RUNNING);
        discreteEventSimulator.putEvent(runEvent);
      } 
    } 
  }
  TOTAL_TIME = CURRENT_TIME; 
}


void parseArgs(int argc, char* argv[]){
  DEBUG_FLAG = false;
  char *cvalue = NULL;
  int c;

  opterr = 0;

  while ((c = getopt (argc, argv, "vs:")) != -1)
    switch (c)
      {
      case 'v':
        DEBUG_FLAG = true;
        break;
      case 's':
        cvalue = optarg;
        sscanf(optarg, "%c%d:%d", &sched, &qnum, &maxprio);
        break;
      case '?':
        if (optopt == 's')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return;
      default:
        abort ();
      }

  // cout<<DEBUG_FLAG<<" "<<sched<<" "<<qnum<<" "<<maxprio<<"\n";
  // cout<<argv[optind]<<" "<<argv[optind+1]<<"\n";
  FILE_PATH = argv[optind];
  R_PATH = argv[optind+1];
}


int main(int argc, char* argv[]){
  parseArgs(argc, argv);
  // cout<<1<<"\n";
  initializeRandom();
  initializeSimulator();
  // cout<<2<<"\n";
  SchedulerInterface *scheduler;
  string schedulerName;

  switch (sched){
    case 'F':
      scheduler = new FCFS();
      schedulerName = "FCFS";
      break;
    case 'L':
      scheduler = new LCFS();
      schedulerName = "LCFS";
      break;
    case 'S':
      scheduler = new SRTF();
      schedulerName = "SRTF";
      break;
    case 'R':
      scheduler = new FCFS();
      schedulerName = "RR "+ to_string(qnum);
      scheduler->setTimeQuantum(qnum);
      break;
    case 'P':
      scheduler = new PRIO();
      schedulerName = "PRIO "+ to_string(qnum);
      scheduler->setTimeQuantum(qnum);
      break;
    case 'E':
      scheduler = new PREPRIO();
      schedulerName = "PREPRIO "+ to_string(qnum);
      scheduler->setTimeQuantum(qnum);
      break;
    default:
      scheduler = new FCFS();
      schedulerName = "FCFS";
  }
  // cout<<3<<"\n";
  Simulation(scheduler);
  // cout<<4<<"\n";

  float ATT = 0, ACW = 0, TCPUT = 0;
  cout<<schedulerName<<"\n";
  for(int i=0; i<processes.size(); i++){
    Process p = *processes[i];

    printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
      p.getPID(), p.getAT(), 
      p.getTC(), p.getCB(), 
      p.getIO(), p.getStaticPriority(), 
      p.getFT(), p.getTT(), 
      p.getIT(), p.getCW());

    ATT+=p.getTT()/(float)processes.size();
    ACW+=p.getCW()/(float)processes.size();
    TCPUT+=p.getTC();
  }
  printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", 
    TOTAL_TIME, 100*(TCPUT/(float)TOTAL_TIME), 
    100*((float)IO_TIME/(float)TOTAL_TIME), 
    ATT, ACW, 
    100*((float)processes.size()/(float)TOTAL_TIME));
  
  return 0;
}