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

int c_notfound(std::string args){
  std::cout << args << ": command not found\n";
  return 0;
}

int c_type(std::string args);

std::map<std::string, std::function<int(std::string)>> commands = {
  {"echo", c_echo},
  {"exit", c_exit},
  {"type", c_type}
};

int c_type(std::string args){
  if(commands.find(args) != commands.end()){
    std::cout << args << " is a shell builtin\n";
  } else {
    c_notfound(args);
  }
  return 0;
}

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
      c_notfound(cmd);
    }
  }
}