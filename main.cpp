#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <readline/readline.h>
#include <readline/history.h>

namespace fs = std::filesystem;

// Custom completion generator
char* completionGenerator(const char* text, int state) {
    static std::vector<std::string> matches;
    static size_t match_index = 0;

    if (state == 0) {  // Initialize completion list
        matches.clear();
        std::string prefix(text);

        // Try command completion first
        if (prefix.empty() || prefix.find('/') == std::string::npos) {
            // Get commands from PATH
            std::string path = getenv("PATH");
            std::string delimiter = ":";
            size_t pos = 0;

            while ((pos = path.find(delimiter)) != std::string::npos) {
                std::string dir = path.substr(0, pos);
                try {
                    for (const auto& entry : fs::directory_iterator(dir)) {
                        std::string name = entry.path().filename().string();
                        if (name.find(prefix) == 0 && static_cast<bool>(entry.status().permissions() & fs::perms::owner_exec)) {
                            matches.push_back(name + " ");
                        }
                    }
                } catch (...) {}
                path.erase(0, pos + delimiter.length());
            }
        }

        // File/directory completion
        try {
            fs::path complete_path = prefix.empty() ? fs::current_path() : fs::path(prefix);
            if (complete_path.has_parent_path() && fs::is_directory(complete_path.parent_path())) {
                std::string parent = complete_path.parent_path();
                std::string stem = complete_path.filename().string();

                for (const auto& entry : fs::directory_iterator(parent)) {
                    std::string name = entry.path().filename().string();
                    if (name.find(stem) == 0) {
                        std::string full_path = (fs::path(parent) / name).string();
                        if (fs::is_directory(entry.status())) full_path += "/";
                        matches.push_back(full_path);
                    }
                }
            }
        } catch (...) {}
    }

    if (match_index >= matches.size()) {
        return nullptr;
    } else {
        return strdup(matches[match_index++].c_str());
    }
}

// Set up completion function
char** completionFunction(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, completionGenerator);
}

int main() {
    // Initialize readline
    rl_attempted_completion_function = completionFunction;
    rl_bind_key('\t', rl_complete);

    // Load history
    using_history();
    read_history(".pyrosh_history");

    while (true) {
        char* input = readline("PyroShell$ ");
        if (!input) break;  // Ctrl+D

        std::string cmd(input);
        free(input);

        if (cmd.empty()) continue;

        // Add to history
        add_history(cmd.c_str());
        write_history(".pyrosh_history");

        if (cmd == "exit") break;

        // Process command here
        std::cout << "You entered: " << cmd << std::endl;
    }

    return 0;
}