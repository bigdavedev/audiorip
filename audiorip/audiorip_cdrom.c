#include <audiorip_cdrom.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <unistd.h>

int audiorip_cdrom_open(char const* device)
{
    int fd = open(device, O_RDONLY | O_NONBLOCK);
    return fd;
}

int audiorip_cdrom_close(int fd)
{
    if (close(fd) != 0)
    {
        fprintf(stderr, "Failed to close CDROM file descriptor: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int audiorip_cdrom_spinup(int fd)
{
    if (ioctl(fd, CDROMSTART) < 0)
    {
        fprintf(stderr, "Failed to spinup CDROM: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int audiorip_cdrom_spindown(int fd)
{
    if (ioctl(fd, CDROMSTOP) < 0)
    {
        fprintf(stderr, "Failed to spindown CDROM: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int audiorip_cdrom_read_toc_header(int fd, struct cdrom_tochdr* toc_header)
{
    int result = ioctl(fd, CDROMREADTOCHDR, toc_header);
    return result;
}

int audiorip_cdrom_read_toc_entry(int fd, struct cdrom_tocentry* toc_entry)
{
    int result = ioctl(fd, CDROMREADTOCENTRY, toc_entry);
    return result;
}

int audiorip_cdrom_read_audio(int fd, struct cdrom_read_audio* read_audio)
{
    int result = ioctl(fd, CDROMREADAUDIO, &read_audio);
    return result;
}
