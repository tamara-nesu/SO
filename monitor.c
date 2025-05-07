#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_LINE 256
#define MAX_PATH 256
#define MAX_TREASURES 100

volatile sig_atomic_t stop_request = 0;
volatile sig_atomic_t command_request = 0;

void signal_handler(int sig, siginfo_t *info, void *context) {
    if (sig == SIGUSR1) {
        command_request = 1;
        printf("[monitor] Received command signal from PID %d.\n", info->si_pid);
        sleep(1);
    } else if (sig == SIGUSR2) {
        printf("[monitor] Shutdown request received from PID %d.\n", info->si_pid);
        stop_request = 1;
        sleep(2);
    }
}

void setup_signal_handlers() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sigaddset(&sa.sa_mask, SIGUSR2);
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }
}

typedef enum {
    CMD_START_MONITOR,
    CMD_STOP_MONITOR,
    CMD_LIST_HUNTS,
    CMD_LIST_TREASURES,
    CMD_VIEW_TREASURE,
    CMD_EXIT,
    CMD_UNKNOWN
} CommandType;

CommandType get_command_type(const char *cmd) {
    if (strcmp(cmd, "start_monitor") == 0) return CMD_START_MONITOR;
    if (strcmp(cmd, "stop_monitor") == 0) return CMD_STOP_MONITOR;
    if (strcmp(cmd, "list_hunts") == 0) return CMD_LIST_HUNTS;
    if (strcmp(cmd, "list_treasures") == 0) return CMD_LIST_TREASURES;
    if (strcmp(cmd, "view_treasure") == 0) return CMD_VIEW_TREASURE;
    if (strcmp(cmd, "exit") == 0) return CMD_EXIT;
    return CMD_UNKNOWN;
}

void list_hunts() {
    DIR *dir = opendir(".");  
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            char path[512];
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);

            FILE *file = fopen(path, "r");
            if (file) {
                printf("Hunt %s: treasures.dat file found.\n", entry->d_name);
                fclose(file);
            } else {
                printf("Hunt %s: treasures.dat file not found.\n", entry->d_name);
            }
        }
    }

    closedir(dir);
}

typedef struct {
    char id[32];
    char username[32];
    float latitude;
    float longitude;
    char clue[128];
    int value;
} Treasure;

void list_treasures(const char *hunt_id) {
    if (!hunt_id) {
        printf("Missing hunt ID.\n");
        return;
    }

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Unable to open treasures file");
        return;
    }

    Treasure t;
    printf("Treasures in hunt '%s':\n", hunt_id);
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %s | User: %s | (%.2f, %.2f) | Value: %d\n",
               t.id, t.username, t.latitude, t.longitude, t.value);
    }

    close(fd);
}

void view_treasure(const char *hunt_id, const char *treasure_id) {
    if (!hunt_id || !treasure_id) {
        printf("Missing parameters for view_treasure command.\n");
        return;
    }

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Unable to open treasures file");
        return;
    }

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, treasure_id) == 0) {
            printf("=== Treasure Details ===\n");
            printf("ID: %s\nUser: %s\nLat: %.2f\nLon: %.2f\nClue: %s\nValue: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Treasure with ID '%s' not found.\n", treasure_id);
    }

    close(fd);
}

void process_command() {
    int fd = open("command.txt", O_RDONLY);
    if (fd < 0) {
        perror("Error opening command.txt");
        return;
    }

    char buffer[256];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes <= 0) return;

    buffer[bytes] = '\0';
    buffer[strcspn(buffer, "\n")] = '\0';

    char *cmd_str = strtok(buffer, " ");
    char *arg1 = strtok(NULL, " ");
    char *arg2 = strtok(NULL, " ");

    CommandType cmd = get_command_type(cmd_str);

    switch (cmd) {
        case CMD_LIST_HUNTS:
            list_hunts();
            break;
        case CMD_LIST_TREASURES:
            list_treasures(arg1);
            break;
        case CMD_VIEW_TREASURE:
            view_treasure(arg1, arg2);
            break;
        default:
            printf("Unknown command: %s\n", cmd_str);
    }
}

int main() {
    setup_signal_handlers();
    printf("[monitor] Waiting for signals...\n");

    while (!stop_request) {
        if (command_request) {
            printf("[monitor] Processing command...\n");
            process_command();
            command_request = 0;
        }
        usleep(100000); 
    }

    printf("[monitor] Monitor is shutting down...\n");
    return 0;
}
