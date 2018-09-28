#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>

const char sys_dir[] = "/sys/class/backlight";

bool read_file_int(const char* filename, int* value) {
    FILE* f = fopen(filename, "r");
    if (f==NULL) return false;
    int r = fscanf(f, "%d", value);
    fclose(f);
    return (r>0);
}

bool write_file_int(const char* filename, int value) {
    FILE* f = fopen(filename, "w");
    if (f==NULL) return false;
    int r = fprintf(f, "%d", value);
    bool ok = (r>0);
    if (fclose(f) != 0) ok = false;
    return ok;
}

bool get_only_subdir(const char* dirname, char* buf) {
    DIR* d = opendir(dirname);
    bool ret = false;
    int num_entries = 0;
    for (;;) {
        struct dirent* ent = readdir(d);
        if (!ent) break;
        if (ent->d_name[0] == '.') continue;  // skip ., ..
        if (++num_entries > 1) {
            // not unique
            break;
        }
        strncpy(buf, ent->d_name, sizeof(ent->d_name));
    }
    closedir(d);
    return (num_entries==1);
}

typedef enum { GET, SET, INC } op_t;

int main(int argc, char** argv) {
    char* backlight_name = NULL;
    op_t op = GET;
    double current = 0;
    double value = 0;
    char display_dir[PATH_MAX] = "";
    bool verbose = false;
    int opt;

    while ((opt = getopt(argc, argv, "d:gs:i:v")) != -1) {
        switch(opt) {
            case 'd':
                strncpy(display_dir, optarg, PATH_MAX - 1);
                break;
            case 'g':
                op = GET;
                break;
            case 's':
                op = SET;
                value = atof(optarg);
                if (value<0 || value>1) {
                    fprintf(stderr, "brightness should be between 0 and 1");
                    exit(2);
                }
                break;
            case 'i':
                op = INC;
                value = atof(optarg);
                if (value<-1 || value>1) {
                    fprintf(stderr, "brightness delta should be between -1 and 1");
                    exit(2);
                }
                break;
            case 'v':
                verbose = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d <display_dir>] [-g] [-s <set_value>] [-i <inc_value>] [-v]\n", argv[0]);
                exit(2);
        }
    }
    // find the subdir under /sys/class/backlight
    if (!display_dir[0]) {
        if (!get_only_subdir(sys_dir, display_dir)) {
            fprintf(stderr, "%s should contain exactly one subdir, or set '-d'\n", sys_dir);
            exit(1);
        }
        if (verbose) {
            printf("display_dir=%s\n", display_dir);
        }
    }
    // filenames
    char fn_brightness[PATH_MAX];
    snprintf(fn_brightness, PATH_MAX, "%s/%s/brightness", sys_dir, display_dir);
    char fn_max_brightness[PATH_MAX];
    snprintf(fn_max_brightness, PATH_MAX, "%s/%s/max_brightness", sys_dir, display_dir);

    int brightness, max_brightness;

    if (!read_file_int(fn_max_brightness, &max_brightness)) {
        perror("reading max_brightness");
        exit(1);
    }
    if (verbose) {
        printf("max_brightness=%d\n", max_brightness);
    }

    if (op == GET || op == INC) {
        if (!read_file_int(fn_brightness, &brightness)) {
            perror("reading brightness");
            exit(1);
        }
        if (verbose) {
            printf("brightness=%d\n", brightness);
        }
        if (op == GET) {
            printf("%.3g\n", (double)brightness/max_brightness);
            return 0;
        }
    }
    if (op == INC) {
        value += (double)brightness/max_brightness;
        if (value<0) value = 0;
        if (value>1) value = 1;
    }
    brightness = (int)(value * max_brightness + 0.5);
    if (verbose) {
        printf("new brightness=%d\n", brightness);
    }
    if (!write_file_int(fn_brightness, brightness)) {
        perror("writing brightness");
        exit(1);
    }
    return 0;
}
