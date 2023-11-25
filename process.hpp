#pragma once

#include <sys/wait.h>
#include <sys/poll.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

namespace kiq {
using args_t = std::vector<std::string>;
struct ProcessResult
{
  std::string output;
  bool        error{false};
  int         termination_code{0};
};

static const std::string get_current_working_directory()
{
  char* path = realpath("/proc/self/exe", NULL);
  char* name = basename(path);
  return std::string{path, path + strlen(path) - strlen(name)};
}

/**
 * Child process output buffer
 * 32k
 */
static const uint32_t buf_size{32768};

/**
 * readFd
 *
 * Helper function to read output from a file descriptor
 *
 * @param   [in]
 * @returns [out]
 *
 */
static std::string readFd(int fd)
{
  char buffer[buf_size];
  std::string s;
  ssize_t r;
  do
  {
    r = read(fd, buffer, buf_size);
    if (r > 0)
      s.append(buffer, r);
  }
  while (r > 0);
  return s;
}
//--------------------------------------------------------------------
[[ maybe_unused ]]
static ProcessResult qx(const args_t&      args,
                        int                timeout_sec       = 30,
                        bool               kill_on_timeout   = false,
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

  const auto timeout     = timeout_sec * 1000;
  const int  poll_result = poll(poll_fds, 2, timeout);
  if (!poll_result)
  {
    if (timeout)
    {
      std::cerr << "Failed to poll file descriptor after " << timeout << std::endl;

      if (kill_on_timeout)
        kill(pid, SIGKILL);

      result.error  = true;
      result.output = "Child process timed out";
    }
    else
      result.output = "Non-block poll() returned immediately with no results";
  }
  else
  if (poll_result == -1)
    std::cerr << "Errno while calling poll(): " << errno << std::endl;
  else
  if (poll_result)
  {
    if (poll_fds[1].revents & POLLIN)
    {
      result.output = readFd(poll_fds[1].fd);;
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

      result.output = readFd(poll_fds[0].fd);
      result.error  = result.output.empty();
    }
  }

  close(stdout_fds[0]);
  close(stderr_fds[0]);
  close(stderr_fds[1]);

  int           status;
  if (const pid_t wait_result = waitpid(pid, &status, (WNOHANG | WUNTRACED | WCONTINUED)); wait_result == -1)
  {
    std::cerr << "Error waiting for " << pid << "Errno: " << errno << std::endl;
  }
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
}

} //ns kiq
