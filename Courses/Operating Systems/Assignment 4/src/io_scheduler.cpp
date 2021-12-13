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
    int arr_time, start_time{}, end_time{}, track, id{};

    Request(int arr_time, int track)
            : arr_time(arr_time), track(track) {};
};

typedef deque<Request *> requests_t;


class IOScheduler {
public:
    int curr_track = 0, scan_incr = 1, total_movement = 0, total_turnaround = 0, total_wait = 0, max_wait = 0;
    requests_t io_queue, disk;
    requests_t::iterator head = disk.begin();

    int disk_dist_by_id(int id) {
        auto disk_it = disk.begin();
        for (; disk_it != disk.end(); disk_it++)
            if ((*disk_it)->id == id)
                break;

        if (disk_it == disk.end()) {
            cout << "Invalid Distance: ID not found on disk.";
            exit(1);
        }

        return distance(head, disk_it);
    }

    void set_head_to_id(requests_t *active_disk, int id) {
        // Adjust head after new mapping.
        auto disk_it = active_disk->begin();
        for (; disk_it < active_disk->end(); disk_it++) {
            if ((*disk_it)->id == id)
                break;
        }
        head = disk_it;
    }

    void map_request_to_disk(Request *request, requests_t &active_disk) {
        /*
         * Map new request on disk in order of tracks.
         */
        if (active_disk.empty()) {
            active_disk.push_back(request);
        } else {
            auto dq_it = active_disk.begin();

            // In the event of head being on another disk (FLOOK).
            Request *curr_request = nullptr;
            if (head >= active_disk.begin() && head < active_disk.end())
                curr_request = *head;

            for (; dq_it != active_disk.end(); dq_it++)
                if ((*dq_it)->track > request->track)
                    break;

            if (dq_it == active_disk.end())
                active_disk.push_back(request);

            else active_disk.insert(dq_it, request);

            // Adjust head after new mapping.
            if (curr_request != nullptr)
                set_head_to_id(&active_disk, curr_request->id);
        }

        if (DEBUG) {
            cout << "Inserting IO ID: " << request->id << endl;
        }
    }

    virtual void put_io(Request *request) {
        /*
         * Queue and map to disk, the incoming IO request.
         */
        io_queue.push_back(request);

        map_request_to_disk(request, disk);
    };

