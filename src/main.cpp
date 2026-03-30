#include <algorithm>
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
#include <vector>

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

void matchOnPath(std::string args, std::vector<std::string> &completions) {
  std::string path = std::getenv("PATH");

  std::string token;
  std::stringstream ss(path);

  while (std::getline(ss, token, ':')) {
    if (fs::exists(token)) {
      for (const auto &entry : fs::directory_iterator(token)) {
        // if(args == entry.path().string().sub)

        if (!isExecutable(entry.path())) {
          continue;
        }

        size_t lastPos = entry.path().string().rfind('/');
        std::string bin = entry.path().string().substr(lastPos + 1);

        if (bin.substr(0, args.size()) == args &&
            builtins.find(bin) == builtins.end()) {
          completions.push_back(bin);
        }
      }
    }
  }
}

void matchFilename(std::string arg, std::vector<std::string> &completions) {

  std::string subdir;
  if (arg.find_last_of('/') != std::string::npos) {
    subdir = arg.substr(0, arg.find_last_of('/'));
  }

  for (const auto &entry :
       fs::directory_iterator(fs::current_path().string() + '/' + subdir)) {

    size_t lastPos = entry.path().string().rfind('/');
    std::string bin = entry.path().string().substr(lastPos + 1);
    
    std::string token = arg;
    if(arg.find_last_of('/') != std::string::npos){
      token = arg.substr(arg.find_last_of('/')+1);
    }

    if (bin.substr(0, token.size()) == token &&
        builtins.find(bin) == builtins.end()) {
      completions.push_back(bin);
    }
  }
}

std::string longestCommonPrefix(std::vector<std::string> &strs) {
  if (strs.empty()) {
    return "";
  }

  // Iterate through each character position of the first string
  for (int i = 0; i < strs[0].length(); ++i) {
    char currentChar = strs[0][i];

    // Compare this character across all other strings
    for (int j = 1; j < strs.size(); ++j) {
      // If the current string is too short or the character doesn't match
      if (i == strs[j].length() || strs[j][i] != currentChar) {
        // Return the common prefix found so far using substr
        return strs[0].substr(0, i);
      }
    }
  }

  // If the entire first string is a common prefix for all strings
  return strs[0];
}

std::string findOnPath(std::string args) {
  std::string path = std::getenv("PATH");

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
  std::cout << fs::current_path().string() << '\n';
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
    bool dtab_flag = false;
    while (true) {
      if (!dtab_flag) {
        set_raw_mode(original_termios);
        if (read(STDIN_FILENO, &ch, 1) <= 0)
          break;
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
      }
      dtab_flag = false;

      if (ch == '\n') {
        std::cout << '\n' << std::flush;
        break;
      } else if (ch == '\t') {
        bool searchForBuiltins = true;
        bool searchOnPath = true;
        bool filenameCompletion = true;

        std::string lastToken = command.substr(command.find_last_of(' ') + 1);

        if (command.find(' ') != command.npos) {
          searchForBuiltins = false;
          searchOnPath = false;
        }

        std::vector<std::string> possible_completions;

        if (searchForBuiltins) {
          for (auto e : builtins) {
            if (e.first.substr(0, command.size()) == command) {
              possible_completions.push_back(e.first);
            }
          }
        }

        if (searchOnPath) {
          matchOnPath(lastToken, possible_completions);
        }

        matchFilename(lastToken, possible_completions);

        std::sort(possible_completions.begin(), possible_completions.end());

        if (!possible_completions.size()) {
          std::cout << '\a';
        } else if (possible_completions.size() == 1) {
          if(command.find_last_of('/') != std::string::npos){
            lastToken = command.substr(command.find_last_of('/')+1);
          }
          std::string autocomplete =
              possible_completions[0].substr(lastToken.size());

          std::cout << autocomplete << ' ';
          command += autocomplete + ' ';
        } else {

          std::string lcp = longestCommonPrefix(possible_completions);

          if (lcp.size() > command.size()) {
            std::cout << lcp.substr(command.size());
            command = lcp;
          }

          std::cout << '\a';
          set_raw_mode(original_termios);
          if (read(STDIN_FILENO, &ch, 1) <= 0)
            break;
          tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);

          if (ch == '\t') {
            std::cout << '\n';
            for (auto e : possible_completions) {
              std::cout << e << "  ";
            }
            std::cout << '\n';
            std::cout << "$ " << command;
          } else {
            dtab_flag = true;
          }
        }

      } else if (ch == 127) {
        if (command.size()) {
          command.pop_back();
          std::cout << "\b \b";
        }
      } else {
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