/*
   Copy me if you can.
   by 20h

   Hacked by bielxvf
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <X11/Xlib.h>

#define PROGRAM_NAME "dwmstatus"

/* Sleep time between refreshing */
#define PERIOD_S 10

#define UNUSED(x) do { (void) x; } while (false)

const char *timezone_madrid = "Europe/Madrid";
static Display *dpy;

char* smprintf(const char *fmt, ...)
{
    va_list fmtargs;
    va_start(fmtargs, fmt);
    uint64_t len = vsnprintf(NULL, 0, fmt, fmtargs);
    va_end(fmtargs);

    char* ret = malloc(++len);
    if (ret == NULL) {
        perror("malloc");
        exit(1);
    }

    va_start(fmtargs, fmt);
    vsnprintf(ret, len, fmt, fmtargs);
    va_end(fmtargs);

    return ret;
}

char* GetTimeFromTZ(const char *fmt, const char *timezone_name)
{
    setenv("TZ", timezone_name, 1);
    time_t utc_time = time(NULL);
    struct tm* local_time = localtime(&utc_time);
    if (local_time == NULL) {
        return smprintf("");
    }

    char time_buffer[129];
    if (!strftime(time_buffer, sizeof(time_buffer) - 1, fmt, local_time)) {
        fprintf(stderr, "strftime == 0\n");
        return smprintf("");
    }

    return smprintf("%s", time_buffer);
}

void SetStatus(const char *str)
{
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

char* ReadFile(const char *base, const char *file)
{
    char line[513];
    memset(line, 0, sizeof(line));

    char* path = smprintf("%s/%s", base, file);
    FILE* fd = fopen(path, "r");
    free(path);
    if (fd == NULL) {
        return NULL;
    }

    if (fgets(line, sizeof(line) - 1, fd) == NULL) {
        fclose(fd);
        return NULL;
    }
    fclose(fd);

    return smprintf("%s", line);
}

char* GetBattery(const char *base)
{
    char* co = ReadFile(base, "present");
    if (co == NULL) {
        return smprintf("");
    }

    if (co[0] != '1') {
        free(co);
        return smprintf("not present");
    }
    free(co);

    co = ReadFile(base, "charge_full_design");
    if (co == NULL) {
        co = ReadFile(base, "energy_full_design");
        if (co == NULL) {
            return smprintf("");
        }
    }
    int64_t descap = -1;
    sscanf(co, "%ld", &descap);
    free(co);

    co = ReadFile(base, "charge_now");
    if (co == NULL) {
        co = ReadFile(base, "energy_now");
        if (co == NULL) {
            return smprintf("");
        }
    }
    int64_t remcap = -1;
    sscanf(co, "%ld", &remcap);
    free(co);

    char battery_status[11];
    co = ReadFile(base, "status");
    if (strncmp(co, "Discharging", 11) == 0) {
        sprintf(battery_status, " Unplugged");
    } else if(strncmp(co, "Charging", 8) == 0) {
        sprintf(battery_status, " Charging");
    } else {
        sprintf(battery_status, " Unknown");
    }

    if (remcap < 0 || descap < 0) {
        return smprintf("invalid");
    }

    return smprintf("%.0f%%%s", ((float)remcap / (float)descap) * 100, battery_status);
}

char* GetTemperature(const char *base, const char *sensor)
{
    char* co = ReadFile(base, sensor);
    if (co == NULL) {
        return smprintf("");
    }
    return smprintf("%02.0f°C", atof(co) / 1000);
}

int main(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, PROGRAM_NAME": cannot open display.\n");
        return 1;
    }

    while (true) {
        char* battery = GetBattery("/sys/class/power_supply/BAT1");
        char* T0 = GetTemperature("/sys/devices/virtual/thermal/thermal_zone0", "temp");
        char* T1 = GetTemperature("/sys/devices/virtual/thermal/thermal_zone1", "temp");
        char* t = GetTimeFromTZ("%H:%M:%S", timezone_madrid);

        char* status = smprintf(" %s %s|%s %s ", battery, T0, T1, t);
        SetStatus(status);

        free(T0);
        free(T1);
        free(battery);
        free(t);
        free(status);

        sleep(PERIOD_S);
    }

    XCloseDisplay(dpy);
    return 0;
}
