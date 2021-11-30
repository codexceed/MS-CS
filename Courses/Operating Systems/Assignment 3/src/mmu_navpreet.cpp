#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <deque>

int frames = 128;
const int ptes = 64;

using namespace std;

bool O = false;
bool P = false;
bool F = false;
bool S = false;

unsigned long long ctx_switches = 0;
unsigned long long process_exits = 0;
unsigned long long cost = 0;

vector<int> randvals;
int ofs = 0;

int myrandom() {

  int rand = randvals[ofs] % frames;
  ofs++;
  if (ofs==randvals.size()) ofs = 0;
  return rand; 

}

class pte{
  public:
  unsigned int frame_no:7;
  unsigned int present:1;
  unsigned int reference:1;
  unsigned int modified:1;
  unsigned int write_protect:1;
  unsigned int pagedout:1;
  unsigned int valid:1;
  unsigned int file_mapped:1;

  pte(){
    this->frame_no = 0;
    this->present = 0;
    this->reference = 0;
    this->modified = 0;
    this->write_protect = 0;
    this->pagedout = 0;
    this->valid = 0;
    this->file_mapped = 0;
  }

};

class fte{
  public:
  int frame_no, proc_id, vpage;
  bool victim;
  unsigned int age;
  unsigned long long time_last_used;

  fte(int frame_no, int proc_id, int vpage){
    this->frame_no = frame_no;
    this->proc_id = proc_id;
    this->vpage = vpage;
    this->victim = false;
    this->age = 0;
    this->time_last_used = 0;
  }

};

vector<fte> fte_t;
deque<fte*> framefreelist;

void print_frametable(){
  vector<fte>:: iterator it = fte_t.begin();
  cout<<"FT: ";
  while(it!=fte_t.end()){
    if ( (*it).proc_id == -1) cout<<"* ";
    else cout<<(*it).proc_id<<":"<<(*it).vpage;
    it++;
    if (it != fte_t.end())
        cout << " ";
  }
  cout<<endl;
}

class vma{
  public:

  int start_vpage, end_vpage, write_protected, file_mapped;

  vma(int start_vpage, int end_vpage, int write_protected, int file_mapped){
    this->start_vpage = start_vpage;
    this->end_vpage = end_vpage;
    this->write_protected = write_protected;
    this->file_mapped = file_mapped;
  }

};



class Process{

  public:

  int proc_id;
  int num_vmas;
  vector<vma*> vmas;
  vector<pte> pte_t;

  Process(int proc_id, int num_vmas){
    this->proc_id = proc_id;
    this->num_vmas = num_vmas;
    for(int i=0;i<ptes;i++){
      pte page;
      this->pte_t.push_back(page);
    }
  }

  void set_vma(int start_vpage, int end_vpage, int write_protected, int file_mapped){
    vma *v = new vma(start_vpage, end_vpage, write_protected, file_mapped);
    this->vmas.push_back(v);
  }

  void print(){
    for(int i=0;i<num_vmas;i++){
      cout<<i<<" "<<vmas[i]->start_vpage<<" "<<vmas[i]->end_vpage<<" "<<vmas[i]->write_protected<<" "<<vmas[i]->file_mapped<<endl;
    }
  }

};

vector<Process*> processes;

class Pstats{
  public:
  unsigned long long unmaps;
  unsigned long long maps;
  unsigned long long ins;
  unsigned long long outs;
  unsigned long long fins;
  unsigned long long fouts;
  unsigned long long zeros;
  unsigned long long segv;
  unsigned long long segprot;

  Pstats(){
    this->unmaps = 0;
    this->maps = 0;
    this->ins = 0;
    this->outs = 0;
    this->fins = 0;
    this->fouts = 0;
    this->zeros = 0;
    this->segv = 0;
    this->segprot = 0;
  }

};

vector<Pstats*> pstatistics;

