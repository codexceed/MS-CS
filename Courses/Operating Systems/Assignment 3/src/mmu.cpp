#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;


class FTE{
public:
    int frame_no, proc_id, vpage;
    bool victim;
    unsigned int age;
    unsigned long long time_last_used;

    FTE(int frame_no, int proc_id, int vpage)
            :
            frame_no(frame_no),
            proc_id(proc_id),
            vpage(vpage)
    {
        victim = false;
        age = 0;
        time_last_used = 0;
    }

};


// Globals
bool O_flag = false, P_flag = false, F_flag = false, S_flag = false, PRINT_CURR_TABLE = false, PRINT_ALL_TABLES = false, DEBUG = false;
int MAX_FRAMES = 128, ofs = 0, MAX_VPAGES=64;
string INPUT_FILE_PATH, RAND_FILE_PATH;
vector<int> rand_vals;
vector<FTE> frame_table;
deque<FTE*> framefreelist;
unsigned long long inst_count = 0, ctx_switches = 0, process_exits = 0, cost = 0;



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


class Pstats{
public:
    unsigned long long unmaps, maps, ins, outs, fins, fouts, zeros, segv, segprot;

    Pstats(){
        unmaps = 0;
        maps = 0;
        ins = 0;
        outs = 0;
        fins = 0;
        fouts = 0;
        zeros = 0;
        segv = 0;
        segprot = 0;
    }

};


vector<Pstats*> pstats;


class PTE{
public:
    unsigned int frame_count:7;
    unsigned int present:1;
    unsigned int referenced:1;
    unsigned int modified:1;
    unsigned int write_protect:1;
    unsigned int pagedout:1;
    unsigned int valid:1;
    unsigned int file_mapped:1;

    PTE(){
        frame_count = 0;
        present = 0;
        referenced = 0;
        modified = 0;
        write_protect = 0;
        pagedout = 0;
        valid = 0;
        file_mapped = 0;
    }

};


class VMA {
public:
    int start_vpage, end_vpage, write_protected, file_mapped;

    VMA(int start_vpage, int end_vpage, int write_protected, int file_mapped) :
            start_vpage(start_vpage),
            end_vpage(end_vpage),
            write_protected(write_protected),
            file_mapped(file_mapped) {}
};


class Pager{
public:
    int hand;
    virtual FTE* select_victim_frame() = 0;
};


Pager *pager = nullptr;


typedef vector<PTE> pages;


class Process {
public:
    int id, num_vma;
    vector<VMA *> vmas;
    pages page_table;

    Process(int id, int num_vma)
    :
    id(id),
    num_vma(num_vma){
    for(int i=0;i<MAX_VPAGES;i++){
        PTE page;
        page_table.push_back(page);
    }
    }

    void set_vma(int start_vpage, int end_vpage, int write_protected, int file_mapped){
        VMA *vma = new VMA(start_vpage, end_vpage, write_protected, file_mapped);
        vmas.push_back(vma);
    }
};


vector<Process*> processes;


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


//DiscreteEventSimulator initializeSimulator() {
//    DiscreteEventSimulator simulator;
//    ifstream input_file(INPUT_FILE_PATH);
//
//    if (!input_file) {
//        cout << "File at " << INPUT_FILE_PATH << " not found." << endl;
//        exit(1);
//    }
//
//    while (input_file >> AT >> TC >> CB >> IO) {
//        auto *process = new Process(AT, TC, CB, IO, PID);
//        auto *event = new Event(AT, process, CREATED, READY);
//        simulator.put_event(event);
//        PID++;
//    }
//
//    return simulator;
//}


FTE* allocate_frame_from_free_list(){
    if (framefreelist.empty()) return nullptr;
    FTE* f = framefreelist.front();
    framefreelist.pop_front();
    return f;
}


FTE* get_frame(){
    FTE* frame = allocate_frame_from_free_list();
    if (frame==nullptr) frame = pager->select_victim_frame();

    return frame;
}


