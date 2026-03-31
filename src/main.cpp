#include "builtins.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

// builtins store
std::map<std::string, std::function<int(std::string)>> builtins = {
    {"echo", c_echo}, {"exit", c_exit},       {"type", c_type}, {"pwd", c_pwd},
    {"cd", c_cd},     {"history", c_history}, {"jobs", c_jobs}};

std::map<int, std::pair<int,std::string>> running_jobs;
int job_idx = 1;

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  struct termios original_termios;
  tcgetattr(STDIN_FILENO, &original_termios);
  while (true) {
    int history_idx = history_list.size();

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
          if (command.find_last_of('/') != std::string::npos) {
            lastToken = command.substr(command.find_last_of('/') + 1);
          }
          std::string autocomplete =
              possible_completions[0].substr(lastToken.size());

          if (autocomplete.back() == '/') {
            std::cout << autocomplete;
            command += autocomplete + '/';
          } else {
            std::cout << autocomplete << ' ';
            command += autocomplete + ' ';
          }

        } else {

          std::string lcp = longestCommonPrefix(possible_completions);
          if (lcp.size() > lastToken.size()) {
            std::string autocomplete = lcp.substr(lastToken.size());
            std::cout << autocomplete;
            command += autocomplete;
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
      } else if (ch == 27) {
        char seq[2];
        set_raw_mode(original_termios);
        bool ok = (read(STDIN_FILENO, &seq[0], 1) > 0 &&
                   read(STDIN_FILENO, &seq[1], 1) > 0);
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);

        if (ok && seq[0] == '[') {
          if (seq[1] == 'A') { // up
            if (history_idx > 0) {
              history_idx--;
              command = history_list[history_idx];
              std::cout << "\r\33[2K$ " << command << std::flush;
            }
          } else if (seq[1] == 'B') { // down
            if (history_idx + 1 < (int)history_list.size()) {
              history_idx++;
              command = history_list[history_idx];
            } else {
              history_idx = history_list.size();
              command.clear();
            }
            std::cout << "\r\33[2K$ " << command << std::flush;
          }
        }

      } else {
        command += ch;
        std::cout << ch << std::flush;
      }
    }
    std::string lastToken = command.substr(command.find_last_of(' ') + 1);
    pid_t pid = -1;
    if (lastToken == "&") {
      pid = fork();
      if (pid) {
        std::cout << '[' << job_idx << "] " << pid << '\n';
        running_jobs[job_idx] = {pid, command};
        job_idx++;
      }
      command = command.substr(0, command.find_last_of(' '));
    }

    if (pid == -1 || pid == 0) {
      history_list.push_back(command);

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

      if (pid == 0) {
        return 0;
      }
    }
  }
}