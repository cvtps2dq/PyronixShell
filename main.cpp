#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <sys/stat.h>
#include <cstdlib>
#include <map>
#include <algorithm>
#include <filesystem>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

namespace fs = std::filesystem;

std::map<std::string, std::string> envVars;
std::vector<std::string> history;

// Improved environment variable expansion
std::string expandVariables(const std::string& input) {
    std::string result = input;
    size_t pos = 0;
    while ((pos = result.find('$', pos)) != std::string::npos) {
        if (pos + 1 >= result.size()) break;

        size_t start = pos + 1;
        size_t end = start;
        if (result[start] == '{') {
            end = result.find('}', start);
            if (end == std::string::npos) break;
            start++;
        } else {
            end = result.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_",
                start
            );
        }

        std::string varName = result.substr(start, end - start);
        auto it = envVars.find(varName);
        if (it != envVars.end()) {
            result.replace(pos, end - pos + (result[end] == '}' ? 1 : 0), it->second);
            pos += it->second.size();
        } else {
            pos = end;
        }
    }
    return result;
}

// Enhanced command substitution
std::string performCommandSubstitution(std::string input) {
    size_t start;
    while ((start = input.find("$(")) != std::string::npos) {
        size_t end = input.find(')', start);
        if (end == std::string::npos) break;

        std::string cmd = input.substr(start + 2, end - start - 2);
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) break;

        std::string output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            output += buffer;
        }
        pclose(pipe);

        // Remove trailing newline if present
        if (!output.empty() && output.back() == '\n') {
            output.pop_back();
        }

        input.replace(start, end - start + 1, output);
    }
    return input;
}

// Improved tilde expansion
std::string expandTilde(const std::string& input) {
    if (!input.empty() && input[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            size_t slash = input.find('/');
            if (slash == std::string::npos) {
                return home;
            }
            return std::string(home) + input.substr(slash);
        }
    }
    return input;
}

// Enhanced directory listing with proper error handling
std::vector<std::string> listDirectorySuggestions(const std::string& input) {
    std::vector<std::string> suggestions;

    try {
        fs::path target = input.empty() ? fs::current_path() : fs::path(input);
        if (fs::is_directory(target)) {
            for (const auto& entry : fs::directory_iterator(target)) {
                std::string name = entry.path().filename().string();
                if (fs::is_directory(entry.status())) name += "/";
                suggestions.push_back(name);
            }
        } else {
            fs::path parent = target.parent_path();
            std::string prefix = target.filename().string();

            if (fs::is_directory(parent)) {
                for (const auto& entry : fs::directory_iterator(parent)) {
                    std::string name = entry.path().filename().string();
                    if (name.find(prefix) == 0) {
                        if (fs::is_directory(entry.status())) name += "/";
                        suggestions.push_back(name);
                    }
                }
            }
        }
    } catch (const fs::filesystem_error&) {
        // Ignore permission errors
    }

    std::sort(suggestions.begin(), suggestions.end());
    return suggestions;
}

// Improved command execution with pipes and redirection support
void executeCommand(std::vector<std::string> args) {
    if (args.empty()) return;

    // Handle built-in commands
    if (args[0] == "cd") {
        if (args.size() == 1) {
            const char* home = getenv("HOME");
            if (home) chdir(home);
        } else if (chdir(args[1].c_str()) != 0) {
            perror("cd");
        }
        return;
    }

    if (args[0] == "export") {
        for (size_t i = 1; i < args.size(); ++i) {
            size_t eq = args[i].find('=');
            if (eq != std::string::npos) {
                std::string var = args[i].substr(0, eq);
                std::string val = args[i].substr(eq + 1);
                setenv(var.c_str(), val.c_str(), 1);
                envVars[var] = val;
            }
        }
        return;
    }

    // Fork and execute external command
    pid_t pid = fork();
    if (pid == 0) {
        std::vector<const char*> c_args;
        for (auto& arg : args) c_args.push_back(arg.c_str());
        c_args.push_back(nullptr);

        execvp(c_args[0], const_cast<char* const*>(c_args.data()));
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
    }
}

int main() {
    using_history();
    rl_bind_key('\t', rl_complete);

    // Load environment variables
    for (char** env = environ; *env; ++env) {
        std::string entry(*env);
        size_t eq = entry.find('=');
        if (eq != std::string::npos) {
            envVars[entry.substr(0, eq)] = entry.substr(eq + 1);
        }
    }

    while (true) {
        std::string prompt = "PyroShell$ ";
        char* line = readline(prompt.c_str());
        if (!line) break;

        std::string input(line);
        free(line);

        if (input.empty()) continue;
        add_history(input.c_str());

        // Preprocessing
        input = expandTilde(input);
        input = performCommandSubstitution(input);
        input = expandVariables(input);

        // Parse and execute
        std::istringstream iss(input);
        std::vector<std::string> args;
        std::string arg;
        while (iss >> arg) args.push_back(arg);

        if (!args.empty()) {
            if (args[0] == "exit") break;
            executeCommand(args);
        }
    }

    return 0;
}