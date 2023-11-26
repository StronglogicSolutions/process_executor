#pragma once

#include <sys/wait.h>
#include <sys/poll.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cerrno>
#include <future>
#include <optional>

namespace kiq {
struct ProcessResult
{
  std::string output;
  bool        error{false};
  int         termination_code{0};
  std::string err_msg;
};

using args_t     = std::vector<std::string>;
using proc_fut_t = std::future<ProcessResult>;
using opt_fut_t  = std::optional<proc_fut_t>;

struct proc_wrap_t
{
  ProcessResult result;
  opt_fut_t     fut;
};
//--------------------------------------------------------------------
static const std::string get_current_working_directory()
{
  std::string ret;
  char        buffer[PATH_MAX];
  if (getcwd(buffer, sizeof(buffer)) != nullptr)
    ret = std::string(buffer);
  return ret;
}
//--------------------------------------------------------------------
static const uint32_t buf_size{32768};
//--------------------------------------------------------------------
static std::string read_fd(int fd)
{
  char buffer[buf_size];
  std::string s;
  ssize_t r;

  while (r = read(fd, buffer, buf_size) > 0)
    s.append(buffer, r);

  if (r < 0)
    std::cerr << "Reading from fd returned error: " << std::strerror(errno) << std::endl;

  return s;
}
//--------------------------------------------------------------------
[[ maybe_unused ]]
static proc_wrap_t qx(const args_t&      args,
                      int                timeout_sec       = 30,
                      bool               kill_on_timeout   = false,
                      bool               handle_process    = true,
                      const std::string& working_directory = "")
{
  int stdout_fds[2];
  int stderr_fds[2];
  pipe(stdout_fds);
  pipe(stderr_fds);

  const pid_t pid = fork();
  if (!pid)                                       // Child
  {
    if (!working_directory.empty()) chdir(working_directory.c_str());

    close(stdout_fds[0]);
    dup2 (stdout_fds[1], 1);
    close(stdout_fds[1]);
    close(stderr_fds[0]);
    dup2 (stderr_fds[1], 2);
    close(stderr_fds[1]);

    std::vector<char*> vc(args.size() + 1, 0);

    for (size_t i = 0; i < args.size(); ++i)
      vc[i] = const_cast<char*>(args[i].c_str());

    execvp(vc[0], &vc[0]);
    exit(0);
  }
                                                  // Parent
  close(stdout_fds[1]);
  close(stderr_fds[1]);

  auto handler = [pid, &stdout_fds, &stderr_fds, timeout_sec, kill_on_timeout]
  {
    ProcessResult result;
    pollfd        poll_fds[2] {
      pollfd{
        .fd      = stdout_fds[0] & 0xFF,
        .events  = POLL_OUT | POLL_ERR | POLL_IN,
        .revents = short{0}
      },
      pollfd{
        .fd      = stderr_fds[0] & 0xFF,
        .events  = POLLHUP | POLLERR | POLLIN,
        .revents = short{0}
      }};

    const auto timeout = timeout_sec * 1000;
    if (const int poll_result = poll(poll_fds, 2, timeout); !poll_result)
    {
      if (timeout)
      {
        if (kill_on_timeout)
          kill(pid, SIGKILL);

        result.error   = true;
        result.err_msg = "Child process timed out";
      }
      else
        result.output = "Non-block poll() returned immediately with no results";
    }
    else
    if (poll_result == -1)
      result.err_msg = "Errno while calling poll(): " + std::string(std::strerror(errno));
    else
    if (poll_result)
    {
      if (poll_fds[1].revents & POLLIN)
      {
        result.output = read_fd(poll_fds[1].fd);;
        result.error  = true;
      }
      else
      if (poll_fds[0].revents & POLLHUP)
      {
        result.error  = true;
        result.output = "Lost connection to forked process";
      }

      if (poll_fds[0].revents & POLLIN)
      {
        result.output = read_fd(poll_fds[0].fd);
        result.error  = result.output.empty();
      }
    }

    close(stdout_fds[0]);
    close(stderr_fds[0]);

    int status;
    if (const pid_t wait_result = waitpid(pid, &status, (WNOHANG | WUNTRACED | WCONTINUED)); wait_result == -1)
      result.err_msg = "Error waiting for " + pid + std::string(". Error: " + std::string(std::strerror(errno)));
    else
    if (!wait_result) // unchanged
    {
      if (result.output.empty())
        result.output = "Forked process may not have completed";
    }
    else
    {
      if (WIFEXITED(status))
        result.termination_code = WEXITSTATUS(status);
      else
      if (WIFSIGNALED(status))
        result.output += "Child process terminated by signal: "  + std::to_string(WTERMSIG(status));
    }

    return result;
  };

  proc_wrap_t process;

  if (handle_process)
    process.result = handler();
  else
    process.fut    = opt_fut_t{std::async(std::launch::async, handler)};

  return process;
}

} //ns kiq
