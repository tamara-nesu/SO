#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "treasure_utils.h"


void create_hunt_directory(const char *hunt_id) {
  char dir_name[256];
  strcpy(dir_name, hunt_id);
  struct stat st;
  if (stat(dir_name, &st) == -1) {
    int result = mkdir(dir_name, 0700);
    if (result == -1) {
      perror("mkdir");
      exit(EXIT_FAILURE);
    }
  }

}

void build_treasure_file_path(char *path, const char *hunt_id) {
  strcpy(path, hunt_id);
  strcat(path, "/");
  strcat(path, TREASURE_FILE);
}

void build_log_file_path(char *path, const char *hunt_id) {
  strcpy(path, hunt_id);
  strcat(path, LOG_FILE);
}


void build_symlink_name(char *link_name, const char *hunt_id) {
    strcpy(link_name, SYMLINK_PREFIX);
    strcat(link_name, hunt_id);
}


void build_tmp_file_path(char *dest, const char *hunt_id) {
    strcpy(dest , hunt_id);
    strcat(dest , "/");
    strcat(dest , TMP_FILE);
  }


int get_option_code(const char *op) {
    if (strcmp(op, "--add") == 0) return 1;
    if (strcmp(op, "--list") == 0) return 2;
    if (strcmp(op, "--view") == 0) return 3;
    if (strcmp(op, "--remove_treasure") == 0) return 4;
    if (strcmp(op, "--remove_hunt") == 0) return 5;
    return -1;
}

  

void log_operation(const char *hunt_id, const char *operation) {
  char log_path[256];
  build_log_file_path(log_path, hunt_id);

  int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (log_fd == -1) {
    perror("[ERROR] Cannot open log file");
    return;
  }

  dprintf(log_fd, "%s: %s\n", operation, hunt_id);
  close(log_fd);
  printf("[DEBUG] Log updated: %s: %s\n", operation, hunt_id);

  char link_name[256];
  build_symlink_name(link_name, hunt_id);

  if (access(link_name, F_OK) != -1) {
    if (unlink(link_name) == -1) {
      perror("[ERROR] Cannot remove existing symlink");
      return;
    }
    printf("[DEBUG] Existing symlink removed: %s\n", link_name);
  }

  if (symlink(log_path, link_name) == -1) {
    perror("[ERROR] symlink failed");
  } else {
    printf("[DEBUG] Symlink created: %s -> %s\n", link_name, log_path);
  }
}