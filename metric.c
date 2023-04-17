#include "meminfo.h"
#include "kill.h"
#include "metric.h"
#include <math.h>
#include <time.h>

float factor_x(float interval,float avg)
{
    return 1.0/expf(interval/avg);
}

//load1 = load0 * e + active * (1 - e)
float avg_x(float curr, float prev, float factor)
{
    return prev*factor + curr*(1-factor);
}

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
    
    int interval = poll->report_interval_ms/1000;
    float curr = 0.0;
    float prev_avg = 0.0;

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
        /* for iowait avg */ 
        prev_avg = poll->cstat_util.iowait_avg10;
        curr = poll->cstat_util.iowait;
        poll->cstat_util.iowait_avg10 = avg_x(curr, prev_avg, factor_x(interval,10));

        prev_avg = poll->cstat_util.iowait_avg30;
        poll->cstat_util.iowait_avg30 = avg_x(curr, prev_avg, factor_x(interval,30));
        
        prev_avg = poll->cstat_util.iowait_avg60;
        poll->cstat_util.iowait_avg60 = avg_x(curr, prev_avg, factor_x(interval,60));

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
    //printf("user: %.2f%% iowait: %.2f%%\n", poll->cstat_util.user,poll->cstat_util.iowait);
    //printf("avg10: %.2f%% avg30: %.2f%% avg60:%.2f%% \n", poll->cstat_util.iowait_avg10,poll->cstat_util.iowait_avg30, poll->cstat_util.iowait_avg60);
    return 0;
}

int metric_init(poll_loop_args_t *poll)
{
    return 0;
}

int metric_exit(poll_loop_args_t *poll)
{
    return 0;
}
