# Daemon Process Creation---Server Process

## Project Overview

This project demonstrates the creation and management of a Daemon Process in Linux using C programming.

A daemon is a background process that runs independently of the terminal and performs tasks without direct user interaction.

The project creates a background server process that periodically writes its execution status into a log file. It also implements signal handling, PID file management, file locking, and graceful termination.

## Objectives

- To understand daemon process creation in Linux.
- To implement process creation using `fork()`.
- To create a new session using `setsid()`.
- To detach the process from the controlling terminal.
- To implement signal handling using `sigaction()`.
- To manage process identification using a PID file.
- To prevent multiple instances using `flock()`.
- To maintain timestamp-based log records.
- To implement graceful daemon termination.

## Technologies Used

| Technology | Description |
|---|---|
| Programming Language | C |
| Operating System | Linux |
| Compiler | GCC |
| System Programming | POSIX APIs |
| Development Environment | Linux Terminal |

## Features

1. **Daemon Creation:** Uses double `fork()` to create a background process.
2. **Session Management:** Uses `setsid()` to create a new session.
3. **Terminal Independence:** Redirects standard input, output, and error to `/dev/null`.
4. **PID Management:** Stores the daemon process ID in a PID file.
5. **Instance Control:** Uses `flock()` to prevent multiple instances.
6. **Signal Handling:** Handles SIGTERM and SIGHUP signals.
7. **Background Logging:** Records daemon activities with timestamps.
8. **Periodic Execution:** Writes status messages every 5 seconds.
9. **Graceful Shutdown:** Performs cleanup when SIGTERM is received.

## System Calls and Functions Used

| Function | Purpose |
|---|---|
| `fork()` | Creates a child process |
| `setsid()` | Creates a new session |
| `chdir()` | Changes the working directory |
| `umask()` | Sets file creation mask |
| `open()` | Opens or creates a file |
| `flock()` | Locks the PID file |
| `dup2()` | Redirects file descriptors |
| `ftruncate()` | Clears PID file contents |
| `write()` | Writes PID into the file |
| `sigaction()` | Registers signal handlers |
| `sleep()` | Delays execution |
| `time()` | Gets current system time |
| `strftime()` | Formats timestamps |
| `fprintf()` | Writes formatted log messages |
| `fflush()` | Flushes buffered output |
| `unlink()` | Removes the PID file |

## Project Working

The daemon follows these steps:

1. Opens or creates the PID file.
2. Acquires an exclusive non-blocking file lock.
3. Opens the log file.
4. Performs the first `fork()`.
5. The parent process exits.
6. Creates a new session using `setsid()`.
7. Performs the second `fork()`.
8. The intermediate parent process exits.
9. Sets the file creation mask using `umask(0)`.
10. Changes the working directory to `/`.
11. Redirects standard file descriptors to `/dev/null`.
12. Writes the final daemon PID into the PID file.
13. Registers signal handlers.
14. Starts the daemon main loop.
15. Writes status messages into the log file every 5 seconds.
16. Handles SIGHUP and SIGTERM signals.
17. Performs cleanup and terminates gracefully.

## Signal Handling

### SIGTERM

SIGTERM requests graceful termination of the daemon.

When SIGTERM is received:
- Stops the main daemon loop.
- Writes shutdown messages into the log file.
- Closes the log file.
- Removes the PID file.
- Releases the file lock.

### SIGHUP

SIGHUP requests configuration reloading.

In this project, receiving SIGHUP sets a reload flag. The main loop detects the flag and writes a reload-request message into the log file.

Note: Actual configuration file reloading is not implemented.

## File Management

**PID File:**
```text
/tmp/student_daemon.pid
```

Stores the process ID of the running daemon.

**Log File:**
```text
/tmp/student_daemon.log
```

Stores timestamped daemon activities, signal events, and shutdown information.

## Compilation

Open the Linux terminal and compile the source code:

```bash
gcc -Wall -Wextra -std=c11 daemon.c -o daemon
```

## Execution

Run the program:

```bash
./daemon
```

The daemon starts running in the background.

## Testing and Monitoring

### 1. Check Running Daemon

```bash
pgrep -af './daemon'
```

### 2. Check Process ID

```bash
cat /tmp/student_daemon.pid
```

### 3. View Log File

```bash
cat /tmp/student_daemon.log
```

### 4. Monitor Logs Continuously

```bash
tail -f /tmp/student_daemon.log
```

### 5. Send SIGHUP Signal

```bash
kill -SIGHUP $(cat /tmp/student_daemon.pid)
```

Check the log file to verify the reload-request message.

### 6. Stop the Daemon

```bash
kill -SIGTERM $(cat /tmp/student_daemon.pid)
```

Check the log file to verify graceful shutdown.

## Expected Output

Example log file:

```text
2026-11-24 10:00:00 | PID 12345 | Daemon started
2026-11-24 10:00:00 | PID 12345 | Student daemon alive (Daemon alive)
2026-11-24 10:00:05 | PID 12345 | Student daemon alive (Daemon alive)
2026-11-24 10:00:10 | PID 12345 | Student daemon alive (Daemon alive)
2026-11-24 10:00:15 | PID 12345 | SIGHUP received - configuration reload requested
2026-11-24 10:00:20 | PID 12345 | SIGTERM received
2026-11-24 10:00:20 | PID 12345 | Daemon shutting down
```

*Note: The above output is illustrative. Actual timestamps and PID values will differ.*

## Applications

- Background server process management.
- Linux system administration.
- Background monitoring services.
- Logging and monitoring applications.
- Linux process management.

## Conclusion

This project demonstrates the creation and management of a daemon process in Linux using C programming.

It provides practical understanding of process creation, session management, signal handling, file locking, PID management, and background logging using POSIX system calls.

## Future Enhancements

- Implement actual configuration reloading.
- Add automatic error recovery.
- Implement resource monitoring.
- Add network communication capabilities.
- Implement log rotation.

---

**Project Name:** Daemon Process Creation---Server Process

**Domain:** Linux System Programming

**Language:** C
