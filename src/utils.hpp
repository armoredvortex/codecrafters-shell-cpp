#include <string>
#include <vector>

void set_raw_mode(struct termios &original);
bool isValidCommand(std::string command);
bool isExecutable(const std::string &path_str);
void matchOnPath(std::string args, std::vector<std::string> &completions);
void matchFilename(std::string arg, std::vector<std::string> &completions);
std::string longestCommonPrefix(std::vector<std::string> &strs);
std::string findOnPath(std::string args);