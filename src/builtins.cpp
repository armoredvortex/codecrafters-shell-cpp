#include "builtins.hpp"
#include "utils.hpp"
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

std::vector<std::string> history_list;
size_t history_saved_idx = 0;

int c_history(std::string args) {
  if (args.substr(0, 2) == "-r" || args.substr(0, 2) == "-w" || args.substr(0, 2) == "-a") {
    size_t start_idx = args.find(' ');
    if (start_idx == std::string::npos || start_idx + 1 >= args.size()) {
      return 0;
    }

    std::string filename = args.substr(start_idx + 1);

    if (args.substr(0, 2) == "-r") {
      std::ifstream file(filename);

      if (!file.is_open()) {
        std::cerr << "history: " << filename << ": cannot open\n";
        return -11;
      }
      std::string line;
      while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }
        if (line.empty()) {
          continue;
        }
        history_list.push_back(line);
      }
      history_saved_idx = history_list.size();
    } else if (args.substr(0, 2) == "-w" || args.substr(0, 2) == "-a") {
      std::ofstream file(
        filename,
        args.substr(0, 2) == "-w" ? std::ios::out : (std::ios::out | std::ios::app)
      );

      if (!file.is_open()) {
        std::cerr << "history: " << filename << ": cannot open\n";
        return -11;
      }

      size_t start = args.substr(0, 2) == "-a" ? history_saved_idx : 0;
      for (size_t i = start; i < history_list.size(); i++) {
        file << history_list[i] << '\n';
      }
      history_saved_idx = history_list.size();

      file.close();
    }

  } else {
    int n = INT_MAX;
    if (args.size())
      n = std::stoi(args);
    for (int i = std::max(0, (int)history_list.size() - n);
         i < history_list.size(); i++) {
      std::cout << std::setw(5) << i + 1 << "  " << history_list[i] << '\n';
    }
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

  for (auto it = running_jobs.begin(); it != running_jobs.end();) {
    std::string status;
    int child_status = 0;
    pid_t result = waitpid(it->second.first, &child_status, WNOHANG);
    if (result == 0) {
      status = "Running";
    } else if (result > 0 &&
               (WIFEXITED(child_status) || WIFSIGNALED(child_status))) {
      status = "Done";
    } else {
      ++it;
      continue;
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
    std::cout << "  " << status << "\t\t";
    if (status == "Done") {
      std::cout << procName.substr(0, procName.size() - 2) << '\n';
      it = running_jobs.erase(it);
    } else {
      std::cout << procName << '\n';
      ++it;
    }
  }
  return 0;
}