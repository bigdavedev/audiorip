#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <audiorip.h>
#include <audiorip_cdrom.h>

/**
 * Stuff for argp
 */
#include <argp.h>

const char *argp_program_version = "0.1";
const char *argp_program_bug_address = "<d.brown@bigdavedev.com>";
static char doc[] = "Rip an audio CD to PCM";

static struct argp_option options[] =
{
    { "verbose",  'v', 0,         0,  "Produce verbose output" },
    { "quiet",    'q', 0,         0,  "Don't produce any output" },
    { "device",   'd', "cdrom",   0,  "Specify which CDROM device to use" },
    { "format",   'f', "WAV|PCM", 0,  "Specify the output file format" },
    { 0 }
};

struct arguments
{
    int  verbose;
    int  quiet;
    char *device;
    char *format;
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
    arguments.format  = "wav";

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    cdrom_fd = audiorip_cdrom_open((char const*)arguments.device);
    if (cdrom_fd < 0)
    {
        fprintf(stderr, "Failed to open CDROM\n");
        return -1;
    }

    audiorip_cdrom_spinup(cdrom_fd);

    /* Get table of contents header to determine number of tracks */
    int num_tracks = audiorip_get_num_tracks(cdrom_fd, arguments.verbose);

    struct track_address *addresses = (struct track_address*)malloc(num_tracks * sizeof(struct track_address));
    if (audiorip_get_track_addresses(cdrom_fd, addresses, num_tracks, arguments.verbose) < 0)
    {
        free(addresses);
        audiorip_spindown_and_close(cdrom_fd);
        return -1;
    }

    for (int i = 0; i < num_tracks; ++i)
    {
        char track_name[32] = {'\0'};
        snprintf(track_name, sizeof(track_name), "track%d.%s", i+1, arguments.format);
        fprintf(stdout, "Processing %s\n", track_name);
        audiorip_rip_track_to_file(cdrom_fd, addresses[i], track_name, arguments.verbose);
    }

    free(addresses);
    audiorip_spindown_and_close(cdrom_fd);
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
    case 'f':
        for (size_t i = 0; arg[i]; ++i)
        {
            arg[i] = tolower(arg[i]);
        }
        arguments->format = arg;
        break;
    default:
      return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static void sigint_handler(int sig)
{
    audiorip_spindown_and_close(cdrom_fd);
    exit(sig);
}
