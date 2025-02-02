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

namespace fs = std::filesystem;

// Global variable map for variable storage
std::map<std::string, std::string> envVars;

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

// Function to handle command substitution $(command)
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

// Function to load environment variables into the envVars map
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

bool isExecutable(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) == 0) {
        return (statbuf.st_mode & S_IXUSR) != 0;  // Check if it's executable by the user
    }
    return false;
}

std::vector<std::string> scanExecutables(const std::string& dirPath) {
    std::vector<std::string> executables;
    DIR* dir = opendir(dirPath.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename != "." && filename != "..") {
                std::string filePath = dirPath + "/" + filename;
                if (isExecutable(filePath)) {
                    executables.push_back(filename);
                }
            }
        }
        closedir(dir);
    }
    return executables;
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
    std::string input;
    const std::string binDir = "/usr/bin";

    // Load system environment variables into the shell's environment
    loadEnvironmentVariables();

    while (true) {
        std::cout << "PyroShell$ ";
        std::getline(std::cin, input);

        if (input == "exit") {
            break;  // Exit the shell
        }

        parseInput(input);
    }

    return 0;
}