#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <deque>
#include <cstring>

using namespace std;


class FTE {
public:
    int frame_no, proc_id, vpage;
    bool victim;
    unsigned int age;
    unsigned long long time_last_used;

    FTE(int frame_no, int proc_id, int vpage)
            :
            frame_no(frame_no),
            proc_id(proc_id),
            vpage(vpage) {
        victim = false;
        age = 0;
        time_last_used = 0;
    }

};


class PTE {
public:
    unsigned int frame_count: 7;
    unsigned int referenced: 1;
    unsigned int file_mapped: 1;
    unsigned int write_protect: 1;
    unsigned int pagedout: 1;
    unsigned int present: 1;
    unsigned int modified: 1;
    unsigned int valid: 1;

    PTE() {
        write_protect = 0;
        referenced = 0;
        valid = 0;
        modified = 0;
        present = 0;
        pagedout = 0;
        frame_count = 0;
        file_mapped = 0;
    }

    void purge_entry() {
        write_protect = 0;
        referenced = 0;
        valid = 0;
        modified = 0;
        present = 0;
        pagedout = 0;
        frame_count = 0;
        file_mapped = 0;
    }

};

typedef vector<PTE> pages;


class VMA {
public:
    int start_vpage, end_vpage, write_protected, file_mapped;

    VMA(int start_vpage, int end_vpage, int write_protected, int file_mapped) :
            start_vpage(start_vpage),
            end_vpage(end_vpage),
            write_protected(write_protected),
            file_mapped(file_mapped) {}
};


// Globals
bool O_flag = false, P_flag = false, F_flag = false, S_flag = false, DEBUG = false;
int MAX_FRAMES = 128, ofs = 0, MAX_VPAGES = 64;
string INPUT_FILE_PATH, RAND_FILE_PATH;
vector<int> rand_vals;
vector<FTE> frame_table;
deque<FTE *> framefreelist;
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


int myrandom() {
    int rand_val = rand_vals[(ofs) % rand_vals.size()] % MAX_FRAMES;
    ofs++;
    return rand_val;
}


class ProcessStats {
public:
    unsigned long long unmaps, maps, ins, outs, fins, fouts, zeros, segv, segprot;

