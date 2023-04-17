#include "meminfo.h"
#include "kill.h"
#include "metric.h"
static void read_cpu_stat(struct cpu_stat *cs) {
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        printf("open /proc/stat error\n");
        exit(1); 
    }
    fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
           &cs->user, &cs->nice, &cs->system, &cs->idle, &cs->iowait,
           &cs->irq, &cs->softirq, &cs->steal, &cs->guest, &cs->guest_nice);
    fclose(fp);
}


static void diff_cpu_stat(struct cpu_stat *prev, struct cpu_stat *curr, struct cpu_stat *diff) {
    diff->user = curr->user - prev->user;
    diff->nice = curr->nice - prev->nice;
    diff->system = curr->system - prev->system;
    diff->idle = curr->idle - prev->idle;
    diff->iowait = curr->iowait - prev->iowait;
    diff->irq = curr->irq - prev->irq;
    diff->softirq = curr->softirq - prev->softirq;
    diff->steal = curr->steal - prev->steal;
    diff->guest = curr->guest - prev->guest;
    diff->guest_nice = curr->guest_nice - prev->guest_nice;
}

static int calc_percent(struct cpu_stat *diff, poll_loop_args_t *poll) {
    long total = diff->user + diff->nice + diff->system + diff->idle + diff->iowait +
                 diff->irq + diff->softirq + diff->steal + diff->guest +
                 diff->guest_nice;
 if (total == 0) {
        memset(&poll->cstat_util, 0, sizeof(poll->cstat_util));
    }
    else {
        poll->cstat_util.iowait = (float)diff->iowait / total * 100.0;
        poll->cstat_util.user = (float)diff->user / total * 100.0;
        poll->cstat_util.system = (float)diff->system / total * 100.0;
        poll->cstat_util.idle = (float)diff->idle / total * 100.0;
    }
    return 0;
}

int get_cpu_stat(poll_loop_args_t *poll)
{
    struct cpu_stat curr, diff;

    read_cpu_stat(&curr);
    diff_cpu_stat(&poll->cstat_prev, &curr, &diff);
    calc_percent(&diff, poll);
    poll->cstat_prev = curr;
    return 0;
}

int metric_init(poll_loop_args_t *poll)
{
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("fopen /proc/stat error");
        exit(1);
    }
    poll->cstat_fd = fp;
    return 0;
}

int metric_exit(poll_loop_args_t *poll)
{
    if (poll->cstat_fd)
        fclose(poll->cstat_fd);
    return 0;
}
