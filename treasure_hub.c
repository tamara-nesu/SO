#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

typedef enum {
    CMD_UNKNOWN,
    CMD_START_MONITOR,
    CMD_STOP_MONITOR,
    CMD_LIST_HUNTS,
    CMD_LIST_TREASURES,
    CMD_VIEW_TREASURE,
    CMD_EXIT
} CommandType;

CommandType get_command_type(const char *input) {
    if (strncmp(input, "start_monitor", 13) == 0)
        return CMD_START_MONITOR;
    if (strncmp(input, "stop_monitor", 12) == 0)
        return CMD_STOP_MONITOR;
    if (strncmp(input, "list_hunts", 10) == 0)
        return CMD_LIST_HUNTS;
    if (strncmp(input, "list_treasures", 14) == 0)
        return CMD_LIST_TREASURES;
    if (strncmp(input, "view_treasure", 13) == 0)
        return CMD_VIEW_TREASURE;
    if (strncmp(input, "exit", 4) == 0)
        return CMD_EXIT;

    return CMD_UNKNOWN;
}

pid_t monitor_pid = -1;
int monitor_stopping = 0;

void write_command_to_file(const char *command){
    int fd = open("command.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1){
        perror("Error opening command.txt");
        return;
    }
    if (write(fd, command, strlen(command)) == -1 || write(fd, "\n", 1) == -1){
        perror("Error writing command");
    }
    close(fd);
}

void send_command_to_monitor(const char *command) {
    char formatted[512];

    if (monitor_pid > 0) {
        char command_copy[512];
        strncpy(command_copy, command, sizeof(command_copy));

        char *cmd_str = strtok(command_copy, " ");
        char *arg1 = strtok(NULL, " ");
        char *arg2 = strtok(NULL, " ");

        if (cmd_str && strcmp(cmd_str, "VIEW_TREASURE") == 0) {
            if (arg1 && arg2 && strlen("VIEW_TREASURE ") + strlen(arg1) + strlen(arg2) + 1 < sizeof(formatted)) {
                snprintf(formatted, sizeof(formatted), "VIEW_TREASURE %s %s", arg1, arg2);
            } else {
                fprintf(stderr, "Command is too long for a buffer of size %lu.\n", sizeof(formatted));
                return;
            }
        }

        write_command_to_file(formatted);
        kill(monitor_pid, SIGUSR1);
        printf("Sent command signal to monitor with PID: %d\n", monitor_pid);
    } else {
        printf("Monitor is not running.\n");
    }
}

void start_monitor(){
    if (monitor_pid > 0){
        printf("Monitor is already running (PID: %d).\n", monitor_pid);
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == -1){
        perror("Error in fork()");
        exit(1);
    }

    if (pid1 == 0){
        execl("./monitor", "monitor", NULL);
        perror("Error in execl()");
        exit(1);
    } else {
        monitor_pid = pid1;
        monitor_stopping = 0;
        printf("Monitor started with PID: %d\n", monitor_pid);
    }
}

void stop_monitor(){
    if (monitor_pid > 0){
        kill(monitor_pid, SIGTERM);
        monitor_stopping = 1;
        printf("Sent SIGTERM signal to monitor (PID: %d)\n", monitor_pid);

        usleep(500000);

        int status;
        waitpid(monitor_pid, &status, 0);
        printf("Monitor stopped. Exit code: %d\n", status);
        monitor_pid = -1;
        monitor_stopping = 0;
    } else {
        printf("Monitor is not running.\n");
    }
}

int main() {
    char input[256];

    while (1) {
        printf(">>> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)){
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (monitor_stopping){
            printf("Monitor is shutting down. Please wait for completion.\n");
            continue;
        }

        CommandType cmd = get_command_type(input);

        switch (cmd) {
            case CMD_START_MONITOR:
                start_monitor();
                break;

            case CMD_STOP_MONITOR:
                stop_monitor();
                break;

            case CMD_LIST_HUNTS:
            case CMD_LIST_TREASURES:
            case CMD_VIEW_TREASURE:
                send_command_to_monitor(input);
                break;

            case CMD_EXIT:
                if (monitor_pid > 0){
                    printf("Monitor is still running. Stop it with stop_monitor.\n");
                } else {
                    return 0;
                }
                break;

            case CMD_UNKNOWN:
            default:
                printf("Unknown command.\n");
        }
    }

    return 0;
}
