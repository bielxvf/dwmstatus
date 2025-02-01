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

#define BATTERY_EMOJI "🔋"
#define PLUG_EMOJI "🔌"
#define TEMP_EMOJI "🌡"
#define CLOCK_EMOJI "⏰"

#define TIMEZONE "Europe/Madrid"

#define UNUSED(x) do { (void) x; } while (false)

static Display* dpy;

char* smprintf(const char* fmt, ...)
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

char* GetTimeFromTZ(const char* fmt, const char* timezone_name)
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

void SetStatus(const char* str)
{
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

char* ReadFile(const char* base, const char* file)
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

char* GetBattery(const char* base)
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
    int64_t battery_max_charge = -1;
    sscanf(co, "%ld", &battery_max_charge);
    free(co);

    co = ReadFile(base, "charge_now");
    if (co == NULL) {
        co = ReadFile(base, "energy_now");
        if (co == NULL) {
            return smprintf("");
        }
    }
    int64_t battery_charge = -1;
    sscanf(co, "%ld", &battery_charge);
    free(co);

    char battery_status[11];
    co = ReadFile(base, "status");
    if (strncmp(co, "Discharging", 11) == 0) {
        sprintf(battery_status, BATTERY_EMOJI);
    } else if(strncmp(co, "Charging", 8) == 0) {
        sprintf(battery_status, PLUG_EMOJI);
    } else {
        sprintf(battery_status, "?");
    }

    if (battery_charge < 0 || battery_max_charge < 0) {
        return smprintf("invalid");
    }

    return smprintf("%s%.2f%%", battery_status, ((float) battery_charge / (float) battery_max_charge) * 100);
}

char* GetTemperature(const char *base, const char *sensor)
{
    char* co = ReadFile(base, sensor);
    if (co == NULL) {
        return smprintf("");
    }
    return smprintf("%02.2f°C", atof(co) / 1000);
}

int main(int argc, char** argv)
{
    // TODO: Use argv for the format
    // Ex: dwmstatus battery cpu-temp timezone="Europe/Madrid"
    UNUSED(argc);
    UNUSED(argv);

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, PROGRAM_NAME": cannot open display.\n");
        return 1;
    }

    while (true) {
        // TODO: Use any BAT* file
        char* battery = GetBattery("/sys/class/power_supply/BAT1");
        char* T1 = GetTemperature("/sys/devices/virtual/thermal/thermal_zone1", "temp");
        char* t = GetTimeFromTZ("%H:%M:%S", TIMEZONE);

        char* status = smprintf(" %s "TEMP_EMOJI"%s "CLOCK_EMOJI"%s ", battery, T1, t);
        SetStatus(status);

        free(T1);
        free(battery);
        free(t);
        free(status);

        sleep(PERIOD_S);
    }

    XCloseDisplay(dpy);
    return 0;
}
