#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <unordered_set>
#include <vector>

constexpr char path_list_separator = ':';

struct Command {
  std::string name;
  std::vector<std::string> args;
};

std::filesystem::path expand_tilde(const std::filesystem::path &p) {

  const char *home = getenv("HOME");
  const auto &native_str = p.native();

  if (native_str[0] != '~')
    return p;
  if (native_str == "~" || native_str.empty())
    return std::filesystem::path(home);
  if (native_str[1] == '/')
    return std::filesystem::path(home) / native_str.substr(2);
  return p;
}

pid_t run_external(const Command &cmd) {

  std::vector<const char *> argv;
  argv.reserve(cmd.args.size() + 2);

  argv.push_back(cmd.name.c_str());
  for (const auto &arg : cmd.args)
    argv.push_back(arg.c_str());
  argv.push_back(nullptr);

  pid_t child_pid = fork();

  if (child_pid == -1) {
    throw std::runtime_error("fork failed: " +
                             std::string(std::strerror(errno)));
  } else if (child_pid > 0) {

    int status;
    if (waitpid(child_pid, &status, 0) == -1) {
      std::cerr << "waitpid failed: " << std::strerror(errno) << std::endl;
    }

  } else {
    execv(cmd.name.c_str(), const_cast<char *const *>(argv.data()));

    const char *msg = "execv failed: ";
    write(STDERR_FILENO, msg, std::strlen(msg));
    write(STDERR_FILENO, std::strerror(errno),
          std::strlen(std::strerror(errno)));
    write(STDERR_FILENO, "\n", 1);
    _exit(127);
  }
  return child_pid;
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
  std::vector<std::filesystem::path> dirs;

  const char *raw_path = std::getenv("PATH");
  if (!raw_path)
    return dirs;

  std::string path_variable(raw_path);
  std::size_t start = 0;

  while (true) {
    const std::size_t end = path_variable.find(path_list_separator, start);
    std::string entry = (end == std::string::npos)
                            ? path_variable.substr(start)
                            : path_variable.substr(start, end - start);

    if (!entry.empty())
      dirs.emplace_back(entry);

    if (end == std::string::npos)
      break;
    start = end + 1;
  }

  return dirs;
}

std::filesystem::path find_exec(const std::string &cmd) {

  for (const auto &dir : split_path_variable()) {
    std::error_code ec;
    const auto candidate = dir / cmd;
    std::string candidate_string = candidate.string();

    if (!access(candidate_string.c_str(), X_OK)) {
      return candidate;
    }
  }
  return {};
}

int main(void) {
  while (1) {
    std::cout << "> ";
    std::string line;
    if (!std::getline(std::cin, line))
      break;

    const std::unordered_set<std::string> builtin = {"echo", "type", "exit",
                                                     "pwd", "cd"};

    Command cmd = parse_cmd(line);
    const std::string &kw = cmd.name;
    auto exe = find_exec(kw);

    if (kw == "exit") {
      break;
    } else if (kw == "echo") {
      for (std::size_t i = 0; i < cmd.args.size(); i++) {
        if (i)
          std::cout << ' ';
        std::cout << cmd.args[i];
      }
      std::cout << '\n';
    } else if (kw == "type") {
      for (const auto &arg : cmd.args) {
        if (builtin.find(arg) != builtin.end()) {
          std::cout << arg << " is a shell builtin\n";
        } else {
          auto exe = find_exec(arg);
          if (!exe.empty()) {
            std::cout << arg << " is " << exe << '\n';
          } else {
            std::cerr << "type: " << arg << ": not found\n";
          }
        }
      }
    } else if (kw == "pwd") {
      std::error_code pwdError;
      std::filesystem::path currDir = std::filesystem::current_path(pwdError);
      std::cout << (pwdError ? pwdError.message() : currDir.string())
                << std::endl;

    } else if (kw == "cd") {
      std::filesystem::path dir;
      if (cmd.args.empty()) {
        const char *home = getenv("HOME");

        if (!home) {
          std::cerr << "cd: HOME not set\n";
          continue;
        }
        dir = home;
      } else {
        dir = expand_tilde(cmd.args[0]);
      }
      std::error_code ec;
      std::filesystem::current_path(dir, ec);
      if (ec) {
        std::cerr << "cd: " << dir << ec.message() << '\n';
      }
    } else if (!exe.empty()) {
      cmd.name = exe.string();
      run_external(cmd);
    } else
      std::cerr << kw << ": command not found" << std::endl;
  }
  return EXIT_SUCCESS;
}
