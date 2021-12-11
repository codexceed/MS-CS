//
// Created by Sarthak Joshi on 12/9/21.
//
#include <iostream>
#include "string"
#include <unistd.h>
#include "deque"
#include "cmath"
#include "fstream"
#include "sstream"
#include "iterator"

using namespace std;


// Globals
bool VERBOSE = false, DEBUG = false;
string INPUT_FILE_PATH;


class Request {
public:
    int arr_time, start_time, end_time, track, id;

    Request(int arr_time, int track)
            : arr_time(arr_time), track(track) {};
};

typedef deque<Request *> requests_t;


class IOScheduler {
public:
    int curr_track = 0, scan_incr = 1, total_movement = 0, total_turnaround = 0, total_wait = 0, max_wait = 0;
    requests_t io_queue, disk;
    requests_t::iterator head = disk.begin();

    void set_head_to_id(requests_t *active_disk, int id) {
        // Adjust head after new mapping.
        auto disk_it = active_disk->begin();
        for (; disk_it < active_disk->end(); disk_it++) {
            if ((*disk_it)->id == id)
                break;
        }
        head = disk_it;
    }

    void map_request_to_disk(Request *request, requests_t *active_disk) {
        /*
         * Map new request on disk in order of tracks.
         */
        if (active_disk->empty()) {
            active_disk->push_back(request);
        } else {
            auto dq_it = active_disk->begin();
            Request *curr_request = *head;

            for (; dq_it != active_disk->end(); dq_it++)
                if ((*dq_it)->track > request->track)
                    break;

            if (dq_it == active_disk->end())
                active_disk->push_back(request);

            else active_disk->insert(dq_it, request);

            // Adjust head after new mapping.
            set_head_to_id(active_disk, curr_request->id);

            if (DEBUG) {
                cout << "Inserting IO ID: " << request->id << endl;
            }
        }
    }

    virtual void put_io(Request *request) {
        /*
         * Queue and map to disk, the incoming IO request.
         */
        io_queue.push_back(request);

        map_request_to_disk(request, &disk);
    };

    virtual Request *get_io(int time) {
        if (disk.empty())
            return nullptr;

        if (DEBUG) {
            if (head < disk.begin() || head >= disk.end()) {
                cout << "Current head ID: " << (*head)->id << endl;
            }
        }

        // Clear the previous IO request and move head to the next one.
        auto seek_target = head + scan_incr;

        if (DEBUG) {
            cout << "IO IDs on disk: ";
            for (auto io: disk) {
                cout << io->id << " ";
            }
            cout << endl;
        }

        if (seek_target < disk.begin() || seek_target >= disk.end()) {
            cout << "IO attempted on invalid disk track.";
            exit(1);
        }

        // Update IO request
        Request *io = *seek_target;
        io->start_time = time;
        int movement = abs(io->track - curr_track), wait_time = io->start_time - io->arr_time;;
        io->end_time = io->start_time + movement;

        // Update metrics
        total_movement += movement;
        total_turnaround += io->end_time - io->arr_time;
        total_wait += wait_time;
        max_wait = wait_time > max_wait ? wait_time : max_wait;

        // Change track
        curr_track = io->track;

        if (!(head == seek_target))
            disk.erase(head);
        set_head_to_id(&disk, io->id);

        return io;
    }

    Request *peek_disk(int incr) {
        /*
         * Peek the next targeted track in specified direction and return the request for it.
         */
        auto peek_head = head + incr;

        // Reached end of disk.
        if (peek_head < disk.begin() || peek_head == disk.end())
            return nullptr;

        return *peek_head;
    }
};

class FIFO : public IOScheduler {
public:
    Request *get_io(int time) override {
        if (io_queue.empty())
            return nullptr;

        // Find the target track on disk.
        Request *next_io = io_queue.front();
        auto disk_it = disk.begin();
        while (disk_it != disk.end()) {
            if ((*disk_it)->id == next_io->id)
                break;
            disk_it++;
        }

        // Determine the seek distance.
        // Ensure if head is on a valid position on disk.
        if (!(head >= disk.begin() && head < disk.end()))
            head = disk.begin();
        scan_incr = distance(head, disk_it);
        io_queue.pop_front();

        return IOScheduler::get_io(time);
    }
};

