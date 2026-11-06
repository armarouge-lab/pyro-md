#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <libgen.h>

#define VERSION "0.4.0"
#define MAX_PATH_LENGTH 1024
#define HEADER_BUFFER_SIZE 512

#define ICON_PATH_SYSTEM "/usr/local/share/pyro/cd-icon.png"
#define ICON_PATH_LOCAL "res/cd-icon.png"

static void show_usage(void) {
    fprintf(stderr, "usage: pyro ~/path/to/file\n");
    exit(EXIT_FAILURE);
}

static void show_help(void) {
    printf("Usage: pyro ~/path/to/file\n\n");
    printf("Opens FILE with the appropriate viewer:\n");
    printf("  video/*  -> mpv\n");
    printf("  audio/*  -> mpv (with CD icon)\n");
    printf("  image/*  -> imv\n");
    printf("  PDF      -> zathura\n");
    printf("  text/*   -> interactive prompt\n");
    printf("  others   -> xdg-open\n\n");
    printf("Options:\n");
    printf("  -h, --help    show this help and exit\n");
    printf("  --version     show version and exit\n");
    exit(EXIT_SUCCESS);
}

static int has_text_extension(const char *file_path) {
    const char *extensions[] = {
        ".txt", ".md", ".c", ".h", ".cpp", ".hpp", ".rs", ".py", 
        ".sh", ".zsh", ".conf", ".toml", ".json", ".xml", ".html", 
        ".css", ".js", ".ts", ".lua", ".vim", ".ini", ".yaml", ".yml"
    };
    int num_extensions = sizeof(extensions) / sizeof(extensions[0]);
    const char *dot = strrchr(file_path, '.');
    if (!dot) return 0;

    for (int i = 0; i < num_extensions; i++) {
        if (strcasecmp(dot, extensions[i]) == 0) return 1;
    }
    return 0;
}

static int looks_like_text(const char *file_path) {
    unsigned char buffer[HEADER_BUFFER_SIZE];
    int file_descriptor = open(file_path, O_RDONLY);
    if (file_descriptor < 0) return 0;

    ssize_t bytes_read = read(file_descriptor, buffer, sizeof(buffer));
    close(file_descriptor);
    if (bytes_read <= 0) return 0;

    for (ssize_t i = 0; i < bytes_read; i++) {
        if (buffer[i] == 0) return 0;
        if (!isprint(buffer[i]) && !isspace(buffer[i])) return 0;
    }
    return 1;
}

static const char *detect_mime_type(const char *file_path) {
    if (has_text_extension(file_path)) return "text/plain";

    unsigned char header_buffer[16];
    int file_descriptor = open(file_path, O_RDONLY);
    if (file_descriptor < 0) return "application/octet-stream";
    
    ssize_t bytes_read = read(file_descriptor, header_buffer, sizeof(header_buffer));
    close(file_descriptor);
    if (bytes_read < 4) return "application/octet-stream";

    if (bytes_read >= 3 && header_buffer[0] == 0xFF && header_buffer[1] == 0xD8 && header_buffer[2] == 0xFF) return "image/jpeg";
    if (bytes_read >= 8 && memcmp(header_buffer, "\x89PNG\r\n\x1a\n", 8) == 0) return "image/png";
    if (memcmp(header_buffer, "GIF8", 4) == 0) return "image/gif";
    if (bytes_read >= 12 && memcmp(header_buffer, "RIFF", 4) == 0 && memcmp(header_buffer + 8, "WEBP", 4) == 0) return "image/webp";
    if (memcmp(header_buffer, "BM", 2) == 0) return "image/bmp";
    
    if (bytes_read >= 3 && memcmp(header_buffer, "ID3", 3) == 0) return "audio/mpeg";
    if (bytes_read >= 2 && header_buffer[0] == 0xFF && (header_buffer[1] & 0xE0) == 0xE0) return "audio/mpeg";
    if (memcmp(header_buffer, "fLaC", 4) == 0) return "audio/flac";
    if (memcmp(header_buffer, "OggS", 4) == 0) return "audio/ogg";
    if (bytes_read >= 12 && memcmp(header_buffer, "RIFF", 4) == 0 && memcmp(header_buffer + 8, "WAVE", 4) == 0) return "audio/wav";

    if (bytes_read >= 4 && memcmp(header_buffer, "\x1A\x45\xDF\xA3", 4) == 0) return "video/x-matroska";
    if (bytes_read >= 12 && memcmp(header_buffer, "RIFF", 4) == 0 && memcmp(header_buffer + 8, "AVI ", 4) == 0) return "video/x-msvideo";
    if (bytes_read >= 8 && memcmp(header_buffer + 4, "ftyp", 4) == 0) return "video/mp4";

    if (memcmp(header_buffer, "%PDF", 4) == 0) return "application/pdf";
    if (looks_like_text(file_path)) return "text/plain";

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

static void prompt_and_open_text_file(const char *target_path) {
    char input_buffer[256];
    printf("Detected text/code file: %s\n", target_path);
    printf("[1] Open with Neovim\n");
    printf("[2] Open with custom application\n");
    printf("[3] Open with system default (xdg-open)\n");
    printf("Choice [1]: ");
    fflush(stdout);

    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) return;

    if (input_buffer[0] == '\n' || input_buffer[0] == '1') {
        if (command_exists("nvim")) execlp("nvim", "nvim", target_path, (char *)NULL);
        else if (command_exists("vim")) execlp("vim", "vim", target_path, (char *)NULL);
        else fprintf(stderr, "pyro: neither nvim nor vim found.\n");
    } 
    else if (input_buffer[0] == '2') {
        printf("Enter the application command: ");
        fflush(stdout);
        if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
            input_buffer[strcspn(input_buffer, "\n")] = 0;
            if (strlen(input_buffer) > 0) {
                execlp(input_buffer, input_buffer, target_path, (char *)NULL);
                perror("pyro: failed to launch custom application");
            }
        }
    } 
    else if (input_buffer[0] == '3') {
        if (command_exists("xdg-open")) execlp("xdg-open", "xdg-open", target_path, (char *)NULL);
        else fprintf(stderr, "pyro: xdg-open not found.\n");
    } 
    else fprintf(stderr, "pyro: invalid choice.\n");
}

