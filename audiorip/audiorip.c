#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

struct track_address
{
    int start;
    int end;

    union cdrom_addr cdrom_addr;
};

static int cdrom_fd = -1;

static void sigint_handler(int sig);
static void spindown_and_close(int fd);

static int get_num_tracks(int fd, int verbose);
static int get_track_addresses(int fd,
                               struct track_address* addresses,
                               int num_tracks,
                               int verbose);
static int rip_track_to_file(int fd,
                             struct track_address address,
                             char const* filename,
                             int verbose);

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

    ioctl(cdrom_fd, CDROMSTART);

    /* Get table of contents header to determine number of tracks */
    int num_tracks = get_num_tracks(cdrom_fd, arguments.verbose);

    struct track_address *addresses = (struct track_address*)malloc(num_tracks * sizeof(struct track_address));
    if (get_track_addresses(cdrom_fd, addresses, num_tracks, arguments.verbose) < 0)
    {
        free(addresses);
        spindown_and_close(cdrom_fd);
        return -1;
    }

    rip_track_to_file(cdrom_fd, addresses[0], "track1.pcm", arguments.verbose);

    free(addresses);
    ioctl(cdrom_fd, CDROMSTOP);
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

static void spindown_and_close(int fd)
{
    ioctl(fd, CDROMSTOP);
    close(fd);
}

static int get_num_tracks(int fd, int verbose)
{
    struct cdrom_tochdr toc_header;
    if (ioctl(fd, CDROMREADTOCHDR, &toc_header) < 0)
    {
        fprintf(stderr, "Failed to read ToC header\n");
        spindown_and_close(fd);
        return -1;
    }

    if (verbose)
    {
        fprintf(stdout, "Number of tracks on CD: %d\n", toc_header.cdth_trk1);
    }

    return toc_header.cdth_trk1;
}

static int get_track_addresses(int fd,
                               struct track_address* addresses,
                               int num_tracks,
                               int verbose)
{
    for (int i = 1; i <= num_tracks; ++i)
    {
        struct cdrom_tocentry current_track =
        {
            .cdte_track = i,
            .cdte_format = CDROM_MSF
        };

        struct cdrom_tocentry next_track =
        {
            .cdte_track = i == num_tracks ? CDROM_LEADOUT : i+1,
            .cdte_format = CDROM_MSF
        };

        if (ioctl(fd, CDROMREADTOCENTRY, &current_track) < 0)
        {
            fprintf(stderr, "Failed to read ToC entry for track: %d\n", i);
            return -1;
        }

        if (ioctl(fd, CDROMREADTOCENTRY, &next_track) < 0)
        {
            if (next_track.cdte_track == CDROM_LEADOUT)
            {
                fprintf(stderr, "Failed to read ToC entry for leadout track\n");
            }
            else
            {
                fprintf(stderr, "Failed to read ToC entry for track: %d\n", i+1);
            }
            return -1;
        }

        addresses[i-1].start = current_track.cdte_addr.msf.frame
                           + (current_track.cdte_addr.msf.minute * CD_FRAMES * 3600)
                           + (current_track.cdte_addr.msf.second * CD_FRAMES)
                           - CD_MSF_OFFSET;
        addresses[i-1].end = next_track.cdte_addr.msf.frame
                         + (next_track.cdte_addr.msf.minute * CD_FRAMES * 3600)
                         + (next_track.cdte_addr.msf.second * CD_FRAMES)
                         - CD_MSF_OFFSET;
        addresses[i-1].cdrom_addr = current_track.cdte_addr;
        if (verbose)
        {
            fprintf(stdout, "Track %d is %d frames long\n", i, addresses[i-1].end - addresses[i-1].start);
        }
    }
    return 0;
}

static int rip_track_to_file(int fd,
                             struct track_address address,
                             char const* filename,
                             int verbose)
{
    int const readframes = address.end - address.start;
    unsigned char buffer[CD_FRAMES * CD_FRAMESIZE_RAW];
    struct cdrom_read_audio read_audio =
    {
        .addr        = address.cdrom_addr,
        .addr_format = CDROM_MSF,
        .nframes     = CD_FRAMES,
        .buf         = &buffer[0]
    };

    if (verbose)
    {
        fprintf(stdout, "Reading track from %d to %d\n", address.start, address.end);
    }

    FILE* out = fopen(filename, "wb");
    for (int chunk = 0; chunk < readframes; chunk += CD_FRAMES)
    {
        if (ioctl(fd, CDROMREADAUDIO, &read_audio) < 0)
        {
            fprintf(stderr, "Failed to read chunk\n");
            fprintf(stderr, "%s\n", strerror(errno));
            fclose(out);
            return -1;
        }

        fwrite(buffer, (size_t)CD_FRAMESIZE_RAW, (size_t)read_audio.nframes, out);

        read_audio.addr.msf.frame += read_audio.nframes;
        if (read_audio.addr.msf.frame >= CD_FRAMES)
        {
            read_audio.addr.msf.second += read_audio.addr.msf.frame / CD_FRAMES;
            read_audio.addr.msf.frame = read_audio.addr.msf.frame % CD_FRAMES;
            if (read_audio.addr.msf.second >= 60)
            {
                read_audio.addr.msf.minute += read_audio.addr.msf.second / 60;
                read_audio.addr.msf.second = read_audio.addr.msf.second % 60;
            }
        }
    }
    return 0;
}