void print_pagetable(Process *p){
  cout<<"PT["<<p->proc_id<<"]: ";
  for(int i=0;i<ptes;i++){
    if (p->pte_t[i].present){
      cout<<i<<":";
      if (p->pte_t[i].reference) cout<<"R";
      else cout<<"-";
      if (p->pte_t[i].modified) cout<<"M";
      else cout<<"-";
      if (p->pte_t[i].pagedout) cout<<"S ";
      else cout<<"- ";
    }
    else{
        if (p->pte_t[i].pagedout) cout<<"#";
        else cout<<"*";
        if (i != ptes - 1)
        cout << " ";
    }
  }
  cout<<endl;
}

class Instruction{
  public:

  char inst_type;
  int proc_id, vpage;

  Instruction(char inst_type, int num){
    this->inst_type = inst_type;
    if (inst_type=='c' || inst_type=='e'){
      this->proc_id = num;
    }
    else{
      this->vpage = num;
    }
    
  }
};

vector<Instruction> instructions;
unsigned long long inst_count = 0;

class Pager{
  public:
  int hand;
  virtual fte* select_victim_frame() = 0;
};

class FIFO : public Pager{
  public:

  FIFO(){
    this->hand = 0;
  }

  fte* select_victim_frame(){
    fte* victim_frame = &fte_t[this->hand];
    this->hand++;
    if (this->hand == fte_t.size()) this->hand = 0;
    victim_frame->victim = true;
    return victim_frame;
  }

};

class Random : public Pager{
  public:
  
  Random(){
    this->hand = 0;
  }

  fte* select_victim_frame(){
    int r = myrandom();
    fte* victim_frame = &fte_t[r];
    victim_frame->victim = true;
    return victim_frame;
  }

};

class Clock : public Pager{
  public:

  Clock(){
    this->hand = 0;
  }

  fte* select_victim_frame(){
    fte* victim_frame;
    pte* pg = &processes[fte_t[this->hand].proc_id]->pte_t[fte_t[this->hand].vpage];
    while (pg->reference){
      pg->reference = 0;
      this->hand++;
      if (this->hand == fte_t.size()) this->hand = 0;
      pg = &processes[fte_t[this->hand].proc_id]->pte_t[fte_t[this->hand].vpage];
    }
    if (!pg->reference){
      victim_frame = &fte_t[this->hand];
      this->hand++;
      if (this->hand == fte_t.size()) this->hand = 0;
      victim_frame->victim = true;
      
      }
    //else cout<<"error";
    return victim_frame;
  }

};

class ESC : public Pager{
  public:
  unsigned long long last_inst;
  fte* Class[4];

  ESC(){
    this->hand = 0;
    this->last_inst = 0;
  }

  fte* select_victim_frame(){

    //cout<<this->hand<<" ";
    //if (inst_count - this->last_inst >= 50) cout<<"1 |";
    //else cout<<"0 |";
    //int lowest_class, victim_frameno, frame_scanned;

    fte* victim_frame = NULL;

    for(int i=0;i<4;i++) Class[i] = NULL;
    int classes = 0;

    for(int cnt=0,i=this->hand;cnt<fte_t.size();cnt++,i++){
      //frame_scanned = cnt+1;
      if (i == fte_t.size()) i = 0;
      pte* pg = &processes[fte_t[i].proc_id]->pte_t[fte_t[i].vpage];
      int classidx = pg->reference*2 + pg->modified;
      if (Class[classidx] == NULL){
        Class[classidx] = &fte_t[i];
        classes++;
      }
      if (classes == 4) break;
    }

    for(int i=0;i<4;i++){
      if (Class[i] != NULL){
        //lowest_class = i;
        //victim_frameno = Class[i]->frame_no;

        victim_frame = Class[i];
        this->hand = victim_frame->frame_no + 1;
        if (this->hand == fte_t.size()) this->hand = 0;
        victim_frame->victim = true;
        break;
      }
    }

    //if (victim_frame == NULL) cout<<"error\n";
    
    if (inst_count - this->last_inst + 1 >= 50){
      vector<fte>:: iterator it = fte_t.begin();
      while(it != fte_t.end()){
        if ( (*it).proc_id != -1){
          processes[(*it).proc_id]->pte_t[(*it).vpage].reference = 0;
        }
        it++;
      }
      last_inst = inst_count + 1;
    }
    //cout<<lowest_class<<" "<<victim_frameno<<" "<<frame_scanned<<endl;
    return victim_frame;
  }

};

