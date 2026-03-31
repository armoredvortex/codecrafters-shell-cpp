#include <functional>
#include <map>
#include <string>
#include <vector>

extern std::map<std::string, std::function<int(std::string)>> builtins;
extern std::vector<std::string> history_list;

int c_exit(std::string args);
int c_echo(std::string args);
int c_type(std::string args);
int c_pwd(std::string args);
int c_cd(std::string args);
int c_history(std::string args);
int c_jobs(std::string args);
