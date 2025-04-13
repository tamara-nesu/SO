#ifndef TREASURE_UTILS_H
#define TREASURE_UTILS_H

#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt.txt"
#define SYMLINK_PREFIX "logged-hunt-"
#define TMP_FILE "tmp_treasures.dat"

#define TREASURE_ID_LEN 32
#define USERNAME_LEN 32
#define CLUE_LEN 128

typedef struct {
    float latitude;
    float longitude;
  } GPS;
  
  typedef struct {
    char treasureID[TREASURE_ID_LEN];
    char username[USERNAME_LEN];
    GPS coordonates;
    char clue[CLUE_LEN];
    int value;
  } Treasure;


void create_hunt_directory(const char *hunt_id);
void add_treasure(const char *hunt_id);
void list_treasures(const char *hunt_id);
void view_treasure(const char *hunt_id , const char *id);
void remove_treasure(const char *hunt_id , const char *id);
void remove_hunt(const char *hunt_id);

void build_logged_hunt_file_path(char *path , const char *hunt_id);
void build_treasure_file_path(char *path, const char *hunt_id);
void build_log_file_path(char *path, const char *hunt_id);
void build_symlink_name(char *link_name, const char *hunt_id);
void build_tmp_file_path(char *dest , const char *hunt_id);
int get_option_code(const char *op);
void log_operation(const char *hunt_id , const char *operation);
void display_menu();
#endif