class Aging : public Pager{
  public:

  Aging(){
    this->hand = 0;
  }

  fte* select_victim_frame(){
    fte* victim_frame = &fte_t[this->hand];
    for(int cnt=0,i=this->hand;cnt<fte_t.size();cnt++,i++){
      if (i == fte_t.size()) i = 0;
      fte *frame = &fte_t[i];
      frame->age = frame->age >> 1;
      if (processes[frame->proc_id]->pte_t[frame->vpage].reference == 1){
        frame->age = (frame->age | 0x80000000);
        processes[frame->proc_id]->pte_t[frame->vpage].reference = 0;
      }
      if (frame->age < victim_frame->age){
        victim_frame = frame;
      }
    }

    this->hand = victim_frame->frame_no + 1;
    if (this->hand == fte_t.size()) this->hand = 0;
    victim_frame->victim = true;
    return victim_frame;
  }

};

class WorkingSet : public Pager{
  public:
  unsigned long long time_last_used;

  WorkingSet(){
    this->hand = 0;
    this->time_last_used = 0;
  }

  fte* select_victim_frame(){
    this->time_last_used = 0;
    //int fstart,fend;
    //fstart = this->hand;
    fte* victim_frame = NULL;

    for(int cnt=0,i=this->hand;cnt<fte_t.size();cnt++,i++){
      if (i == fte_t.size()) i = 0;
      fte* frame = &fte_t[i];
      pte* pg = &processes[frame->proc_id]->pte_t[frame->vpage];

      //cout<<frame->frame_no<<" ("<<pg->reference<<" "<<frame->proc_id<<":"<<frame->vpage<<" "<<frame->time_last_used<<") | ";

      if (pg->reference){
        pg->reference = 0;
        frame->time_last_used = inst_count;
      }
      else{
        if (inst_count - frame->time_last_used >= 50){
          victim_frame = frame;
          victim_frame->victim = true;
          this->hand = victim_frame->frame_no + 1;
          if (this->hand == fte_t.size()) this->hand = 0;
          break;
        }
        else{
          if (inst_count - frame->time_last_used > time_last_used){
            time_last_used = inst_count - frame->time_last_used;;
            victim_frame = frame;
            victim_frame->victim = true;
            this->hand = victim_frame->frame_no + 1;
            if (this->hand == fte_t.size()) this->hand = 0;
          }
        }
      }
      //fend = i;
      
    }
    if (victim_frame == NULL){
      victim_frame = &fte_t[this->hand];
      victim_frame->victim = true;
      this->hand = victim_frame->frame_no + 1;
      if (this->hand == fte_t.size()) this->hand = 0;
    }
    //cout<<fstart<<" "<<fend<<endl;
    return victim_frame;
  }
  
  
};

Pager *pager = NULL;

fte* allocate_frame_from_free_list(){
  if (framefreelist.size()==0) return NULL;
  fte* f = framefreelist.front();
  framefreelist.pop_front();
  return f;
}

fte* get_frame(){
  fte* frame = allocate_frame_from_free_list();
  if (frame==NULL) frame = pager->select_victim_frame();
  
  return frame;
}

Process* curr_proc;

