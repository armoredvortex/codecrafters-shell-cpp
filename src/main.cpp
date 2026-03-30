#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <termios.h>
#include <unistd.h>

namespace fs = std::filesystem;

int c_exit(std::string args);
int c_echo(std::string args);
int c_type(std::string args);
int c_pwd(std::string args);
int c_cd(std::string args);

// builtins store
std::map<std::string, std::function<int(std::string)>> builtins = {
    {"echo", c_echo},
    {"exit", c_exit},
    {"type", c_type},
    {"pwd", c_pwd},
    {"cd", c_cd}};

// utils
void set_raw_mode(struct termios &original) {
  struct termios raw = original;
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

bool isValidCommand(std::string command) {
  return (builtins.find(command) != builtins.end());
}

bool isExecutable(const std::string &path_str) {
  fs::path p(path_str);
  std::error_code ec;
  fs::file_status s = fs::status(p, ec);

  if (ec) {
    return false;
  }

  // Check if the file has execute permissions for the owner, group, or others
  fs::perms perms = s.permissions();

  bool executable = ((perms & fs::perms::owner_exec) != fs::perms::none) ||
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
    if (isExecutable(token + '/' + args)) {
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

int c_pwd(std::string arg) {
  std::cout << fs::current_path().c_str() << '\n';
  return 0;
}

int c_cd(std::string arg) {
  if (arg.size() == 0) {
    fs::current_path(std::getenv("HOME"));
  } else {

    std::string home_path = std::getenv("HOME");
    size_t pos = arg.find('~');
    while (pos != std::string::npos) {
      arg.replace(pos, 1, home_path);
      pos = arg.find("~", pos + home_path.size());
    }

    fs::path p(arg);
    fs::file_status s = fs::status(p);

    if (!fs::exists(s)) {
      std::cout << "cd: " << arg << ": No such file or directory\n";
      return 1;
    }

    fs::current_path(arg);
  }

  return 0;
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  struct termios original_termios;
  tcgetattr(STDIN_FILENO, &original_termios);
  while (true) {
    std::cout << "$ ";
    std::string command;
    // std::getline(std::cin, command);

    char ch;

    while (true) {
      set_raw_mode(original_termios);
      if (read(STDIN_FILENO, &ch, 1) <= 0)
        break;
      tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);

      if (ch == '\n') {
        std::cout << '\n' << std::flush;
        break;
      } else if (ch == '\t') {
        bool flag = false;
        for(auto e: builtins){
          if(e.first.substr(0, command.size()) == command){
            std::string autocomplete = e.first.substr(command.size(), e.first.size() - command.size()) + ' ';
            std::cout << autocomplete;
            command += autocomplete;
            flag = true;
          }
        }

        if(!flag){
          std::cout << '\a';
        }

      } else if (ch == 127){
        if(command.size()){
          command.pop_back();
          std::cout << "\b \b";
        }
      }else {
        command += ch;
        std::cout << ch << std::flush;
      }
    }

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
    } else if (findOnPath(cmd).size()) {
      std::system(command.c_str());
    } else {
      std::cout << command << ": command not found\n";
    }
  }
}