class SSTF : public IOScheduler {
public:
    Request *get_io(int time) final {
        /*
         * Find the IO request in queue with the shortest seek time and return it.
         */
        // Determine the nearest request by disk track.
        Request *up_req = peek_disk(-1), *down_req = peek_disk(1), *nearest_io;
        if (up_req == nullptr && down_req == nullptr) {
            if (DEBUG)
                cout << "IO queue empty.";
            return nullptr;
        }

        // Determine the nearest track for IO.
        if (up_req == nullptr) {
            scan_incr = 1;
        } else if (down_req == nullptr) {
            scan_incr = -1;
        } else {
            scan_incr = abs(up_req->track - curr_track) <= abs(down_req->track - curr_track) ? -1 : 1;
        }

        // Jump to the target track and get the IO request for it.
        return IOScheduler::get_io(time);
    }
};

class LOOK : public IOScheduler {
public:
    Request *get_io(int time) override {
        Request *next_io = peek_disk(scan_incr);

        // Switch direction if no more IO requests in current direction.
        if (next_io == nullptr)
            scan_incr *= -1;

        return IOScheduler::get_io(time);
    }
};

class CLOOK : public IOScheduler {
public:
    Request *get_io(int time) override {
        Request *next_io = peek_disk(scan_incr);

        // Jump back to first track
        if (next_io == nullptr) {
            head = disk.begin();
        }

        return IOScheduler::get_io(time);
    }
};

class FLOOK : public IOScheduler {
    requests_t add_disk;
public:
    void put_io(Request *request) override {
        /*
         * Map new request to add disk, instead of active disk.
         */
        map_request_to_disk(request, &add_disk);
    }

    Request *get_io(int time) override {
        if (disk.empty()) {
            disk = add_disk;
            add_disk.clear();
            scan_incr *= -1;
            head = scan_incr == 1 ? disk.begin() : disk.end();
        }

        return IOScheduler::get_io(time);

    }
};


class IOSimulator {
    int time = 0, io_idx = 0;
    requests_t io_requests;
    Request *curr_io = nullptr;
public:
    void add_io_event(Request *request) {
        io_requests.push_back(request);
    }

    void simulate(IOScheduler *scheduler) {
        while (true) {
            if (!io_requests.empty() && time == io_requests.front()->arr_time) {
                Request *new_io = io_requests.front();
                new_io->id = io_idx;
                io_idx++;
                scheduler->put_io(new_io);
                io_requests.pop_front();
            }

            if (curr_io != nullptr && time == curr_io->end_time) {
                printf("%5d: %5d %5d %5d\n", curr_io->id, curr_io->arr_time, curr_io->start_time, curr_io->end_time);

                curr_io = nullptr;
            }

            if (curr_io == nullptr) {
                curr_io = scheduler->get_io(time);

                if (curr_io == nullptr) {
                    // All events processed.
                    if (io_requests.empty())
                        break;
                }
            }

            time++;
        }

        printf("SUM: %d %d %.2lf %.2lf %d\n",
               time, scheduler->total_movement, (float) scheduler->total_turnaround / (float) io_idx, (float) scheduler->total_wait / (float) io_idx,
               scheduler->max_wait);

    }
};


IOSimulator initializeSimulator() {
    IOSimulator simulator;
    ifstream input_file(INPUT_FILE_PATH);

    if (!input_file) {
        cout << "File at " << INPUT_FILE_PATH << " not found." << endl;
        exit(1);
    }

    string line;
    while (getline(input_file, line)) {
        if (line[0] != '#') {
            istringstream iss(line);
            int time, track;

            //while the iss is a number
            if (!(iss >> time >> track)) {
                cout << "Error while parsing input.";
                exit(1);
            }

            auto *request = new Request(time, track);
            simulator.add_io_event(request);
        }
    }

    return simulator;
}


char parseArgs(int argc, char **argv) {
    char c, scheduler_code;
    // Disable getopt error output
    opterr = 0;

    while ((c = getopt(argc, argv, "dvs:")) != -1)
        switch (c) {
            case 's':
                sscanf(optarg, "%c", &scheduler_code);
                break;
            case 'v':
                VERBOSE = true;
                break;
            case 'd':
                DEBUG = true;
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

    INPUT_FILE_PATH = argv[optind];

    return scheduler_code;
}


int main(int argc, char **argv) {
    char sched_flag = parseArgs(argc, argv);
    IOScheduler *scheduler;

    switch (sched_flag) {
        case 'i':
            scheduler = new FIFO();
            break;
        case 'j':
            scheduler = new SSTF();
            break;
        case 's':
            scheduler = new LOOK();
            break;
        case 'c':
            scheduler = new CLOOK();
            break;
        case 'f':
            scheduler = new FLOOK();
            break;
        default:
            cout << "Invalid scheduler option.";
            exit(1);
    }

    IOSimulator simulator = initializeSimulator();
    simulator.simulate(scheduler);
}