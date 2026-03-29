#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  while(true){
    std::cout << "$ ";
    std::string command;
    std::getline(std::cin, command);
    
    if(command == "exit"){
      break;
    } else if(command.substr(0, 4) == "echo" || command.substr(0, 5) == "echo "){
      std::cout << command.substr(5, command.size()-5) << '\n';
    }else {
      std::cout << command << ": command not found\n";
    }
  }
}
