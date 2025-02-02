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
#include <cstdlib>  // For system()

void executeCommand(const std::vector<std::string>& args) {
    // Check if the command is "clear"
    if (args.size() == 1 && args[0] == "clear") {
        system("clear");  // Clear the terminal
        return;
    }

    pid_t pid = fork();

    if (pid == 0) {  // Child process
        std::vector<const char*> cargs;
        for (const auto& arg : args) {
            cargs.push_back(arg.c_str());
        }
        cargs.push_back(NULL);  // Null terminate the array

        execvp(cargs[0], const_cast<char* const*>(cargs.data()));
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

void parseInput(const std::string& input) {
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
            // For now, we just break
            break;
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

    // Scan for executables in /usr/bin
    std::vector<std::string> executables = scanExecutables(binDir);

    while (true) {
        std::cout << "pyro >   ";
        std::getline(std::cin, input);

        if (input == "exit") {
            break;  // Exit the shell
        }

        parseInput(input);
    }

    return 0;
}