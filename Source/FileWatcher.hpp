#pragma once

#include <chrono>                 // Header for time-related utilities
#include <filesystem>             // Header for file system operations
#include <functional>             // Header for function objects
#include <future>                 // Header for asynchronous operations
#include <thread>                 // Header for multithreading support
#include <unordered_map>          // Header for unordered map container

class FileWatcher {
public:
    using Callback = std::function<void(const std::string&)>;   // Type alias for callback function

    explicit FileWatcher(const std::string& path, std::chrono::duration<int, std::milli> delay = std::chrono::milliseconds(500))
            : path_(path), delay_(delay), running_(false) {}

    ~FileWatcher() {
        Stop();
    }

    void Start() {
        if (!running_) {
            running_ = true;
            thread_ = std::thread([this]() {
                WatchLoop();
            });
        }
    }

    void Stop() {
        if (running_) {
            running_ = false;
            thread_.join();
        }
    }

    // Add a callback function to be executed when a file is changed
    void AddCallback(const std::string& filename, std::function<void(const std::string&)> callback) {
        callbacks_[filename] = std::move(callback);
    }

    // Remove a callback function for a specific file
    void RemoveCallback(const std::string& filename) {
        callbacks_.erase(filename);
    }

private:
    // Continuously watches for file changes
    void WatchLoop() {
        std::unordered_map<std::string, std::filesystem::file_time_type> currentFiles;

        while (running_) {
            std::this_thread::sleep_for(delay_);

            std::unordered_map<std::string, std::filesystem::file_time_type> newFiles;
            for (const auto& entry : std::filesystem::directory_iterator(path_)) {
                if (entry.is_regular_file()) {
                    const auto& path = entry.path().string();
                    newFiles[path] = std::filesystem::last_write_time(entry);
                    if (currentFiles.find(path) == currentFiles.end() || currentFiles[path] != newFiles[path]) {
                        NotifyFileChanged(path);
                    }
                }
            }

            currentFiles = newFiles;
        }
    }

    // Notifies the registered callbacks about a file change
    void NotifyFileChanged(const std::string& filename) {
        for (const auto& callback : callbacks_) {
            if (filename.find(callback.first) != std::string::npos) {
                callback.second(filename);
            }
        }
    }

    std::string path_;                                      // The path to watch for file changes
    std::chrono::duration<int, std::milli> delay_;          // The delay between checks for file changes
    std::unordered_map<std::string, Callback> callbacks_;   // Mapping of filenames to callback functions

    std::thread thread_;                                    // The thread running the WatchLoop function
    std::atomic<bool> running_;                             // Flag indicating whether the watcher is running
};