    virtual Request *get_io(int &time) {
        if (disk.empty())
            return nullptr;

        if (DEBUG) {
            if (head >= disk.begin() && head < disk.end()) {
                cout << "Current head ID: " << (*head)->id << endl;
            }
        }

        // Clear the previous IO request and move head to the next one.
        auto seek_target = head + scan_incr;

        if (DEBUG) {
            cout << "IO ID, Arrival Time, Track" << endl;
            for (auto io: disk) {
                cout << io->id << ", " << io->arr_time << ", " << io->track << endl;
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

        // If we're performing operations on the same track as before, time doesn't increment.
        if (movement == 0)
            time--;

        io->end_time = io->start_time + movement;

        // Update metrics
        total_movement += movement;
        total_turnaround += io->end_time - io->arr_time;
        total_wait += wait_time;
        max_wait = wait_time > max_wait ? wait_time : max_wait;

        // Change track
        curr_track = io->track;

        // Remove previous IO request from disk and remap head to the current one.
        if (!(head == seek_target)) {
            disk.erase(head);
            set_head_to_id(&disk, io->id);
        }

        return io;
    }

    Request *peek_disk(int incr) {
        /*
         * Peek the next targeted track in specified direction and return the request for it.
         */
        auto peek_head = head + incr;

        // Check for multiple overlapping io requests on the same track when moving backwards.
        if (incr < 0) {
            auto precision_peek = peek_head;
            while (true) {
                precision_peek--;
                if ((precision_peek < disk.begin()) || ((*precision_peek)->track != (*peek_head)->track)) {
                    precision_peek++;
                    break;
                }
            }
            peek_head = precision_peek;
        }


        // Reached end of disk.
        if (peek_head < disk.begin() || peek_head >= disk.end())
            return nullptr;

        return *peek_head;
    }
};

class FIFO : public IOScheduler {
    int q_idx = 0;
public:
    Request *get_io(int &time) override {
        if (q_idx >= io_queue.size())
            return nullptr;

        // Find the target track on disk.
        Request *next_io = io_queue[q_idx];
        q_idx++;
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

        return IOScheduler::get_io(time);
    }
};

class SSTF : public IOScheduler {
public:
    Request *get_io(int &time) final {
        /*
         * Find the IO request in queue with the shortest seek time and return it.
         */
        if (disk.empty())
            return nullptr;

        if (!(head >= disk.begin() && head < disk.end())) {
            head = disk.begin();
            scan_incr = 0;
        } else {
            Request *up_req = peek_disk(-1), *down_req = peek_disk(1), *nearest_io;
            if (up_req == nullptr && down_req == nullptr) {
                if (DEBUG)
                    cout << "IO queue empty.";
                return nullptr;
            }

            // Determine the nearest track for IO.
            if (up_req == nullptr) {
                scan_incr = disk_dist_by_id(down_req->id);
            } else if (down_req == nullptr) {
                scan_incr = disk_dist_by_id(up_req->id);
            } else {
                int up_distance = abs(up_req->track - curr_track), down_distance = abs(down_req->track - curr_track);
                if (up_distance == down_distance) {
                    scan_incr = up_req->id < down_req->id ? disk_dist_by_id(up_req->id) : disk_dist_by_id(down_req->id);
                } else {
                    scan_incr =
                            up_distance < down_distance ? disk_dist_by_id(up_req->id) : disk_dist_by_id(down_req->id);
                }

            }
        }

        // Jump to the target track and get the IO request for it.
        return IOScheduler::get_io(time);
    }
};

class LOOK : public IOScheduler {
    bool io_overlap_reverse = false;
public:
    Request *get_io(int &time) override {
        if (disk.empty())
            return nullptr;

        // Start of IO scheduling, set head to seek the first track.
        if (!(head >= disk.begin() && head < disk.end())) {
            head = disk.begin();
            scan_incr = 0;
        }
            // Handle the case where we encountered multiple IOs on the same track by fixing previous large track jump.
        else if (scan_incr != 1 && scan_incr != -1)
            scan_incr = 1;

        Request *next_io = peek_disk(scan_incr);

        // Switch direction if no more IO requests in current direction.
        if (next_io == nullptr) {
            scan_incr *= -1;
            next_io = peek_disk(scan_incr);

            // No new io on this disk
            if (next_io == nullptr)
                return nullptr;
        }

        // Switch back to original scanning direction once we're done handling overlapping IOs in reverse.
        if (io_overlap_reverse && next_io->track != (*head)->track) {
            scan_incr = -1;
            next_io = peek_disk(scan_incr);
            if (next_io == nullptr) {
                scan_incr *= -1;
                next_io = peek_disk(scan_incr);

                // No new io on this disk
                if (next_io == nullptr)
                    return nullptr;
            }
            io_overlap_reverse = false;
        }

        // Update increment for multiple IOs on the same track.
        scan_incr = disk_dist_by_id(next_io->id);

        // Found overlapping IOs while going up the tracks. Enter reverse scanning to handle overlapping IOs.
        if (scan_incr < -1)
            io_overlap_reverse = true;

        return IOScheduler::get_io(time);
    }
};

class CLOOK : public LOOK {
public:
    Request *get_io(int &time) override {
        if (disk.empty())
            return nullptr;

        // If we're at the end of disk, set head to invalid track so that LOOK resets it to track 0;
        bool head_at_end = false;
        if (head == disk.end() - 1 && head != disk.begin()) {
            if (DEBUG) {
                if (head >= disk.begin() && head < disk.end()) {
                    cout << "Current head ID: " << (*head)->id << endl;
                }
                cout << "IO ID, Arrival Time, Track" << endl;
                for (auto io: disk) {
                    cout << io->id << ", " << io->arr_time << ", " << io->track << endl;
                }
                cout << endl;
            }
            head = disk.end();
            head_at_end = true;
        }

        // Lookup next IO on disk.
        Request *next_io = LOOK::get_io(time);

        // Remove the last IO at end of disk.
        if (head_at_end)
            disk.pop_back();

        return next_io;
    }
};

class FLOOK : public LOOK {
    requests_t add_disk;
public:
    FLOOK() { scan_incr = -1; }

    void put_io(Request *request) override {
        /*
         * Map new request to add disk, instead of active disk.
         */
        io_queue.push_back(request);
        map_request_to_disk(request, add_disk);
    }

    Request *get_io(int &time) override {
        // Once the active disk (queue) is empty, swap it with add disk (queue).
        if (disk.size() < 2) {
            if (add_disk.empty())
                return nullptr;

            // If head is previously positioned, we need to map it again onto the new disk mapping for LOOK scan to work.
            int head_id = -1;
            if (head >= disk.begin() && head < disk.end()) {
                Request *curr_io = *head;
                auto disk_it = add_disk.begin();
                for (; disk_it != add_disk.end(); disk_it++) {
                    if ((*disk_it)->track >= curr_io->track)
                        break;
                }

                if (disk_it != add_disk.end())
                    add_disk.insert(disk_it, curr_io);
                else add_disk.push_back(curr_io);

                head_id = curr_io->id;
            }

            // Map add_disk values to disk and remap head.
            disk = add_disk;
            if (head_id != -1)
                set_head_to_id(&disk, head_id);
            add_disk.clear();

        }

        return LOOK::get_io(time);

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

    void print_metrics(IOScheduler *scheduler) const {
        auto io_it = scheduler->io_queue.begin();
        Request *io = *io_it;
        for (; io_it != scheduler->io_queue.end(); io_it++) {
            io = *io_it;
            printf("%5d: %5d %5d %5d\n", io->id, io->arr_time, io->start_time, io->end_time);
        }
        printf("SUM: %d %d %.2lf %.2lf %d\n",
               time, scheduler->total_movement, (double) scheduler->total_turnaround / (double) io_idx,
               (double) scheduler->total_wait / (double) io_idx,
               scheduler->max_wait);
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

        print_metrics(scheduler);

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