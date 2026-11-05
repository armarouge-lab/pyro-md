#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define VERSION "0.1.0"
#define MAX_PATH_LENGTH 1024

static void show_usage(void) { fprintf(stderr, "usage: pyro ~/path/to/file...\n"); exit(EXIT_FAILURE); }
static void show_help(void) {
    printf("Usage: pyro ~/path/to/file...\n\nOpens FILE with the appropriate viewer:\n  video/*  -> mpv\n  image/*  -> imv\n  PDF      -> zathura\n  others   -> xdg-open\n\nOptions:\n  -h, --help    show this help and exit\n  --version     show version and exit\n");
    exit(EXIT_SUCCESS);
}

static const char *detect_mime_type(const char *file_path) {
    unsigned char header_buffer[16];
    int file_descriptor = open(file_path, O_RDONLY);
    if (file_descriptor < 0) return "application/octet-stream";
    ssize_t bytes_read = read(file_descriptor, header_buffer, sizeof(header_buffer));
    close(file_descriptor);
    if (bytes_read < 4) return "application/octet-stream";
    if (bytes_read >= 3 && header_buffer[0] == 0xFF && header_buffer[1] == 0xD8 && header_buffer[2] == 0xFF) return "image/jpeg";
    if (bytes_read >= 8 && memcmp(header_buffer, "\x89PNG\r\n\x1a\n", 8) == 0) return "image/png";
    if (memcmp(header_buffer, "GIF8", 4) == 0) return "image/gif";
    if (memcmp(header_buffer, "%PDF", 4) == 0) return "application/pdf";
    if (bytes_read >= 8 && memcmp(header_buffer + 4, "ftyp", 4) == 0) return "video/mp4";
    return "application/octet-stream";
}

static int command_exists(const char *command_name) {
    const char *path_environment = getenv("PATH");
    if (!path_environment) return 0;
    char candidate_path[MAX_PATH_LENGTH];
    const char *path_iterator = path_environment;
    while (*path_iterator) {
        const char *colon_position = strchr(path_iterator, ':');
        size_t directory_length = colon_position ? (size_t)(colon_position - path_iterator) : strlen(path_iterator);
        if (directory_length > 0 && directory_length + 1 + strlen(command_name) + 1 < sizeof(candidate_path)) {
            snprintf(candidate_path, sizeof(candidate_path), "%.*s/%s", (int)directory_length, path_iterator, command_name);
            if (access(candidate_path, X_OK) == 0) return 1;
        }
        if (!colon_position) break;
        path_iterator = colon_position + 1;
    }
    return 0;
}

static int execute_viewer(const char *mime_type, const char *target_path) {
    if (strncmp(mime_type, "video/", 6) == 0) {
        if (!command_exists("mpv")) { fprintf(stderr, "pyro: mpv not found\n"); return EXIT_FAILURE; }
        printf("Opening video: %s\n", target_path); fflush(stdout);
        execlp("mpv", "mpv", "--no-border", "--really-quiet", target_path, (char *)NULL);
        return EXIT_FAILURE;
    }
    if (strcmp(mime_type, "image/gif") == 0) {
        if (!command_exists("mpv")) { fprintf(stderr, "pyro: mpv not found\n"); return EXIT_FAILURE; }
        printf("Opening GIF: %s\n", target_path); fflush(stdout);
        execlp("mpv", "mpv", "--loop", "--no-border", target_path, (char *)NULL);
        return EXIT_FAILURE;
    }
    if (strncmp(mime_type, "image/", 6) == 0) {
        const char *viewer_name = NULL;
        if (command_exists("imv")) viewer_name = "imv";
        else if (command_exists("imv-x11")) viewer_name = "imv-x11";
        else if (command_exists("imv-wayland")) viewer_name = "imv-wayland";
        if (!viewer_name) { fprintf(stderr, "pyro: imv not found\n"); return EXIT_FAILURE; }
        printf("Opening image: %s\n", target_path); fflush(stdout);
        execlp(viewer_name, viewer_name, target_path, (char *)NULL);
        return EXIT_FAILURE;
    }
    if (strcmp(mime_type, "application/pdf") == 0) {
        if (!command_exists("zathura")) { fprintf(stderr, "pyro: zathura not found\n"); return EXIT_FAILURE; }
        printf("Opening PDF: %s\n", target_path); fflush(stdout);
        execlp("zathura", "zathura", target_path, (char *)NULL);
        return EXIT_FAILURE;
    }
    if (!command_exists("xdg-open")) { fprintf(stderr, "pyro: xdg-open not found\n"); return EXIT_FAILURE; }
    printf("Opening with xdg-open: %s\n", target_path); fflush(stdout);
    execlp("xdg-open", "xdg-open", target_path, (char *)NULL);
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    if (argc < 2) show_usage();
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) show_help();
    if (strcmp(argv[1], "--version") == 0) { printf("pyro %s\n", VERSION); return EXIT_SUCCESS; }
    const char *target_path = argv[1];
    struct stat file_status;
    if (stat(target_path, &file_status) != 0) { fprintf(stderr, "pyro: '%s' does not exist\n", target_path); return EXIT_FAILURE; }
    if (!S_ISREG(file_status.st_mode)) { fprintf(stderr, "pyro: '%s' is not a regular file\n", target_path); return EXIT_FAILURE; }
    const char *detected_mime_type = detect_mime_type(target_path);
    return execute_viewer(detected_mime_type, target_path);
}
