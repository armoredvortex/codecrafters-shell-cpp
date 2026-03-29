#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

int c_exit(std::string args);
int c_echo(std::string args);
int c_type(std::string args);
int c_pwd(std::string args);

// builtins store
std::map<std::string, std::function<int(std::string)>> builtins = {
    {"echo", c_echo}, {"exit", c_exit}, {"type", c_type}};

// utils
bool isValidCommand(std::string command) {
  return (builtins.find(command) != builtins.end());
}

bool isExecutable(const std::string& path_str) {
    fs::path p(path_str);
    std::error_code ec;
    fs::file_status s = fs::status(p, ec);

    if (ec) {
      return false;
    }

    // Check if the file has execute permissions for the owner, group, or others
    fs::perms perms = s.permissions();
    
    bool executable = 
        ((perms & fs::perms::owner_exec) != fs::perms::none) ||
        ((perms & fs::perms::group_exec) != fs::perms::none) ||
        ((perms & fs::perms::others_exec) != fs::perms::none);

    // Note: On Windows, the concept of an "executable permission" doesn't exist 
    // in the same way as Unix. The library might check if it's a file type 
    // that the OS considers runnable (e.g., .exe, .bat, .cmd).
    
    return executable;
}

std::string findOnPath(std::string args) {
  std::string path = std::getenv("PATH");
  // std::cerr << "PATH: " << path << '\n';

  std::string token;
  std::stringstream ss(path);

  while (std::getline(ss, token, ':')) {
    if(isExecutable(token + '/' + args)){
      return token + '/' + args + '\n';
    }
  }

  return "";
}

// builtins
int c_type(std::string args) {
  if (builtins.find(args) != builtins.end()) {
    std::cout << args << " is a shell builtin\n";
  } else if (findOnPath(args).size() != 0) {
    std::cout << args << " is " << findOnPath(args);
  } else {
    std::cout << args << ": not found\n";
  }
  return 0;
}
int c_exit(std::string args) { return -1; }

int c_echo(std::string args) {
  std::cout << args << '\n';
  return 0;
}

int c_pwd(std::string arg){
  std::cout << fs::current_path() << '\n';
  return 0;
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  while (true) {
    std::cout << "$ ";
    std::string command;
    std::getline(std::cin, command);

    std::string cmd = command.substr(0, command.find(' '));
    std::string args;
    if (command.find(' ') != std::string::npos) {
      args = command.substr(command.find(' ') + 1,
                            command.size() - command.find(' ') - 1);
    }

    if (isValidCommand(cmd)) {
      if (builtins[cmd](args) == -1) {
        return 0;
      };
    } else if(findOnPath(cmd).size()){
      std::system(command.c_str());
    } else {
      std::cout << command << ": command not found\n";
    }
  }
}