static int execute_viewer(const char *mime_type, const char *target_path) {
    if (strncmp(mime_type, "video/", 6) == 0) {
        if (!command_exists("mpv")) return EXIT_FAILURE;
        printf("Opening video: %s\n", target_path); fflush(stdout);
        execlp("mpv", "mpv", "--no-border", "--really-quiet", "--ao=alsa", target_path, (char *)NULL);
        return EXIT_FAILURE;
    }
    if (strncmp(mime_type, "audio/", 6) == 0) {
        if (!command_exists("mpv")) return EXIT_FAILURE;
        printf("Opening audio: %s\n", target_path); fflush(stdout);
        
        const char *icon_to_use = NULL;
        if (access(ICON_PATH_SYSTEM, F_OK) == 0) {
            icon_to_use = ICON_PATH_SYSTEM;
        } else if (access(ICON_PATH_LOCAL, F_OK) == 0) {
            icon_to_use = ICON_PATH_LOCAL;
        }

        if (icon_to_use) {
            char path_copy[MAX_PATH_LENGTH];
            snprintf(path_copy, sizeof(path_copy), "%s", target_path);
            char *song_name = basename(path_copy);

            char audio_arg[MAX_PATH_LENGTH + 32];
            snprintf(audio_arg, sizeof(audio_arg), "--audio-file=%s", target_path);

            char title_arg[MAX_PATH_LENGTH + 32];
            snprintf(title_arg, sizeof(title_arg), "--title=%s", song_name);

            execlp("mpv", "mpv", 
                   "--force-window=yes", 
                   "--image-display-duration=inf", 
                   "--autofit=800x600",
                   "--panscan=1.0",
                   "--ao=alsa",
                   title_arg,
                   icon_to_use, 
                   audio_arg, 
                   "--really-quiet", 
                   (char *)NULL);
        } else {
            execlp("mpv", "mpv", "--force-window=yes", "--ao=alsa", "--really-quiet", target_path, (char *)NULL);
        }
        perror("pyro: mpv");
        return EXIT_FAILURE;
    }
    if (strcmp(mime_type, "image/gif") == 0) {
        if (!command_exists("mpv")) return EXIT_FAILURE;
        printf("Opening GIF: %s\n", target_path); fflush(stdout);
        execlp("mpv", "mpv", "--loop", "--no-border", target_path, (char *)NULL);
        return EXIT_FAILURE;
    }
    if (strncmp(mime_type, "image/", 6) == 0) {
        const char *viewer_name = NULL;
        if (command_exists("imv")) viewer_name = "imv";
        else if (command_exists("imv-x11")) viewer_name = "imv-x11";
        else if (command_exists("imv-wayland")) viewer_name = "imv-wayland";
        if (!viewer_name) return EXIT_FAILURE;
        printf("Opening image: %s\n", target_path); fflush(stdout);
        execlp(viewer_name, viewer_name, target_path, (char *)NULL);
        return EXIT_FAILURE;
    }
    if (strcmp(mime_type, "application/pdf") == 0) {
        if (!command_exists("zathura")) return EXIT_FAILURE;
        printf("Opening PDF: %s\n", target_path); fflush(stdout);
        execlp("zathura", "zathura", target_path, (char *)NULL);
        return EXIT_FAILURE;
    }
    if (strcmp(mime_type, "text/plain") == 0) {
        prompt_and_open_text_file(target_path);
        return EXIT_SUCCESS;
    }
    if (!command_exists("xdg-open")) return EXIT_FAILURE;
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

    if (stat(target_path, &file_status) != 0) {
        fprintf(stderr, "pyro: '%s' does not exist\n", target_path); return EXIT_FAILURE;
    }
    if (!S_ISREG(file_status.st_mode)) {
        fprintf(stderr, "pyro: '%s' is not a regular file\n", target_path); return EXIT_FAILURE;
    }

    return execute_viewer(detect_mime_type(target_path), target_path);
}