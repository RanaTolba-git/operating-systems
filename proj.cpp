#include <iostream>
#include <string>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

void copy_directory(const fs::path& source, const fs::path& destination) {
    try {
        if (!fs::exists(source) || !fs::is_directory(source)) {
            std::cerr << "Source directory " << source.string() << " does not exist or is not a directory." << std::endl;
            return;
        }

        if (fs::exists(destination)) {
            std::cerr << "Destination directory " << destination.string() << " already exists." << std::endl;
            return;
        }

        // Create the destination directory
        if (fs::create_directory(destination)) {
            std::cout << "Created directory: " << destination.string() << std::endl;
        }

        // Iterate through the source directory
        for (fs::directory_iterator file(source); file != fs::directory_iterator(); ++file) {
            fs::path current(file->path());
            fs::path dest = destination / current.filename();
            if (fs::is_directory(current)) {
                // Recursively call copy_directory
                copy_directory(current, dest);
            } else {
                // Copy the file with the proper overload
                fs::copy_file(current, dest, fs::copy_option::overwrite_if_exists);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void create_snapshot(const std::string& source_dir, const std::string& snapshot_dir) {
    fs::path source(source_dir);
    fs::path destination(snapshot_dir);
    copy_directory(source, destination);
}

void restore_snapshot(const std::string& snapshot_dir, const std::string& target_dir) {
    fs::path snapshot(snapshot_dir);
    fs::path target(target_dir);
    copy_directory(snapshot, target);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <operation> <source> <destination>" << std::endl;
        std::cerr << "Operations: snapshot, restore" << std::endl;
        return 1;
    }

    std::string operation = argv[1];
    std::string source = argv[2];
    std::string destination = argv[3];

    if (operation == "snapshot") {
        create_snapshot(source, destination);
    } else if (operation == "restore") {
        restore_snapshot(source, destination);
    } else {
        std::cerr << "Unknown operation: " << operation << std::endl;
        return 1;
    }

    return 0;
}