void Simulation(){
  
  for(;inst_count<instructions.size();inst_count++){

    Instruction instr = instructions[inst_count];

    if (O){
      if (instr.inst_type=='c' || instr.inst_type=='e'){
        cout<<inst_count<<": ==> "<<instr.inst_type<<" "<<instr.proc_id<<endl;
      }
      else{
        cout<<inst_count<<": ==> "<<instr.inst_type<<" "<<instr.vpage<<endl;
      }
    }
    
    //context switch
    if (instr.inst_type == 'c'){
      ctx_switches++;
      curr_proc = processes[instr.proc_id];
      cost += 130;
      continue;
    }

    //process exit
    if (instr.inst_type == 'e'){
      process_exits++;
      cost += 1250;

      cout<<"EXIT current process "<<instr.proc_id<<endl;
      vector<pte>:: iterator it = processes[instr.proc_id]->pte_t.begin();
      while(it != processes[instr.proc_id]->pte_t.end()){
        pte* pg = &(*it);
        //unmap page
        if (pg->present){
          fte* frame = &fte_t[pg->frame_no];
          int fpid = frame->proc_id;
          int fvpage = frame->vpage;
          frame->proc_id = -1;
          frame->vpage = -1;
          frame->victim = false;
          frame->age = 0;
          frame->time_last_used = 0;
          framefreelist.push_back(frame);
          if (O){
            cout<<" UNMAP "<<fpid<<":"<<fvpage<<endl;
          }
          pstatistics[instr.proc_id]->unmaps++;
          cost += 400;

          //fout modified page
          if (pg->modified){
            if (pg->file_mapped){
              pstatistics[instr.proc_id]->fouts++;
              cost += 2400;
              if (O){
                cout<<" FOUT"<<endl;
              }
            }
          }
        }
        pg->frame_no = 0;
        pg->present = 0;
        pg->reference = 0;
        pg->modified = 0;
        pg->write_protect = 0;
        pg->pagedout = 0;
        pg->valid = 0;
        pg->file_mapped = 0;
        it++;
      }
      continue;

    }
    
    //read and write
    cost++;
    pte* page = &curr_proc->pte_t[instr.vpage];
    
    if (!page->present){

      if (!page->valid){
        for(int j=0;j<curr_proc->vmas.size();j++){
          if (instr.vpage >= curr_proc->vmas[j]->start_vpage && instr.vpage <= curr_proc->vmas[j]->end_vpage){
            page->valid = 1;
            page->file_mapped = curr_proc->vmas[j]->file_mapped;
            page->write_protect = curr_proc->vmas[j]->write_protected;
            break;
          }
        }
      }

      
      if (!page->valid){
        pstatistics[curr_proc->proc_id]->segv++;
        cost += 340;
        if (O){
          cout<<" SEGV"<<endl;
        }
        continue;
      }

      fte* newframe = get_frame();

      if (newframe->victim){
        //unmap
        pte* pg = &processes[newframe->proc_id]->pte_t[newframe->vpage];
        pg->present = 0;
        pstatistics[newframe->proc_id]->unmaps++;
        cost += 400;
        if (O){
          cout<<" UNMAP "<<newframe->proc_id<<":"<<newframe->vpage<<endl;
        }

        //Save frame to disk
        if (pg->modified){
          if (!pg->file_mapped){
            pg->pagedout = 1;
            pstatistics[newframe->proc_id]->outs++;
            cost += 2700;
            if (O){
              cout<<" OUT"<<endl;
            }
          }
          else{
            pstatistics[newframe->proc_id]->fouts++;
            cost += 2400;
            if (O){
              cout<<" FOUT"<<endl;
            }
          }
          pg->modified = 0;
          pg->frame_no = 0;
          
        }

      }
      
      //Fill frame
      if (!page->file_mapped){
        if (page->pagedout){
          pstatistics[curr_proc->proc_id]->ins++;
          cost += 3100;
          if (O){
            cout<<" IN"<<endl;
          }
        }
        else{
          pstatistics[curr_proc->proc_id]->zeros++;
          cost += 140;
          if (O){
            cout<<" ZERO"<<endl;
          }
          
        }
      }
      else{
        pstatistics[curr_proc->proc_id]->fins++;
        cost += 2800;
        if (O){
          cout<<" FIN"<<endl;
        }
      }

      //map
      newframe->proc_id = curr_proc->proc_id;
      newframe->vpage = instr.vpage;
      newframe->age = 0;
      newframe->time_last_used = inst_count ;
      pstatistics[curr_proc->proc_id]->maps++;
      page->present = 1;
      page->frame_no = newframe->frame_no;
      
      cost += 300;
      if (O){
        cout<<" MAP "<<newframe->frame_no<<endl;
      }

    }
    
    page->reference = 1;
    
    if(instr.inst_type=='w' && page->write_protect){
      
      pstatistics[curr_proc->proc_id]->segprot++;
      cost += 420;
      if (O){
        cout<<" SEGPROT"<<endl;
      }
    }
    else if (instr.inst_type=='w' && !page->write_protect){
      page->modified = 1;
    }

    
    //print_pagetable(curr_proc);
    //print_frametable();
  }
  

}