void Simulation(){

    Process *curr_process;

    for(;inst_count<instructions.size();inst_count++){

        Instruction instr = instructions[inst_count];

        if (O_flag){
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
            curr_process = processes[instr.proc_id];
            cost += 130;
            continue;
        }

        //process exit
        if (instr.inst_type == 'e'){
            process_exits++;
            cost += 1250;

            cout<<"EXIT current process "<<instr.proc_id<<endl;
            auto it = processes[instr.proc_id]->page_table.begin();
            while(it != processes[instr.proc_id]->page_table.end()){
                PTE* pg = &(*it);
                //unmap page
                if (pg->present){
                    FTE* frame = &frame_table[pg->frame_count];
                    int fpid = frame->proc_id;
                    int fvpage = frame->vpage;
                    frame->proc_id = -1;
                    frame->vpage = -1;
                    frame->victim = false;
                    frame->age = 0;
                    frame->time_last_used = 0;
                    framefreelist.push_back(frame);
                    if (O_flag){
                        cout<<" UNMAP "<<fpid<<":"<<fvpage<<endl;
                    }
                    pstats[instr.proc_id]->unmaps++;
                    cost += 400;

                    //fout modified page
                    if (pg->modified){
                        if (pg->file_mapped){
                            pstats[instr.proc_id]->fouts++;
                            cost += 2400;
                            if (O_flag){
                                cout<<" FOUT"<<endl;
                            }
                        }
                    }
                }
                pg->frame_count = 0;
                pg->present = 0;
                pg->referenced = 0;
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
        PTE* page = &curr_process->page_table[instr.vpage];

        if (!page->present){

            if (!page->valid){
                for(auto & vma : curr_process->vmas){
                    if (instr.vpage >= vma->start_vpage && instr.vpage <= vma->end_vpage){
                        page->valid = 1;
                        page->file_mapped = vma->file_mapped;
                        page->write_protect = vma->write_protected;
                        break;
                    }
                }
            }


            if (!page->valid){
                pstats[curr_process->id]->segv++;
                cost += 340;
                if (O_flag){
                    cout<<" SEGV"<<endl;
                }
                continue;
            }

            FTE* newframe = get_frame();

            if (newframe->victim){
                //unmap
                PTE* pg = &processes[newframe->proc_id]->page_table[newframe->vpage];
                pg->present = 0;
                pstats[newframe->proc_id]->unmaps++;
                cost += 400;
                if (O_flag){
                    cout<<" UNMAP "<<newframe->proc_id<<":"<<newframe->vpage<<endl;
                }

                //Save frame to disk
                if (pg->modified){
                    if (!pg->file_mapped){
                        pg->pagedout = 1;
                        pstats[newframe->proc_id]->outs++;
                        cost += 2700;
                        if (O_flag){
                            cout<<" OUT"<<endl;
                        }
                    }
                    else{
                        pstats[newframe->proc_id]->fouts++;
                        cost += 2400;
                        if (O_flag){
                            cout<<" FOUT"<<endl;
                        }
                    }
                    pg->modified = 0;
                    pg->frame_count = 0;

                }

            }

            //Fill frame
            if (!page->file_mapped){
                if (page->pagedout){
                    pstats[curr_process->id]->ins++;
                    cost += 3100;
                    if (O_flag){
                        cout<<" IN"<<endl;
                    }
                }
                else{
                    pstats[curr_process->id]->zeros++;
                    cost += 140;
                    if (O_flag){
                        cout<<" ZERO"<<endl;
                    }

                }
            }
            else{
                pstats[curr_process->id]->fins++;
                cost += 2800;
                if (O_flag){
                    cout<<" FIN"<<endl;
                }
            }

            //map
            newframe->proc_id = curr_process->id;
            newframe->vpage = instr.vpage;
            newframe->age = 0;
            newframe->time_last_used = inst_count ;
            pstats[curr_process->id]->maps++;
            page->present = 1;
            page->frame_count = newframe->frame_no;

            cost += 300;
            if (O_flag){
                cout<<" MAP "<<newframe->frame_no<<endl;
            }

        }

        page->referenced = 1;

        if(instr.inst_type=='w' && page->write_protect){

            pstats[curr_process->id]->segprot++;
            cost += 420;
            if (O_flag){
                cout<<" SEGPROT"<<endl;
            }
        }
        else if (instr.inst_type=='w' && !page->write_protect){
            page->modified = 1;
        }
    }


}


void initializeSimulator(){
    /*
     * Initialize the required components for simulation.
     */
    ifstream input_file(INPUT_FILE_PATH);

    if (!input_file) {
        cout << "File at " << INPUT_FILE_PATH << " not found." << endl;
        exit(1);
    }

    if (input_file.is_open()){
        string line;
        int num_process;

        getline(input_file, line);

        // Ignore '#' lines
        while(line[0]=='#'){
            getline(input_file, line);
        }

        num_process = stoi(line);

        for(int i=0;i<num_process;i++){
            int num_vma, start_vpage, end_vpage, write_protected, file_mapped;

            getline(input_file, line);

            // Ignore '#' lines
            while(line[0]=='#'){
                getline(input_file, line);
            }

            num_vma = stoi(line);
            auto *process = new Process(i, num_vma);
            for(int j=0; j < num_vma; j++){
                getline(input_file, line);
                istringstream ss(line);
                ss>>start_vpage>>end_vpage>>write_protected>>file_mapped;
                process->set_vma(start_vpage, end_vpage, write_protected, file_mapped);
            }
            processes.push_back(process);
        }

        while(getline(input_file, line)){
            if (line[0]=='#') continue;
            char c;
            int num;
            istringstream ss(line);
            ss>>c>>num;
            Instruction instruction(c,num);
            instructions.push_back(instruction);
        }

        input_file.close();
    }

    //initialise frame table and framefreelist
    for(int i=0;i<MAX_FRAMES;i++){
        FTE f(i,-1,-1);
        frame_table.push_back(f);
    }
    for(int i=0;i<MAX_FRAMES;i++){
        FTE* f = &frame_table[i];
        framefreelist.push_back(f);
    }
}


char parseArgs(int argc, char **argv) {
    /*
     * Parse commandline arguments.
     */
    int c;
    char algo;
    // Disable getopt error output

    while ((c = getopt(argc, argv, "xydf::a:o:")) != -1)
        switch (c) {
            case 'o':
                for (int i = 0; i < strlen(optarg); i++) {
                    if (optarg[i] == 'O') O_flag = true;
                    if (optarg[i] == 'P') P_flag = true;
                    if (optarg[i] == 'F') F_flag = true;
                    if (optarg[i] == 'S') S_flag = true;
                }
                break;
            case 'f':
                sscanf(optarg, "%d", &MAX_FRAMES);
                break;
            case 'a':
                cout << optarg << endl;
                sscanf(optarg, "%c", &algo);
                break;
            case 'x':
                PRINT_CURR_TABLE = true;
                break;
            case 'y':
                PRINT_ALL_TABLES = true;
                break;
            case 'd':
                DEBUG = true;
                break;
            case '?':
                if (optopt == 'o')
                    fprintf(stderr, "Option -o is mandatory. Please provide the necessary arguments.");
                else if (optopt == 'a')
                    fprintf(stderr, "Option -a is mandatory. Please provide the necessary arguments.");
                else if (optopt != 'f')
                    fprintf(stderr, "Unknown option: '%d'", optopt);
                exit(1);
            default:
                exit(1);
        }

    if (DEBUG)
        printf("-f: %d, -a: %c, -o: O: %s, P: %s, F: %s, S: %s\n", MAX_FRAMES, algo,
               O_flag ? "true" : "false",
               P_flag ? "true" : "false",
               F_flag ? "true" : "false",
               S_flag ? "true" : "false");

    INPUT_FILE_PATH = argv[optind];
    RAND_FILE_PATH = argv[++optind];

    cout << INPUT_FILE_PATH << RAND_FILE_PATH << endl;

    return algo;
}


int main(int argc, char **argv) {
    char algo = parseArgs(argc, argv);

    // Initialize the simulator
    initRandNums();
    initializeSimulator();

//    switch(algo){
//        case 'f':
//            pager = new FIFO();
//            break;
//        case 'r':
//            pager = new Random();
//            break;
//        case 'c':
//            pager = new Clock();
//            break;
//        case 'e':
//            pager = new ESC();
//            break;
//        case 'a':
//            pager = new Aging();
//            break;
//        case 'w':
//            pager = new WorkingSet();
//            break;
//        default:
//            break;
//    }
}