#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

volatile sig_atomic_t command_flag = 0;
volatile sig_atomic_t stop_flag = 0;
pid_t last_sender_pid = 0;


typedef struct{
  float longitude;
  float latitude;
}GPS;

typedef struct {
    char id[32];
    char username[32];
  GPS coordonates;
    char clue[128];
    int value;
} Treasure;


void handle_sigusr1(int sig, siginfo_t *info, void *context) {
  if(sig == SIGUSR1){
    command_flag = 1;
    if(info!= NULL){
      last_sender_pid = info->si_pid;
      printf("Received SIGUSR1 from process with PID: %d\n", info->si_pid);
    }else{
      last_sender_pid = -1;
      printf("Received SIGUSR1, but sender PID is unavailable (info is NULL)\n");
    }
  }else{
    printf("Unexpected signal received in SIGUSR1 handler: %d\n", sig);
  }
}

void handle_sigusr2(int sig) {
  if(sig == SIGUSR2){
    stop_flag = 1;
  }
}

void setup_signals() {
    struct sigaction sa1, sa2;
    sa1.sa_sigaction = handle_sigusr1;
    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa1, NULL);

    sa2.sa_handler = handle_sigusr2;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    sigaction(SIGUSR2, &sa2, NULL);
}

void list_hunts(FILE *out) {
    DIR *d = opendir(".");
    if (!d) {
        fprintf(out, "Error opening current directory\n");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            char path[256];
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
            if (access(path, F_OK) == 0) {
                fprintf(out, "Hunt '%s' has treasures.dat file\n", entry->d_name);
            } else {
                fprintf(out, "Hunt '%s' missing treasures.dat file\n", entry->d_name);
            }
        }
    }
    closedir(d);
}

void list_treasures(const char *hunt_id, FILE *out) {
    if (!hunt_id) {
        fprintf(out, "Missing hunt ID\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(out, "Cannot open treasures.dat for hunt '%s'\n", hunt_id);
        return;
    }

    Treasure t;
    fprintf(out, "Treasures in hunt '%s':\n", hunt_id);
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        fprintf(out, "ID: %s | User: %s | Lat: %.2f | Lon: %.2f | Value: %d\n",
                t.id, t.username, t.coordonates.latitude, t.coordonates.longitude, t.value);
    }
    close(fd);
}


void view_treasure(const char *hunt_id, const char *treasure_id, FILE *out) {
    if (!hunt_id || !treasure_id) {
        fprintf(out, "Missing hunt ID or treasure ID\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(out, "Cannot open treasures.dat for hunt '%s'\n", hunt_id);
        return;
    }

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, treasure_id) == 0) {
            fprintf(out, "=== Treasure Details ===\n");
            fprintf(out, "ID: %s\nUser: %s\nLat: %.2f\nLon: %.2f\nClue: %s\nValue: %d\n",
                    t.id, t.username, t.coordonates.latitude, t.coordonates.longitude, t.clue, t.value);
            found = 1;
            break;
        }
    }
    if (!found) {
        fprintf(out, "Treasure ID '%s' not found in hunt '%s'\n", treasure_id, hunt_id);
    }
    close(fd);
}



void process_command(const char *cmdline) {
    char cmd_copy[512];
    strncpy(cmd_copy, cmdline, sizeof(cmd_copy));
    cmd_copy[sizeof(cmd_copy)-1] = '\0';

    char *cmd = strtok(cmd_copy, " ");
    char *arg1 = strtok(NULL, " ");
    char *arg2 = strtok(NULL, " ");

    FILE *fout = fopen("response.txt", "w");
    if (!fout) {
        perror("fopen response.txt");
        return;
    }

    if (!cmd) {
        fprintf(fout, "Empty command\n");
        fclose(fout);
        return;
    }

    if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts(fout);
    } else if (strcmp(cmd, "list_treasures") == 0) {
        list_treasures(arg1, fout);
    } else if (strcmp(cmd, "view_treasure") == 0) {
        view_treasure(arg1, arg2, fout);
    } else {
        fprintf(fout, "Unknown command: %s\n", cmd);
    }

    fclose(fout);
}

int main() {
    setup_signals();
    printf("[monitor] PID %d waiting for commands...\n", getpid());

    while (!stop_flag) {
        if (command_flag) {
            int fd = open("command.txt", O_RDONLY);
            if (fd < 0) {
                perror("open command.txt");
                command_flag = 0;
                continue;
            }

            char cmdline[512] = {0};
            ssize_t bytes_read = read(fd, cmdline, sizeof(cmdline) - 1);
            close(fd);

            if (bytes_read <= 0) {
                command_flag = 0;
                continue;
            }

            
            cmdline[strcspn(cmdline, "\n")] = 0;

            printf("[monitor] Processing command: %s\n", cmdline);
            process_command(cmdline);

            if (last_sender_pid > 0) {
                kill(last_sender_pid, SIGUSR2);
            }

            command_flag = 0;
        }
        usleep(50000);
    }

    printf("[monitor] Shutting down.\n");
    return 0;
}
