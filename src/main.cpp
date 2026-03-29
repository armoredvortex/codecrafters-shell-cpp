#include <iostream>
#include <string>
#include <functional>
#include <map>

int c_exit(std::string args){
  return -1;
}

int c_echo(std::string args){
  std::cout << args << '\n';
  return 0;
}

std::map<std::string, std::function<int(std::string)>> commands = {
  {"echo", c_echo},
  {"exit", c_exit}
};

bool isValidCommand(std::string command){
  return (commands.find(command) != commands.end());
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  while(true){
    std::cout << "$ ";
    std::string command;
    std::getline(std::cin, command);
    
    std::string cmd = command.substr(0, command.find(' '));
    std::string args;
    if (command.find(' ') != std::string::npos){
      args = command.substr(command.find(' ')+1, command.size() - command.find(' ')-1);
    }

    if(isValidCommand(cmd)){
      if(commands[cmd](args) == -1){
        return 0;
      };
    } else {
      std::cout << command << ": command not found\n";
    }
  }
}