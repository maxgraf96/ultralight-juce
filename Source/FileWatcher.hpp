#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <future>
#include <thread>
#include <unordered_map>

class FileWatcher {
public:
    using Callback = std::function<void(const std::string&)>;

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

//    void AddCallback(const std::string& filename, Callback callback) {
//        callbacks_[filename] = callback;
//    }

    void AddCallback(const std::string& filename, std::function<void(const std::string&)> callback) {
        callbacks_[filename] = std::move(callback);
    }

    void RemoveCallback(const std::string& filename) {
        callbacks_.erase(filename);
    }

private:
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

    void NotifyFileChanged(const std::string& filename) {
        for (const auto& callback : callbacks_) {
            if (filename.find(callback.first) != std::string::npos) {
                callback.second(filename);
            }
        }
    }

    std::string path_;
    std::chrono::duration<int, std::milli> delay_;
    std::unordered_map<std::string, Callback> callbacks_;

    std::thread thread_;
    std::atomic<bool> running_;
};