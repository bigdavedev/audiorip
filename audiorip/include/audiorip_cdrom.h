#ifndef AUDIORIP_CDROM_H
#define AUDIORIP_CDROM_H

#include <linux/cdrom.h>

/**
 * @brief Open the file descriptor for the CDROM drive
 *
 * @param device Location of the CDROM drive
 * @return file descriptor on success, otherwise -1
 */
int audiorip_cdrom_open(char const* device);

/**
 * @brief Close the file descriptor for the CDROM drive
 *
 * @param fd file descriptor for CDROM drive
 * @return 0 on success, otherwise -1
 */
int audiorip_cdrom_close(int fd);

/**
 * @brief Start spinning up the CDROM drive
 *
 * @param fd file descriptor for CDROM drive
 * @return 0 on success, otherwise -1
 */
int audiorip_cdrom_spinup(int fd);

/**
 * @brief Stops the CDROM drive
 *
 * @param fd filedescriptor for CDROM drive
 * @return 0 on success, otherwise -1
 */
int audiorip_cdrom_spindown(int fd);

/**
 * @brief Extract the TOC Header from CDROM
 *
 * @param fd file descriptor for CDROM drive
 * @param toc_header where to store the contents read from
 *        the CDROM drive
 *
 * @return the result of the ioctl command
 */
int audiorip_cdrom_read_toc_header(int fd, struct cdrom_tochdr* toc_header);

/**
 * @brief Extract the TOC Entry from CDROM
 *
 * @param fd file descriptor for CDROM drive
 * @param toc_entry where to store the contents read from
 *        the CDROM drive
 *
 * @return the result of the ioctl command
 */
int audiorip_cdrom_read_toc_entry(int fd, struct cdrom_tocentry* toc_entry);

/**
 * @brief Extract the PCM data from CDROM
 *
 * @param fd file descriptor for CDROM drive
 * @param read_audio where to store the contents read from
 *        the CDROM drive
 *
 * @return the result of the ioctl command
 */
int audiorip_cdrom_read_audio(int fd, struct cdrom_read_audio* read_audio);

#endif
