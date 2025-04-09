#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "treasure_utils.h"
#include <sys/stat.h>

#define TREASURE_ID_LEN 32
#define USERNAME_LEN 32
#define CLUE_LEN 128


// --add
void add_treasure(const char *hunt_id) {
  create_hunt_directory(hunt_id);
  char path[256];
  build_treasure_file_path(path, hunt_id);

  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd == -1) {
    perror("Error opening file");
    exit(1);
  }

  Treasure t;
  printf("Treasure ID: \n"); scanf("%s", t.treasureID);
  printf("Username: \n"); scanf("%s", t.username);
  printf("Latitude: \n"); scanf("%f", &t.coordonates.latitude);
  printf("Longitude: \n"); scanf("%f", &t.coordonates.longitude);
  printf("Clue: \n");
  getchar();
  fgets(t.clue, CLUE_LEN, stdin);
  printf("Value: \n"); scanf("%d", &t.value);

  if (write(fd, &t, sizeof(Treasure)) == -1) {
    perror("write");
    close(fd);
    exit(1);
  }
  close(fd);
  
  log_operation(hunt_id , "added");
  printf("Treasure added successfully.\n");
}

// --list
void list_treasures(const char *hunt_id) {
  char path[256];
  build_treasure_file_path(path, hunt_id);

  struct stat st;
  if (stat(path, &st) == -1) {
    perror("stat");
    exit(-1);
  }

  printf("Hunt name: %s\n", hunt_id);
  printf("Total file size: %ld bytes\n", st.st_size);
  printf("Last modification: %ld\n", st.st_mtime);

  int file = open(path, O_RDONLY);
  if (file == -1) {
    perror("open");
    exit(-1);
  }

  Treasure t;
  while (read(file, &t, sizeof(Treasure)) == sizeof(Treasure)) {
    printf("ID: %s | User: %s | Location: (%.2f , %.2f) | Clue: %s | Value: %d\n",
           t.treasureID, t.username, t.coordonates.latitude,
           t.coordonates.longitude, t.clue, t.value);
  }

  close(file);
}

// --view
void view_treasure(const char *hunt_id, const char *id) {
  char path[256];
  build_treasure_file_path(path, hunt_id);

  int file = open(path, O_RDONLY);
  if (file == -1) {
    perror("open");
    exit(-1);
  }

  Treasure t;
  int found = 0;
  while (read(file, &t, sizeof(Treasure)) == sizeof(Treasure)) {
    if (strcmp(t.treasureID, id) == 0) {
      printf("Treasure details:\n");
      printf("ID: %s \n", t.treasureID);
      printf("Username: %s \n", t.username);
      printf("Coordinates: (%.2f , %.2f)\n",
             t.coordonates.latitude, t.coordonates.longitude);
      printf("Clue: %s", t.clue);
      printf("Value: %d\n", t.value);
      found = 1;
      break;
    }
  }

  if (!found) {
    printf("Treasure with ID %s not found in hunt %s.\n", id, hunt_id); 
  }

  close(file);
}

// --remove_treasure
void remove_treasure(const char *hunt_id, const char *id) {
  char path[256], tmp_path[256];
  build_treasure_file_path(path, hunt_id);

  build_tmp_file_path(tmp_path , hunt_id);

  int fd = open(path, O_RDONLY);
  int fd_tmp = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (fd == -1 || fd_tmp == -1) {
    perror("open");
    exit(1);
  }

  Treasure t;
  int found = 0;
  while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
    if (strcmp(t.treasureID, id) != 0) {
      if (write(fd_tmp, &t, sizeof(Treasure)) == -1) {
        perror("write");
        close(fd);
        close(fd_tmp);
      }
    } else {
      found = 1;
    }
  }

  close(fd);
  close(fd_tmp);

  if (found) {
    if (rename(tmp_path, path) == -1) {
      perror("rename");
      exit(1);
    }
    printf("Treasure %s was removed from hunt %s.\n", id, hunt_id);
    log_operation(hunt_id, "remove_treasure");
  } else {
    printf("Treasure with ID %s not found.\n", id);
    unlink(tmp_path);
  }
}

// --remove_hunt
void remove_hunt(const char *hunt_id) {
  char path[256];
  build_treasure_file_path(path, hunt_id);
  unlink(path); // delete treasure.dat

  build_log_file_path(path, hunt_id);
  unlink(path); // delete logged_hunt

  build_symlink_name(path, hunt_id);
  unlink(path); // delete symlink

  if (rmdir(hunt_id) == -1) {
    perror("rmdir");
    exit(1);
  }
  printf("Hunt %s was completely removed.\n", hunt_id);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s --add <hunt_id>\n", argv[0]);
    return 1;
  }

  const char *op = argv[1];
  const char *hunt_id = argv[2];
  const char *treasure_id = NULL;

  if((strcmp(op , "--view") == 0 || strcmp(op , "--remove_treasure") == 0)){
  if (argc >= 4) {
    treasure_id = argv[3];
    printf("[DEBUG] Received treasure_id: %s\n", treasure_id);
  } else {
    printf("[DEBUG] treasure_id was not provided\n");
  }
}
  int opt = get_option_code(op);
  switch (opt) {
    case 1: // add
      create_hunt_directory(hunt_id);
      add_treasure(hunt_id);
      break;
    case 2: // list
      list_treasures(hunt_id);
      break;
    case 3: // view
      if (!treasure_id) {
        fprintf(stderr, "Treasure ID not specified\n");
        return 1;
      }
      view_treasure(hunt_id, treasure_id);
      break;
    case 4: // remove_treasure
      if (!treasure_id) {
        fprintf(stderr, "Treasure ID not specified\n");
        return 1;
      }
      remove_treasure(hunt_id, treasure_id);
      break;
    case 5: // remove_hunt
      remove_hunt(hunt_id);
      break;
    default:
      fprintf(stderr, "Unknown option: %s\n", op);
      return 1;
  }

  return 0;
}
