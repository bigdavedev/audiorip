#include <audiorip.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void audiorip_spindown_and_close(int fd)
{
    ioctl(fd, CDROMSTOP);
    close(fd);
}

int audiorip_get_num_tracks(int fd, int verbose)
{
    struct cdrom_tochdr toc_header;
    if (ioctl(fd, CDROMREADTOCHDR, &toc_header) < 0)
    {
        fprintf(stderr, "Failed to read ToC header\n");
        audiorip_spindown_and_close(fd);
        return -1;
    }

    if (verbose)
    {
        fprintf(stdout, "Number of tracks on CD: %d\n", toc_header.cdth_trk1);
    }

    return toc_header.cdth_trk1;
}

static int msf_to_frames(union cdrom_addr const entry)
{
    int seconds = (entry.msf.minute * 60) + entry.msf.second;
    int frames = (seconds * CD_FRAMES) + entry.msf.frame;
    return frames - CD_MSF_OFFSET;
}

int audiorip_get_track_addresses(int fd,
                                 struct track_address *const addresses,
                                 int const num_tracks,
                                 int verbose)
{
    for (int i = 1; i <= num_tracks; ++i)
    {
        struct cdrom_tocentry current_track =
        {
            .cdte_track = i,
            .cdte_format = CDROM_MSF
        };

        if (ioctl(fd, CDROMREADTOCENTRY, &current_track) < 0)
        {
            fprintf(stderr, "Failed to read ToC entry for track: %d\n", i);
            audiorip_spindown_and_close(fd);
            return -1;
        }

        /**
         * In order to ascertain the length of the current track, we need
         * to know the address of the next track in the list.  Should the
         * current track be the last one, we simply request the address
         * of the Leadout track.
         */
        struct cdrom_tocentry next_track =
        {
            .cdte_track = i == num_tracks ? CDROM_LEADOUT : i+1,
            .cdte_format = CDROM_MSF
        };

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
            audiorip_spindown_and_close(fd);
            return -1;
        }

        addresses[i-1].start      = msf_to_frames(current_track.cdte_addr);
        addresses[i-1].end        = msf_to_frames(next_track.cdte_addr);
        addresses[i-1].cdrom_addr = current_track.cdte_addr;
        if (verbose)
        {
            fprintf(stdout, "Track %d is %d frames long\n", i, addresses[i-1].end - addresses[i-1].start);
        }
    }
    return 0;
}

unsigned char const* audiorip_rip_track(int fd,
                                        struct track_address const address,
                                        int verbose)
{
    int const readframes = address.end - address.start;
    unsigned char *buffer = malloc(readframes * CD_FRAMESIZE_RAW);
    unsigned char interim_buffer[CD_FRAMES * CD_FRAMESIZE_RAW];
    struct cdrom_read_audio read_audio =
    {
        .addr        = address.cdrom_addr,
        .addr_format = CDROM_MSF,
        .nframes     = CD_FRAMES,
        .buf         = &interim_buffer[0]
    };

    if (verbose)
    {
        fprintf(stdout, "Reading track from %d to %d\n", address.start, address.end);
    }

    for (int chunk = 0; chunk < readframes; chunk += read_audio.nframes)
    {
        if ((CD_FRAMES + chunk) > readframes)
        {
            read_audio.nframes = readframes - chunk;
        }

        if (ioctl(fd, CDROMREADAUDIO, &read_audio) < 0)
        {
            fprintf(stderr, "Failed to read chunk\n");
            fprintf(stderr, "%s\n", strerror(errno));
            audiorip_spindown_and_close(fd);
            return NULL;
        }

        memcpy(buffer + (chunk * CD_FRAMESIZE_RAW),
               interim_buffer,
               read_audio.nframes * CD_FRAMESIZE_RAW);

        if (verbose)
        {
            fprintf(stdout, "progress %d/%d\n", chunk, readframes);
        }

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

    return buffer;
}

int audiorip_rip_track_to_file(int fd,
                               struct track_address const address,
                               char const* filename,
                               int verbose)
{
    FILE *out = fopen(filename, "wb");
    unsigned char const* track_data = audiorip_rip_track(fd, address, verbose);
    if (track_data == NULL)
    {
        fclose(out);
        return -1;
    }
    fwrite((void const*)track_data,
           (size_t)CD_FRAMESIZE_RAW,
           (size_t)(address.end - address.start),
           out);
    fclose(out);
    audiorip_free_track(track_data);
    return 0;
}

void audiorip_free_track(unsigned char const* buffer)
{
    free((void*)buffer);
}
