#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "tvhtime.h"

#define NTPD_BASE	0x4e545030	/* "NTP0" */
#define NTPD_UNIT	2

typedef struct
{
  int    mode; /* 0 - if valid set
				        *       use values, 
				        *       clear valid
				        * 1 - if valid set 
				        *       if count before and after read of values is equal,
				        *         use values 
				        *       clear valid
				        */
  int    count;
  time_t clockTimeStampSec;
  int    clockTimeStampUSec;
  time_t receiveTimeStampSec;
  int    receiveTimeStampUSec;
  int    leap;
  int    precision;
  int    nsamples;
  int    valid;
  int    pad[10];
} ntp_shm_t;

static ntp_shm_t *
ntp_shm_init ( void )
{
  int shmid, unit, mode;
  static ntp_shm_t *shmptr = NULL;

  if (shmptr != NULL)
    return shmptr;

  unit = getuid() ? 2 : 0;
  mode = getuid() ? 0666 : 0600;

  shmid = shmget((key_t)NTPD_BASE + unit, sizeof(ntp_shm_t), IPC_CREAT | mode);
  if (shmid == -1)
    return NULL;

  shmptr = shmat(shmid, 0, 0);
  memset(shmptr, 0, sizeof(ntp_shm_t));
  if (shmptr) {
    shmptr->mode      = 1;
    shmptr->precision = -1;
    shmptr->nsamples  = 1;
  }

  return shmptr;
}

void
tvhtime_update ( struct tm *tm )
{
  time_t now;
  struct timeval tv;
  ntp_shm_t *ntp_shm;

  if (!(ntp_shm = ntp_shm_init()))
    return;

  now = mktime(tm);
  gettimeofday(&tv, NULL);

#if NTP_TRACE
  int64_t t1, t2;
  t1 = now * 1000000;
  t2 = tv.tv_sec * 1000000 + tv.tv_usec;
  tvhlog(LOG_DEBUG, "ntp", "delta = %"PRId64" us\n", t2 - t1);
#endif

  ntp_shm->valid = 0;
  ntp_shm->count++;
  ntp_shm->clockTimeStampSec    = now;
  ntp_shm->clockTimeStampUSec   = 0;
  ntp_shm->receiveTimeStampSec  = tv.tv_sec;
  ntp_shm->receiveTimeStampUSec = (int)tv.tv_usec;
  ntp_shm->count++;
  ntp_shm->valid = 1;
}
