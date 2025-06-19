#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH 1024
#define KEYWORD "TODO"

pthread_mutex_t lock;

void search_in_file(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) return;

    char line[1024];
    int line_num = 1;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, KEYWORD)) {
            pthread_mutex_lock(&lock);
            printf("[MATCH] %s:%d: %s", filepath, line_num, line);
            pthread_mutex_unlock(&lock);
        }
        line_num++;
    }

    fclose(file);
}

void *traverse_dir(void *arg) {
    char *path = (char *)arg;
    struct dirent *entry;
    DIR *dir = opendir(path);

    if (!dir) return NULL;

    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1) continue;

        if (S_ISDIR(st.st_mode)) {
            pthread_t tid;
            char *sub_path = strdup(full_path);
            pthread_create(&tid, NULL, traverse_dir, sub_path);
            pthread_detach(tid);
        } else if (S_ISREG(st.st_mode)) {
            search_in_file(full_path);
        }
    }

    closedir(dir);
    free(path);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    pthread_mutex_init(&lock, NULL);
    traverse_dir(strdup(argv[1]));
    sleep(2); // allow detached threads to finish (hacky, better use join in practice)
    pthread_mutex_destroy(&lock);

    return 0;
}