    ProcessStats() {
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

vector<ProcessStats *> process_stat_list;


class Process {
public:
    int id;
    vector<VMA *> vmas;
    pages page_table;

    explicit Process(int id)
            :
            id(id) {
        for (int i = 0; i < MAX_VPAGES; i++) {
            PTE page;
            page_table.push_back(page);
        }
    }

    void set_vma(int start_vpage, int end_vpage, int write_protected, int file_mapped) {
        VMA *vma = new VMA(start_vpage, end_vpage, write_protected, file_mapped);
        vmas.push_back(vma);
    }
};

vector<Process *> processes;


class Pager {
public:
    int hand{};

    virtual FTE *select_victim_frame() = 0;
};

class FIFO : public Pager {
public:

    FIFO() {
        hand = 0;
    }

    FTE *select_victim_frame() override {
        FTE *victim_frame = &frame_table[hand];
        victim_frame->victim = true;
        hand = (hand + 1) % frame_table.size();
        return victim_frame;
    }

};

class Clock : public Pager {
public:

    Clock() {
        hand = 0;
    }

    FTE *select_victim_frame() override {
        FTE *victim_frame = nullptr;
        PTE *pg = &processes[frame_table[hand].proc_id]->page_table[frame_table[hand].vpage];
        while (pg->referenced) {
            pg->referenced = 0;
            hand++;
            if (hand == frame_table.size()) hand = 0;
            pg = &processes[frame_table[hand].proc_id]->page_table[frame_table[hand].vpage];
        }
        if (!pg->referenced) {
            victim_frame = &frame_table[hand];
            victim_frame->victim = true;
            hand = (hand + 1) % frame_table.size();

        }
        return victim_frame;
    }

};

class ESC : public Pager {
public:
    unsigned long long last_inst;
    FTE *frame_classes[4]{};

    ESC() {
        hand = 0;
        last_inst = 0;
    }

    FTE *select_victim_frame() override {
        FTE *victim_frame = nullptr;

        for (auto &Clas: frame_classes) Clas = nullptr;
        int classes = 0;

        for (int i = 0, j = hand; i < frame_table.size(); i++, j++) {
            j = j % frame_table.size();
            PTE *page = &processes[frame_table[j].proc_id]->page_table[frame_table[j].vpage];
            int classidx = page->referenced * 2 + page->modified;

            if (frame_classes[classidx] == nullptr) {
                frame_classes[classidx] = &frame_table[j];
                classes++;
            }

            if (classes == 4) break;
        }

        for (auto &f_class: frame_classes) {
            if (f_class != nullptr) {
                victim_frame = f_class;
                hand = (victim_frame->frame_no + 1) % frame_table.size();
                victim_frame->victim = true;
                break;
            }
        }

        //if (victim_frame == NULL) cout<<"error\n";

        if (inst_count - last_inst + 1 >= 50) {
            auto it = frame_table.begin();
            while (it != frame_table.end()) {
                if ((*it).proc_id != -1) {
                    processes[(*it).proc_id]->page_table[(*it).vpage].referenced = 0;
                }
                it++;
            }
            last_inst = inst_count + 1;
        }
        return victim_frame;
    }

};

class Aging : public Pager {
public:

    Aging() {
        hand = 0;
    }

    FTE *select_victim_frame() override {
        FTE *victim_frame = &frame_table[hand];
        for (int i = 0, j = hand; i < frame_table.size(); i++, j++) {
            j = j % frame_table.size();
            FTE *frame = &frame_table[j];
            frame->age = frame->age >> 1;

            if (processes[frame->proc_id]->page_table[frame->vpage].referenced == 1) {
                frame->age = (frame->age | 0x80000000);
                processes[frame->proc_id]->page_table[frame->vpage].referenced = 0;
            }

            if (frame->age < victim_frame->age) {
                victim_frame = frame;
            }
        }

        hand = (victim_frame->frame_no + 1) % frame_table.size();
        victim_frame->victim = true;
        return victim_frame;
    }

};

class WorkingSet : public Pager {
public:
    unsigned long long time_last_used;

    WorkingSet() {
        hand = 0;
        time_last_used = 0;
    }

    FTE *select_victim_frame() override {
        time_last_used = 0;
        FTE *victim_frame = nullptr;

        for (int i = 0, j = hand; i < frame_table.size(); i++, j++) {
            j = j % frame_table.size();
            FTE *frame = &frame_table[j];
            PTE *pg = &processes[frame->proc_id]->page_table[frame->vpage];

            if (pg->referenced) {
                pg->referenced = 0;
                frame->time_last_used = inst_count;
            } else {
                if (inst_count - frame->time_last_used >= 50) {
                    victim_frame = frame;
                    victim_frame->victim = true;
                    hand = (victim_frame->frame_no + 1) % frame_table.size();
                    break;
                } else {
                    if (inst_count - frame->time_last_used > time_last_used) {
                        time_last_used = inst_count - frame->time_last_used;;
                        victim_frame = frame;
                        victim_frame->victim = true;
                        hand = (victim_frame->frame_no + 1) % frame_table.size();
                    }
                }
            }

        }
        if (victim_frame == nullptr) {
            victim_frame = &frame_table[hand];
            victim_frame->victim = true;
            hand = (victim_frame->frame_no + 1) % frame_table.size();
        }
        return victim_frame;
    }
};

class Random : public Pager {
public:

    Random() {
        hand = 0;
    }

    FTE *select_victim_frame() override {
        int r = myrandom();
        FTE *victim_frame = &frame_table[r];
        victim_frame->victim = true;
        return victim_frame;
    }

};

Pager *pager = nullptr;


class Instruction {
public:
    char inst_type;
    int proc_id, vpage;

    Instruction(char inst_type, int num) :
            inst_type(inst_type) {
        if (inst_type == 'c' || inst_type == 'e') proc_id = num;
        else vpage = num;
    }
};

vector<Instruction> instructions;


FTE *allocate_frame_from_free_list() {
    if (framefreelist.empty()) return nullptr;
    FTE *f = framefreelist.front();
    framefreelist.pop_front();
    return f;
}


FTE *get_frame() {
    FTE *frame = allocate_frame_from_free_list();
    if (frame == nullptr) frame = pager->select_victim_frame();

    return frame;
}


void print_instruction(Instruction instruction) {
    int idx = (instruction.inst_type == 'c' || instruction.inst_type == 'e') ? instruction.proc_id : instruction.vpage;
    cout << inst_count << ": ==> " << instruction.inst_type << " " << idx << endl;
}


void Simulation() {
    /*
     * Simulate the instruction execution.
     */
    Process *curr_process;

    for (; inst_count < instructions.size(); inst_count++) {

        Instruction instr = instructions[inst_count];

        if (O_flag) print_instruction(instr);

        // Context switch
        if (instr.inst_type == 'c') {
            ctx_switches++;
            curr_process = processes[instr.proc_id];
            cost += 130;
        }

            // Exiting process
        else if (instr.inst_type == 'e') {
            process_exits++;
            cost += 1250;

            cout << "EXIT current process " << instr.proc_id << endl;
            for (auto it = processes[instr.proc_id]->page_table.begin();
                 it != processes[instr.proc_id]->page_table.end(); it++) {
                // Get reference to update object attributes
                PTE *pg = &(*it);

                // Unmap
                if (pg->present) {
                    FTE *frame = &frame_table[pg->frame_count];
                    int pid = frame->proc_id, vpage = frame->vpage;

                    frame->proc_id = -1;
                    frame->vpage = -1;
                    frame->victim = false;
                    frame->age = 0;
                    frame->time_last_used = 0;
                    framefreelist.push_back(frame);

                    if (O_flag) {
                        cout << " UNMAP " << pid << ":" << vpage << endl;
                    }
                    process_stat_list[instr.proc_id]->unmaps++;
                    cost += 400;

                    // Print modified page
                    if (pg->modified && pg->file_mapped) {
                        process_stat_list[instr.proc_id]->fouts++;
                        cost += 2400;

                        if (O_flag) {
                            cout << " FOUT" << endl;
                        }
                    }
                }
                pg->purge_entry();
            }
        }

            //read and write
        else {
            cost++;
            PTE *page = &curr_process->page_table[instr.vpage];

            if (!page->present) {
                if (!page->valid) {
                    for (auto &vma: curr_process->vmas) {
                        if (instr.vpage >= vma->start_vpage && instr.vpage <= vma->end_vpage) {
                            page->valid = 1;
                            page->file_mapped = vma->file_mapped;
                            page->write_protect = vma->write_protected;
                            break;
                        }
                    }

                    if (!page->valid) {
                        process_stat_list[curr_process->id]->segv++;
                        cost += 340;

                        if (O_flag) {
                            cout << " SEGV" << endl;
                        }
                        continue;
                    }
                }

                FTE *newframe = get_frame();
                if (newframe->victim) {
                    // Unmap
                    PTE *pg = &processes[newframe->proc_id]->page_table[newframe->vpage];
                    pg->present = 0;

                    process_stat_list[newframe->proc_id]->unmaps++;
                    cost += 400;

                    if (O_flag) cout << " UNMAP " << newframe->proc_id << ":" << newframe->vpage << endl;

                    // Save frame to disk
                    if (pg->modified) {
                        if (!pg->file_mapped) {
                            pg->pagedout = 1;

                            process_stat_list[newframe->proc_id]->outs++;
                            cost += 2700;

                            if (O_flag) cout << " OUT" << endl;
                        } else {
                            process_stat_list[newframe->proc_id]->fouts++;
                            cost += 2400;

                            if (O_flag) cout << " FOUT" << endl;
                        }
                        pg->modified = 0;
                        pg->frame_count = 0;

                    }

                }

                // Fill frame
                if (!page->file_mapped) {
                    if (page->pagedout) {
                        process_stat_list[curr_process->id]->ins++;
                        cost += 3100;

                        if (O_flag) cout << " IN" << endl;
                    } else {
                        process_stat_list[curr_process->id]->zeros++;
                        cost += 140;

                        if (O_flag) cout << " ZERO" << endl;
                    }
                } else {
                    process_stat_list[curr_process->id]->fins++;
                    cost += 2800;

                    if (O_flag) cout << " FIN" << endl;
                }

                // Map page to frame.
                newframe->proc_id = curr_process->id;
                newframe->vpage = instr.vpage;
                newframe->age = 0;
                newframe->time_last_used = inst_count;
                process_stat_list[curr_process->id]->maps++;
                page->present = 1;
                page->frame_count = newframe->frame_no;

                cost += 300;
                if (O_flag) cout << " MAP " << newframe->frame_no << endl;

            }

            page->referenced = 1;

            if (instr.inst_type == 'w' && page->write_protect) {
                process_stat_list[curr_process->id]->segprot++;
                cost += 420;

                if (O_flag) cout << " SEGPROT" << endl;
            } else if (instr.inst_type == 'w' && !page->write_protect) page->modified = 1;
        }
    }
}


void initializeSimulator() {
    /*
     * Initialize the required components for simulation.
     */
    ifstream input_file(INPUT_FILE_PATH);

    if (!input_file) {
        cout << "File at " << INPUT_FILE_PATH << " not found." << endl;
        exit(1);
    }

    if (input_file.is_open()) {
        string line;
        int num_process;

        getline(input_file, line);

        // Ignore '#' lines
        while (line[0] == '#') {
            getline(input_file, line);
        }

        num_process = stoi(line);

        for (int i = 0; i < num_process; i++) {
            int num_vma, start_vpage, end_vpage, write_protected, file_mapped;

            getline(input_file, line);

            // Ignore '#' lines
            while (line[0] == '#') {
                getline(input_file, line);
            }

            num_vma = stoi(line);
            auto *process = new Process(i);
            auto *pstats = new ProcessStats();
            for (int j = 0; j < num_vma; j++) {
                getline(input_file, line);
                istringstream ss(line);
                ss >> start_vpage >> end_vpage >> write_protected >> file_mapped;
                process->set_vma(start_vpage, end_vpage, write_protected, file_mapped);
            }
            processes.push_back(process);
            process_stat_list.push_back(pstats);
        }

        while (getline(input_file, line)) {
            if (line[0] == '#') continue;
            char c;
            int num;
            istringstream ss(line);
            ss >> c >> num;
            Instruction instruction(c, num);
            instructions.push_back(instruction);
        }

        input_file.close();
    }

    //initialise frame table and framefreelist
    for (int i = 0; i < MAX_FRAMES; i++) {
        FTE f(i, -1, -1);
        frame_table.push_back(f);
    }
    for (int i = 0; i < MAX_FRAMES; i++) {
        FTE *f = &frame_table[i];
        framefreelist.push_back(f);
    }
}


void print_pagetable(Process *process) {
    cout << "PT[" << process->id << "]: ";
    for (int i = 0; i < MAX_VPAGES; i++) {
        if (process->page_table[i].present) {
            cout << i << ":";
            if (process->page_table[i].referenced) cout << "R";
            else cout << "-";
            if (process->page_table[i].modified) cout << "M";
            else cout << "-";
            if (process->page_table[i].pagedout) cout << "S";
            else cout << "-";
            if (i != MAX_VPAGES - 1)
                cout << " ";
        } else {
            if (process->page_table[i].pagedout) cout << "#";
            else cout << "*";
            if (i != MAX_VPAGES - 1)
                cout << " ";
        }
    }
    cout << endl;
}


void print_frametable() {
    /*
     * Print final states for plain table entries.
     */
    cout << "FT: ";
    for (auto frame_it = frame_table.begin(); frame_it != frame_table.end(); frame_it++) {
        FTE frame = *frame_it;

        if (frame.proc_id == -1) cout << "*";
        else cout << frame.proc_id << ":" << frame.vpage;

        // Print spacing only until the last frame.
        if (frame_it != frame_table.end() - 1)
            cout << " ";
    }
    cout << endl;
}


void print_summary() {
    /*
     * Print aggregated statistics.
     */
    ProcessStats *pstats;
    for (auto &process: processes) {
        int process_id = process->id;
        pstats = process_stat_list[process_id];

        printf("PROC[%d]: U=%llu M=%llu I=%llu O=%llu FI=%llu FO=%llu Z=%llu SV=%llu SP=%llu\n", process_id,
               pstats->unmaps, pstats->maps,
               pstats->ins, pstats->outs, pstats->fins, pstats->fouts, pstats->zeros, pstats->segv, pstats->segprot);
    }

    printf("TOTALCOST %llu %llu %llu %llu %lu\n", inst_count, ctx_switches, process_exits, cost, sizeof(PTE));
}


void summary() {
    if (P_flag) {
        for (auto &process: processes) print_pagetable(process);
    }
    if (F_flag) {
        print_frametable();
    }
    if (S_flag) print_summary();
}


void parseArgs(int argc, char **argv) {
    /*
     * Parse commandline arguments.
     */
    int c;
    char algo;
    // Disable getopt error output

    while ((c = getopt(argc, argv, "df::a:o:")) != -1)
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
                sscanf(optarg, "%c", &algo);
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

//    cout << INPUT_FILE_PATH << RAND_FILE_PATH << endl;

    switch (algo) {
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
}


int main(int argc, char **argv) {
    parseArgs(argc, argv);

    // Initialize the simulator
    initRandNums();
    initializeSimulator();
    Simulation();
    summary();
}