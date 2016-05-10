#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include <stdio.h>
#include <signal.h>

/**
 * Stuff for argp
 */
#include <argp.h>

const char *argp_program_version = "0.1";
const char *argp_program_bug_address = "<d.brown@bigdavedev.com>";
static char doc[] = "Rip an audio CD to PCM";

static struct argp_option options[] =
{
    { "verbose",  'v', 0,       0,  "Produce verbose output" },
    { "quiet",    'q', 0,       0,  "Don't produce any output" },
    { "device",   'd', "cdrom", 0,  "Specify which CDROM device to use" },
    { 0 }
};

struct arguments
{
    int  verbose;
    int  quiet;
    char *device;
};
static error_t parse_opt (int key, char *arg, struct argp_state *state);
static struct argp argp = { options, parse_opt, NULL, doc };
/**
 * end argp section
 */

static int cdrom_fd = -1;

static void sigint_handler(int sig);

int main(int argc, char* argv[])
{
    signal(SIGINT, sigint_handler);

    struct arguments arguments;
    arguments.verbose = 0;
    arguments.quiet   = 0;
    arguments.device  = "/dev/sr0";

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    cdrom_fd = open((char const*)arguments.device, O_RDONLY | O_NONBLOCK);
    if (cdrom_fd < 0)
    {
        fprintf(stderr, "Failed to open CDROM\n");
        return -1;
    }
    close(cdrom_fd);
    return 0;
}

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key)
    {
    case 'q':
        arguments->quiet = 1;
        break;
    case 'v':
        arguments->verbose = 1;
        break;
    case 'd':
        arguments->device = arg;
        break;
    default:
      return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static void sigint_handler(int sig)
{
    ioctl(cdrom_fd, CDROMSTOP);
    close(cdrom_fd);
}