void summary(){
  if (P){
    vector<Process* >:: iterator it = processes.begin();
    while(it!=processes.end()){
      print_pagetable(*it);
      it++;
    }
  }
  if (F){
    print_frametable();
  }
  if (S){
    vector<Process* >:: iterator it = processes.begin();
    Process* proc;
    Pstats* pstats;
    while(it!=processes.end()){
      proc = *it;
      pstats = pstatistics[proc->proc_id];
      printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",proc->proc_id,pstats->unmaps,pstats->maps,
      pstats->ins, pstats->outs,pstats->fins, pstats->fouts, pstats->zeros,pstats->segv, pstats->segprot);
      it++;
    }
    printf("TOTALCOST %lu %lu %lu %llu %lu\n",inst_count, ctx_switches, process_exits, cost, sizeof(pte));
  }
}

int main(int argc, char **argv){

  int c;

  while ((c = getopt (argc, argv, "f:a:o:")) != -1)
  switch (c)
    {
    case 'f':
      frames = stoi(optarg);
      break;
    case 'a':
      switch(optarg[0]){
        case 'f':
          pager = new FIFO();
          break;
        case 'r':
          pager = new Random();
          break;
        case 'c':
          pager = new Clock();
          break;
        case 'e':
          pager = new ESC();
          break;
        case 'a':
          pager = new Aging();
          break;
        case 'w':
          pager = new WorkingSet();
          break;
        default:
          break;
      }
      break;
    case 'o':
      for(int i=0;i<strlen(optarg);i++){
        if (optarg[i]=='O') O = true;
        if (optarg[i]=='P') P = true;
        if (optarg[i]=='F') F = true;
        if (optarg[i]=='S') S = true;
      }
      break;
    default:
      break;
    }

  if (pager == NULL){
    pager = new FIFO();
  }
  //Reading input file and random file
  
  fstream infile,rfile;

  infile.open(argv[optind],ios::in);
  optind++;
  rfile.open(argv[optind],ios::in);
  if (rfile.is_open()){  
    string line;
    getline(rfile, line);
    while(getline(rfile, line)){     
      randvals.push_back(stoi(line));
    }
  rfile.close();
  }
  
  if (infile.is_open()){
    string line;
    int num_process;

    getline(infile, line);
    while(line[0]=='#'){
      getline(infile, line);
    }
    num_process = stoi(line);

    for(int i=0;i<num_process;i++){
      
      int num_vmas, start_vpage, end_vpage, write_protected, file_mapped;
      getline(infile, line);
      while(line[0]=='#'){
        getline(infile, line);
      }
      num_vmas = stoi(line);
      Process *p = new Process(i,num_vmas);
      Pstats *pstats = new Pstats();
      for(int j=0;j<num_vmas;j++){
        getline(infile, line);
        istringstream ss(line);
        ss>>start_vpage>>end_vpage>>write_protected>>file_mapped;
        p->set_vma(start_vpage,end_vpage,write_protected,file_mapped);
      }
      processes.push_back(p);
      pstatistics.push_back(pstats);
    }
    while(getline(infile, line)){
      if (line[0]=='#') continue;
      char c;
      int num;
      istringstream ss(line);
      ss>>c>>num;
      Instruction ins(c,num);
      instructions.push_back(ins);
    }

    infile.close(); 
  }
  
  //initialise frame table and framefreelist
  
  for(int i=0;i<frames;i++){
    fte f(i,-1,-1);
    fte_t.push_back(f);
  }
  for(int i=0;i<frames;i++){
    fte* f = &fte_t[i];
    framefreelist.push_back(f);
  }

  Simulation();
  summary();

}