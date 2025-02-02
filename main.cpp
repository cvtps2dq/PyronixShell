#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>
#include <map>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <sys/stat.h>
#include <cstdlib>

namespace fs = std::filesystem;

// Global variable map for variable storage
std::map<std::string, std::string> envVars;
std::vector<std::string> history;  // Store command history
size_t historyIndex = 0;          // Current position in history

void loadEnvironmentVariables() {
    extern char** environ;  // Pointer to environment variables
    for (char** env = environ; *env != nullptr; ++env) {
        std::string envEntry = *env;
        size_t pos = envEntry.find('=');
        if (pos != std::string::npos) {
            std::string varName = envEntry.substr(0, pos);
            std::string varValue = envEntry.substr(pos + 1);
            envVars[varName] = varValue;
        }
    }
}

// Function to expand variables (e.g., $VAR -> value)
std::string expandVariables(const std::string& input) {
    std::string result = input;
    size_t pos = 0;
    while ((pos = result.find("$", pos)) != std::string::npos) {
        size_t endPos = result.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", pos + 1);
        std::string varName = result.substr(pos + 1, endPos - pos - 1);
        if (envVars.find(varName) != envVars.end()) {
            result.replace(pos, endPos - pos, envVars[varName]);
        } else {
            pos = endPos;  // Move past the variable name
        }
    }
    return result;
}

// Function to perform command substitution $(command)
std::string performCommandSubstitution(const std::string& input) {
    size_t start = input.find('$');
    if (start == std::string::npos) return input;

    size_t end = input.find(')', start);
    if (end != std::string::npos && input[start + 1] == '(') {
        std::string command = input.substr(start + 2, end - start - 2);
        FILE* fp = popen(command.c_str(), "r");
        if (!fp) {
            std::cerr << "Error executing command substitution" << std::endl;
            return input;
        }
        char buffer[256];
        std::string output;
        while (fgets(buffer, sizeof(buffer), fp)) {
            output += buffer;
        }
        fclose(fp);
        return input.substr(0, start) + output + input.substr(end + 1);
    }
    return input;
}

// Function to expand tilde (~) to the home directory
std::string expandTilde(const std::string& input) {
    if (input[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            return std::string(home) + input.substr(1);
        }
    }
    return input;
}

// Function to execute commands, passing environment variables to the child process
void executeCommand(std::vector<std::string>& args) {
    // Expand variables in each argument
    for (auto& arg : args) {
        arg = expandVariables(arg);
    }

    // Check if the command is "clear"
    if (args.size() == 1 && args[0] == "clear") {
        system("clear");
        return;
    }

    // Check if the command is "cd"
    if (args.size() == 1 && args[0] == "cd") {
        const char* homeDir = getenv("HOME");
        if (homeDir) {
            chdir(homeDir);  // Change to the home directory
        } else {
            std::cerr << "Error: HOME directory not set!" << std::endl;
        }
        return;
    }

    if (args.size() > 1 && args[0] == "cd") {
        if (chdir(args[1].c_str()) != 0) {
            perror("cd");
        }
        return;
    }

    pid_t pid = fork();

    if (pid == 0) {  // Child process
        std::vector<const char*> cargs;
        for (const auto& arg : args) {
            cargs.push_back(arg.c_str());
        }
        cargs.push_back(NULL);  // Null terminate the array

        // Pass the environment variables to the child process
        std::vector<char*> envp;
        for (const auto& [varName, varValue] : envVars) {
            std::string envVar = varName + "=" + varValue;
            envp.push_back(const_cast<char*>(envVar.c_str()));
        }
        envp.push_back(NULL);  // Null terminate the environment array

        execvp(cargs[0], const_cast<char* const*>(cargs.data()));
        execve(cargs[0], const_cast<char* const*>(cargs.data()), envp.data());
        std::cerr << "Command execution failed: " << args[0] << std::endl;
        exit(1);
    } else if (pid > 0) {  // Parent process
        int status;
        waitpid(pid, &status, 0);  // Wait for the child process to finish
    } else {
        std::cerr << "Fork failed!" << std::endl;
    }
}

// Function to list directories and files for autocompletion
std::vector<std::string> listDirectorySuggestions(const std::string& input) {
    std::vector<std::string> suggestions;
    std::string path = input;
    if (input.back() == '/') {
        path = input.substr(0, input.size() - 1);  // Remove trailing slash
    }

    fs::path dirPath(path);
    if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            std::string entryName = entry.path().filename().string();
            if (entryName.find(path.substr(path.find_last_of('/') + 1)) == 0) {
                suggestions.push_back(entryName);
            }
        }
    }
    return suggestions;
}

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

void parseInput(std::string input) {
    input = expandTilde(input);  // Expand ~ to home directory
    input = performCommandSubstitution(input);  // Perform $(command) substitution

    std::istringstream stream(input);
    std::string token;
    std::vector<std::string> command;
    bool lastCommandSuccess = true;

    while (stream >> token) {
        if (token == "&&") {
            if (!lastCommandSuccess) {
                break;  // Skip the next command if the previous one failed
            }
            continue;
        } else if (token == "||") {
            if (lastCommandSuccess) {
                break;  // Skip the next command if the previous one succeeded
            }
            continue;
        } else if (token == "|") {
            // Handle pipe (this will require more complex implementation with pipe() system call)
            break;
        } else if (token.find('=') != std::string::npos) {  // Variable assignment (e.g., VAR=value)
            size_t pos = token.find('=');
            std::string varName = token.substr(0, pos);
            std::string varValue = token.substr(pos + 1);
            envVars[varName] = varValue;
            continue;
        }

        command.push_back(token);
    }

    if (!command.empty()) {
        executeCommand(command);
    }
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
        parseInput(cmd);
    }

    return 0;
}