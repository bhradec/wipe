#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <argp.h>

#include <sys/stat.h>
#include <sys/types.h>

#define OUTPUT_COLOR_RED "\033[0;31m"
#define OUTPUT_COLOR_NORMAL "\033[0m"

#define METHOD_ZERO "zero"
#define METHOD_ONE "one"
#define METHOD_RAND "rand"
#define METHOD_GUTMANN "gutmann"

const char *argp_program_version = "wipe 1.0";
const char *argp_program_bug_address = "<bhradec@gmail.com>";

static char doc[] = "wipe -- a secure erasure utility";
static char args_doc[] = "INPUT_FILE_PATH";

static struct argp_option options[] = {
    { "method", 'm', "METHOD", 0, "Method of secure erasure" },
    { "passes", 'p', "PASSES", 0, "Number of overwrite passes" },
    { "verbose", 'v', 0, 0, "Print progress messages" },
    { "unlink", 'u', 0, 0, "Remove the file after overwriting" },
    { 0 }
};

struct arguments {
    /* Input file path is the only arg without a flag. */
    char *args[1];
    char *method;
    int passes;
    int verbose;
    int unlink;
};

static int parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'm':
            arguments->method = arg;
            break;
        case 'p':
            arguments->passes = atoi(arg);
            break;
        case 'v':
            arguments->verbose = 1;
            break;
        case 'u':
            arguments->unlink = 1;
            break;
        case ARGP_KEY_ARG:
            /* Check if too many args without a flag. */
            if (state->arg_num > 1) { 
                argp_usage(state); 
            }
            /* Input file path is the only arg without
             * a flag. It is stored in the args array
             * for the args without flags. */
            arguments->args[state->arg_num] = arg;
            break;
        case ARGP_KEY_END:
            /* Check if not enough args. */
            if (state->arg_num < 1) { 
                argp_usage(state); 
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

void print_error(int print_errno, char *message) {
    fprintf(stderr, OUTPUT_COLOR_RED);
    fprintf(stderr, "Error: %s\n", message);

    if (print_errno != 0) {
        fprintf(stderr, "Descr: %s (errno: %d)\n", strerror(errno), errno);
    }

    fprintf(stderr, OUTPUT_COLOR_NORMAL);
}

void print_verbose(int verbose, char *format, ...) {
    va_list argp;
    
    if (verbose) {
        va_start(argp, format);
        vprintf(format, argp);
        va_end(argp);
    }
}

void print_error_verbose(int verbose, int print_errno, char *message) {
    if (verbose) {
        print_error(print_errno, message);
    }
}

void *generate_buffer(int size_in_bytes, unsigned char value) {
    void *buffer;

    /* Buffer must be aligned for direct I/O. */
    if (posix_memalign(&buffer, size_in_bytes, size_in_bytes)) {
        return NULL;
    }

    memset(buffer, value, size_in_bytes);

    return buffer;
}

void *generate_random_buffer(int size_in_bytes) {
    void *buffer;
    int urandom_fd;

    urandom_fd = open("/dev/urandom", O_RDONLY);

    if (urandom_fd < 0) { 
        return NULL; 
    }

    /* Buffer must be aligned for direct I/O. */
    if (posix_memalign(&buffer, size_in_bytes, size_in_bytes)) {
        return NULL;
    }

    if (read(urandom_fd, buffer, size_in_bytes) < 0) {
        return NULL;
    }

    close(urandom_fd);

    return buffer;
}

int overwrite(char *path, char *method, int passes, int verbose) {
    int fd;
    int buffer_size;
    int file_size;
    int full_buffers;
    int remainder;
    int i, j;
    void *buffer;
    struct stat file_stat;

    fd = open(path, O_WRONLY);

    if (fd < 0) {
        print_error_verbose(verbose, 1, "Can't open file"); 
        return -1;
    }

    if (fstat(fd, &file_stat) < 0) {
        print_error_verbose(verbose, 1, "Can't get file stats");
        return -1;
    }

    buffer_size = file_stat.st_blksize;
    file_size = file_stat.st_size;

    full_buffers = file_size / buffer_size;
    remainder = file_size % buffer_size;

    if (strcmp(method, METHOD_ZERO) == 0) {
        buffer = generate_buffer(buffer_size, 0);
    } else if (strcmp(method, METHOD_ONE) == 0) {
        buffer = generate_buffer(buffer_size, 0xff);
    } else if (strcmp(method, METHOD_RAND) == 0) {
        buffer = generate_random_buffer(buffer_size);
    } else {
        print_error_verbose(verbose, 0, "Given method does not exist");
        return -1;
    }

    print_verbose(verbose, "Starting overwrite\n");
    
    for (i = 0; i < passes; i++) {
        print_verbose(verbose, "Starting pass %d/%d\n", i + 1, passes);
        print_verbose(verbose, "Rewinding file\n");

        if (lseek(fd, 0, SEEK_SET) < 0) {
            print_error_verbose(verbose, 1, "Can't rewind file");
            return -1;
        }

        for (j = 0; j < full_buffers; j++) {
            print_verbose(verbose, "Writing block %d/%d\n", j + 1, full_buffers);

            if (write(fd, buffer, buffer_size) < 1) {
                print_error_verbose(verbose, 1, "Can't write buffer");
                return -1;
            }
        }

        print_verbose(verbose, "Writing remainder\n");
        
        if (write(fd, buffer, remainder) < 0) {
            print_error_verbose(verbose, 1, "Can't write remainder");
            return -1;
        }

        print_verbose(verbose, "Flushing to disc\n");

        if (fsync(fd) < 0) {
            print_error_verbose(verbose, 1, "Can't flush data to disc");
            return -1;
        }
    }

    print_verbose(verbose, "Overwrite finished\n");

    free(buffer);
    fsync(fd);
    close(fd);

    return 0;
}

int main(int argc, char **argv) {
    char *file_path;
    struct arguments args;

    /* Default arg values if not specified in args. */
    args.method = METHOD_ZERO;
    args.passes = 1;
    args.verbose = 0;
    args.unlink = 0;

    argp_parse(&argp, argc, argv, 0, 0, &args);

    file_path = args.args[0];

    if (args.passes <= 0) {
        print_error(0, "Incorrect number of passes");
        return -1;
    }

    if (strcmp(args.method, METHOD_ZERO) 
            && strcmp(args.method, METHOD_ONE) 
            && strcmp(args.method, METHOD_RAND)) {

        print_error(0, "Given method does not exist");
        return -1;
    }

    if (overwrite(file_path, args.method, args.passes, args.verbose) < 0) {
        print_error(0, "Secure erasure unsuccessful");
        return -1;
    } 

    printf("Unmount the device to force sync.\n");

    if (args.unlink) {
        print_verbose(args.verbose, "Unlinking file\n");
        if (unlink(file_path) < 0) {
            print_error(1, "Unlinking file unsuccessful");
            return -1;
        }
    }

    return 0;
}
