#include "builtins.hpp"
#include "utils.hpp"
#include <climits>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

std::vector<std::string> history_list;

int c_history(std::string args) {
  int n = INT_MAX;
  if (args.size())
    n = std::stoi(args);
  for (int i = std::max(0, (int)history_list.size() - n);
       i < history_list.size(); i++) {
    std::cout << i + 1 << "  " << history_list[i] << '\n';
  }
  return 0;
}

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

int c_jobs(std::string) {

  for (auto it = running_jobs.begin(); it != running_jobs.end(); ++it) {
    std::string status;
    int result = waitpid(it->second.first, nullptr, WNOHANG);
    if (result == 0) {
      status = "Running   ";
    } else {
      status = "Terminated";
    }

    std::cout << '[' << it->first << ']';
    if (std::next(it) == running_jobs.end()) {
      std::cout << '+';
    } else if (std::next(std::next(it)) == running_jobs.end()) {
      std::cout << '-';
    } else {
      std::cout << ' ';
    }
    std::string procName = it->second.second;
    std::cout << "  " << status << "\t" << procName << "\n";
  }
  return 0;
}