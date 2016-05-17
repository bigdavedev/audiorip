#include <fcntl.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <argp.h>
#include <stdio.h>

const char* argp_program_version     = "0.1";
const char* argp_program_bug_address = "<d.brown@bigdavedev.com>";
static char doc[]                    = "Spin down the CDROM device";

static struct argp_option options[] = {
    { "device", 'd', "cdrom", 0, "Specify which CDROM device to use" },
    { 0 }
};

static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = { options, parse_opt, NULL, doc };

static char* device = NULL;

int main(int argc, char* argv[])
{
    device = "/dev/sr0";
    argp_parse(&argp, argc, argv, 0, 0, device);

    int cdrom = open((char const*)device, O_RDONLY | O_NONBLOCK);
    if (cdrom < 0)
    {
        fprintf(stderr, "Failed to open CDROM\n");
        return -1;
    }
    ioctl(cdrom, CDROMSTOP);
    close(cdrom);
    return 0;
}

static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
    char* device = state->input;

    switch (key)
    {
    case 'd':
        device = arg;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}
