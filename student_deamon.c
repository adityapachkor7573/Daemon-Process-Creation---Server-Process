#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PID_FILE       "/tmp/student_daemon.pid"
#define LOG_FILE       "/tmp/student_daemon.log"
#define SLEEP_INTERVAL 5

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t reload_requested = 0;

/* Signal handler for SIGTERM */
static void handle_sigterm(int sig)
{
    (void)sig; 
    running = 0;
}

/* Signal handler for SIGHUP */
static void handle_sighup(int sig)
{
    (void)sig;
    reload_requested = 1;
}

/* Append a timestamped and PID-tagged entry to the daemon log file */
static void write_log(FILE *log_fp, const char *message)
{
    if (log_fp == NULL) {
        return;
    }

    time_t now = time(NULL);
    struct tm tm_info;
    char time_buf[32];

    if (localtime_r(&now, &tm_info) != NULL) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    } else {
        snprintf(time_buf, sizeof(time_buf), "UNKNOWN_TIME");
    }

    fprintf(log_fp, "%s | PID %d | %s\n", time_buf, (int)getpid(), message);
    fflush(log_fp);
}

/* Install signal handlers using sigaction() */
static int setup_signals(void)
{
    struct sigaction sa_term;
    struct sigaction sa_hup;
    struct sigaction sa_ign;

    
    memset(&sa_term, 0, sizeof(sa_term));
    sa_term.sa_handler = handle_sigterm;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0; /* Do not set SA_RESTART so sleep() wakes immediately */
    if (sigaction(SIGTERM, &sa_term, NULL) < 0) {
        return -1;
    }

    /* Handle SIGHUP */
    memset(&sa_hup, 0, sizeof(sa_hup));
    sa_hup.sa_handler = handle_sighup;
    sigemptyset(&sa_hup.sa_mask);
    sa_hup.sa_flags = 0;
    if (sigaction(SIGHUP, &sa_hup, NULL) < 0) {
        return -1;
    }

    /* Ignore SIGCHLD and SIGPIPE for clean background operation */
    memset(&sa_ign, 0, sizeof(sa_ign));
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    sa_ign.sa_flags = 0;
    sigaction(SIGCHLD, &sa_ign, NULL);
    sigaction(SIGPIPE, &sa_ign, NULL);

    return 0;
}

int main(void)
{
    int pid_fd = open(PID_FILE, O_RDWR | O_CREAT, 0644);
    if (pid_fd < 0) {
        perror("Failed to open PID file " PID_FILE);
        exit(EXIT_FAILURE);
    }

    if (flock(pid_fd, LOCK_EX | LOCK_NB) < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            fprintf(stderr, "Error: student_daemon is already running (PID file locked).\n");
        } else {
            perror("Failed to lock PID file");
        }
        close(pid_fd);
        exit(EXIT_FAILURE);
    }

    /* Ensure log file can be opened before detaching from terminal */
    FILE *log_fp = fopen(LOG_FILE, "a");
    if (log_fp == NULL) {
        perror("Failed to open log file " LOG_FILE);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }

    /* Step 2: First fork() - detach from parent and allow shell prompt to return */
    pid_t pid = fork();
    if (pid < 0) {
        perror("First fork() failed");
        fclose(log_fp);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        /* Parent exits immediately */
        exit(EXIT_SUCCESS);
    }

    /* Step 3: Create a new session and detach from controlling terminal */
    if (setsid() < 0) {
        perror("setsid() failed");
        fclose(log_fp);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }

    /* Step 4: Second fork() - ensure daemon is not a session leader */
    pid = fork();
    if (pid < 0) {
        perror("Second fork() failed");
        fclose(log_fp);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        /* Intermediate parent exits */
        exit(EXIT_SUCCESS);
    }

    /* Step 5: Set explicit file mode creation mask */
    umask(0);

    /* Step 6: Change current working directory to root */
    if (chdir("/") < 0) {
        write_log(log_fp, "Fatal: chdir(\"/\") failed");
        fclose(log_fp);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }

    /* Step 7: Redirect standard file descriptors (stdin, stdout, stderr) to /dev/null */
    int null_fd = open("/dev/null", O_RDWR);
    if (null_fd < 0) {
        write_log(log_fp, "Fatal: open(\"/dev/null\") failed");
        fclose(log_fp);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }
    if (dup2(null_fd, STDIN_FILENO) < 0 ||
        dup2(null_fd, STDOUT_FILENO) < 0 ||
        dup2(null_fd, STDERR_FILENO) < 0) {
        if (null_fd > STDERR_FILENO) {
            close(null_fd);
        }
        write_log(log_fp, "Fatal: dup2() failed for standard descriptors");
        fclose(log_fp);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }
    if (null_fd > STDERR_FILENO) {
        close(null_fd);
    }

    /* Step 8: Write final daemon PID into the locked PID file */
    if (ftruncate(pid_fd, 0) < 0 || lseek(pid_fd, 0, SEEK_SET) < 0) {
        write_log(log_fp, "Fatal: failed to truncate/seek PID file");
        fclose(log_fp);
        unlink(PID_FILE);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }

    char pid_buf[32];
    int pid_len = snprintf(pid_buf, sizeof(pid_buf), "%d\n", (int)getpid());
    if (write(pid_fd, pid_buf, (size_t)pid_len) != pid_len) {
        write_log(log_fp, "Fatal: failed to write PID to PID file");
        fclose(log_fp);
        unlink(PID_FILE);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }
    fsync(pid_fd);

    /* Step 9: Install signal handlers for SIGTERM and SIGHUP */
    if (setup_signals() < 0) {
        write_log(log_fp, "Fatal: sigaction() setup failed");
        fclose(log_fp);
        unlink(PID_FILE);
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        exit(EXIT_FAILURE);
    }

    /* Step 10: Run the main daemon loop */
    write_log(log_fp, "Daemon started");

    while (running) {
        write_log(log_fp, "Student daemon alive (Daemon alive)");

        unsigned int remaining = SLEEP_INTERVAL;
        while (remaining > 0 && running) {
            remaining = sleep(remaining);

            if (reload_requested) {
                reload_requested = 0;
                write_log(log_fp, "SIGHUP received - configuration reload requested");
            }
        }
    }

    /* Graceful shutdown and cleanup */
    write_log(log_fp, "SIGTERM received");
    write_log(log_fp, "Daemon shutting down");

    fclose(log_fp);

    /* Remove PID file and release lock */
    unlink(PID_FILE);
    flock(pid_fd, LOCK_UN);
    close(pid_fd);

    return EXIT_SUCCESS;
}
