#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

pid_t monitor_pid = -1;
int monitor_stopping = 0;
volatile sig_atomic_t response_ready = 0;


void sigusr2_handler(int sig) {
  if(sig == SIGUSR2){
    response_ready = 1;
  }
}

void start_monitor() {
    if (monitor_pid > 0) {
        printf("Monitor already running (PID %d)\n", monitor_pid);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Erorr in fork()");
        exit(1);
    }
    if (pid == 0) {
        execl("./monitor", "monitor", NULL);
        perror("execl");
        exit(1);
    }else{
    monitor_pid = pid;
    monitor_stopping = 0;
    printf("Monitor started with PID %d\n", monitor_pid);
    }
}

void stop_monitor() {
    if (monitor_pid > 0) {
      kill(monitor_pid , SIGTERM);
      monitor_stopping = 1;
      printf("Sent SIGTERM signal to monitor (PID: %d)\n", monitor_pid);

      usleep(500000);

        int status;
        waitpid(monitor_pid, &status, 0);
        printf("Monitor stopped. Exit code: %d\n", status);
        monitor_pid = -1;
        monitor_stopping = 0;
    }else{
      printf("Monitor is not running.\n");
    }
}


void send_command(const char *cmdline) {
    if (monitor_pid <= 0) {
        printf("Monitor is not running. Use start_monitor first.\n");
        return;
    }


    int fd = open("command.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open command.txt");
        return;
    }
    write(fd, cmdline, strlen(cmdline));
    write(fd, "\n", 1);
    close(fd);

 
    response_ready = 0;
    kill(monitor_pid, SIGUSR1);


    while (!response_ready) {
        usleep(10000);
    }


    fd = open("response.txt", O_RDONLY);
    if (fd < 0) {
        perror("open response.txt");
        return;
    }
    char buffer[4096];
    ssize_t n = read(fd, buffer, sizeof(buffer)-1);
    close(fd);
    if (n > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    } else {
        printf("No response from monitor.\n");
    }
}


int main() {
    struct sigaction sa;
    sa.sa_handler = sigusr2_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    char input[256];
    while (1) {
        printf(">>> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = '\0'; 

           if (strcmp(input, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(input, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_pid > 0) {
                printf("Stop monitor first with stop_monitor\n");
            } else {
                printf("Exiting.\n");
                break;
            }
		} else if (strncmp(input, "calculate_score" , 15) == 0) {
	 
    char *arg = strchr(input, ' ');
    if (arg) {
        arg++; 
        send_command(input);
    } else {
        printf("Usage: calculate_score <hunt_id>\n");
    }
        } else if (strlen(input) > 0) {
            send_command(input);
        }
    }

    if (monitor_pid > 0) {
        stop_monitor();
    }

    return 0;
} 
