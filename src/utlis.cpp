#include "builtins.hpp"
#include "utils.hpp"
#include <cstddef>
#include <filesystem>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

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
    if (arg.find_last_of('/') != std::string::npos) {
      token = arg.substr(arg.find_last_of('/') + 1);
    }

    if (bin.substr(0, token.size()) == token &&
        builtins.find(bin) == builtins.end()) {

      if (fs::is_directory(entry.path())) {
        bin += '/';
      }
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