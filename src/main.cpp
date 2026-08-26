#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <unordered_set>
#include <vector>
#include <wait.h>

constexpr char path_list_separator = ':';

struct Command {
  std::string name;
  std::vector<std::string> args;
};

pid_t runExternal(const Command &cmd) {

  std::vector<char *> argv;
  argv.reserve(cmd.args.size() + 2);

  argv.push_back(const_cast<char *>(cmd.name.c_str()));
  for (auto &arg : cmd.args)
    argv.push_back(const_cast<char *>(arg.c_str()));
  argv.push_back(nullptr);

  pid_t c_pid = fork();

  if (c_pid == -1) {
    throw std::runtime_error("fork failed: " +
                             std::string(std::strerror(errno)));
  } else if (c_pid > 0) {

    int status;
    if (waitpid(c_pid, &status, 0) == -1) {
      // You can either throw, log, or ignore depending on your design
    }

  } else {
    execv(cmd.name.c_str(), argv.data());

    const char *msg = "execv failed: ";
    write(STDERR_FILENO, msg, std::strlen(msg));
    write(STDERR_FILENO, std::strerror(errno),
          std::strlen(std::strerror(errno)));
    write(STDERR_FILENO, "\n", 1);
    _exit(127);
  }
  return c_pid;
}

Command parse_cmd(const std::string &line) {
  Command cmd;

  std::istringstream iss(line);
  std::string tok;

  if (iss >> tok) {
    cmd.name = tok;
    while (iss >> tok) {
      cmd.args.push_back(tok);
    }
  }
  return cmd;
}

std::vector<std::filesystem::path> split_path_variable() {
  std::vector<std::filesystem::path> dir;

  const char *raw_path = std::getenv("PATH");
  if (raw_path == nullptr) {
    return dir;
  }
  std::string path_variable(raw_path);
  std::size_t start = 0;

  while (start <= path_variable.size()) {
    const std::size_t end = path_variable.find(path_list_separator, start);
    const std::string entry = path_variable.substr(start, end - start);
    if (!entry.empty())
      dir.emplace_back(entry);
    if (end == std::string::npos)
      break;
    start = end + 1;
  }

  return dir;
}
std::filesystem::path find_exec(const std::string &cmd) {

  for (const auto &dir : split_path_variable()) {

    const auto candidate = dir / cmd;

    std::error_code ec;
    auto st = std::filesystem::status(candidate, ec);

    if (ec)
      continue;
    if (!std::filesystem::is_regular_file(st))
      continue;

    if ((st.permissions() & std::filesystem::perms::owner_exec) !=
        std::filesystem::perms::none) {
      return candidate;
    }
  }

  return {};
}

int main(int argc, char *argv[]) {
  while (1) {
    std::cout << "$ ";
    std::string line;
    if (!std::getline(std::cin, line))
      break;

    const std::unordered_set<std::string> builtin = {"echo", "type", "exit",
                                                     "pwd"};

    Command cmd = parse_cmd(line);
    const std::string &kw = cmd.name;
    auto exe = find_exec(kw);

    if (kw == "exit") {
      break;
    } else if (kw == "echo") {
      if (line.size() > 5) {
        std::string msg = line.substr(5);
        std::cout << msg << std::endl;
      } else {
        std::cout << "Usage: echo message" << std::endl;
      }
    } else if (kw == "type") {
      for (auto arg : cmd.args) {
        if (builtin.find(arg) != builtin.end())
          std::cout << arg << " is a shell builtin";
        else if (!find_exec(arg).empty())
          std::cout << arg << " is " << find_exec(arg) << std::endl;
        else if (find_exec(arg).empty())
          std::cout << "type: Command not found" << std::endl;
        else {
          std::cout << "Usage: type commmand";
        }
      }
    } else if (kw == "pwd") {

    } else if (!exe.empty()) {
      cmd.name = exe.string();
      runExternal(cmd);
    } else
      std::cout << kw << ": command not found" << std::endl;
  }
  return 0;
}
