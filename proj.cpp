#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_set>
#include <vector>
#include <atomic>

namespace fs = std::filesystem;
using namespace std;
atomic<bool> stop_monitoring(false);

void copy_directory(const fs::path& source, const fs::path& destination) {
    try {
        if (!fs::exists(source) || !fs::is_directory(source)) {
            cerr << "Source directory " << source.string() << " does not exist or is not a directory." << endl;
            return;
        }

        if (fs::exists(destination)) {
            fs::remove_all(destination);
        }

        if (fs::create_directory(destination)) {
            cout << "Created directory: " << destination.string() << endl;
        }

        for (const auto& entry : fs::directory_iterator(source)) {
            const auto& current = entry.path();
            auto dest = destination / current.filename();
            if (fs::is_directory(current)) {
                copy_directory(current, dest);
            } else {
               
                fs::copy_file(current, dest, fs::copy_options::overwrite_existing);
            }
        }
    } catch (const fs::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << endl;
    } catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }
}


void create_snapshot(const string& source_dir, const string& snapshot_dir) {
    fs::path source(source_dir);
    fs::path destination(snapshot_dir);
    
    if (!fs::exists(destination)) {
        fs::create_directories(destination);
    }

    vector<fs::path> snapshot_files;
    for (const auto& entry : fs::recursive_directory_iterator(destination)) {
        snapshot_files.push_back(entry.path());
    }

    
    for (const auto& snapshot_file : snapshot_files) {
        auto relative_path = fs::relative(snapshot_file, destination);
        auto source_file = source / relative_path;

        if (!fs::exists(source_file)) {
           
            fs::remove(snapshot_file);
            cout << "Deleted from snapshot: " << snapshot_file << endl;
        }
    }

    
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
        const auto& source_file = entry.path();
        auto relative_path = fs::relative(source_file, source);
        auto snapshot_file = destination / relative_path;

        if (fs::is_directory(source_file)) {
           
            if (!fs::exists(snapshot_file)) {
                fs::create_directories(snapshot_file);
            }
        } else {
            
            fs::copy_file(source_file, snapshot_file, fs::copy_options::update_existing | fs::copy_options::recursive);
            cout << "Copied or updated in snapshot: " << source_file << endl;
        }
    }
}


void restore_snapshot(const string& snapshot_dir, const string& target_dir) {
    fs::path snapshot(snapshot_dir);
    fs::path target(target_dir);

    
    for (const auto& entry : fs::recursive_directory_iterator(snapshot)) {
        const auto& snapshot_file = entry.path();
        auto relative_path = fs::relative(snapshot_file, snapshot);
        auto target_file = target / relative_path;

        if (!fs::exists(target_file)) {
            fs::create_directories(target_file.parent_path());
            fs::copy_file(snapshot_file, target_file, fs::copy_options::overwrite_existing);
            cout << "Restored file: " << target_file << endl;
        }
    }

    for (const auto& entry : fs::recursive_directory_iterator(target)) {
        const auto& target_file = entry.path();
        auto relative_path = fs::relative(target_file, target);
        auto snapshot_file = snapshot / relative_path;

        if (!fs::exists(snapshot_file)) {
            fs::remove(target_file);
            cout << "Deleted file: " << target_file << endl;
        }
    }
}

void monitor_and_snapshot(const string& source_dir, const string& snapshot_dir, int interval_seconds) {
    unordered_set<string> last_snapshot_files;

    create_snapshot(source_dir, snapshot_dir);
    for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
        last_snapshot_files.insert(entry.path().string());
    }

    while (!stop_monitoring) {
       
        this_thread::sleep_for(chrono::seconds(interval_seconds));

        unordered_set<string> current_files;
        for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
            current_files.insert(entry.path().string());
        }

        bool changes_detected = false;
        for (const auto& file : current_files) {
            if (last_snapshot_files.find(file) == last_snapshot_files.end()) {
                changes_detected = true;
                break;
            }
        }

        if (!changes_detected) {
            for (const auto& file : last_snapshot_files) {
                if (current_files.find(file) == current_files.end()) {
                    changes_detected = true;
                    break;
                }
            }
        }

        if (changes_detected) {
            
            create_snapshot(source_dir, snapshot_dir);
            cout << "Snapshot updated." << endl;

           
            last_snapshot_files.clear();
            for (const auto& file : current_files) {
                last_snapshot_files.insert(file);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cerr << "Usage: " << argv[0] << " <operation> <source> <snapshot_destination> <interval_seconds>" << endl;
        cerr << "Operations: snapshot, restore" << endl;
        return 1;
    }

    string operation = argv[1];
    string source = argv[2];
    string destination = argv[3];
    int interval_seconds = stoi(argv[4]);

    if (operation == "snapshot") {
        thread monitor_thread(monitor_and_snapshot, source, destination, interval_seconds);
        monitor_thread.detach(); 

        cout << "Press 'q' to quit." << endl;
        while (true) {
            if (cin.get() == 'q') {
                stop_monitoring = true;
                break;
            }
        }
    } else if (operation == "restore") {
        restore_snapshot(source, destination);
    } else {
        cerr << "Unknown operation: " << operation << endl;
        return 1;
    }

    return 0;
}
