#ifndef AUDIORIP_H
#define AUDIORIP_H

#include <linux/cdrom.h>

struct track_address
{
    int start;
    int end;

    union cdrom_addr cdrom_addr;
};

void audiorip_spindown_and_close(int fd);

int audiorip_get_num_tracks(int fd, int verbose);
int audiorip_get_track_addresses(int fd,
                                 struct track_address *const addresses,
                                 int num_tracks,
                                 int verbose);

unsigned char const* audiorip_rip_track(int fd,
                                        struct track_address const address,
                                        int verbose);
int audiorip_rip_track_to_file(int fd,
                               struct track_address const address,
                               char const* filename,
                               int verbose);

void audiorip_free_track(unsigned char const* buffer);

#endif
