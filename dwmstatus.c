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

#include <X11/Xlib.h>

#define UNUSED(x) do { (void) x; } while (false)

const char *tzmadr = "Europe/Madrid";

static Display *dpy;

char* smprintf(const char *fmt, ...)
{
    va_list fmtargs;
    va_start(fmtargs, fmt);
    int len = vsnprintf(NULL, 0, fmt, fmtargs);
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

char* mktimes(const char *fmt, const char *tzname)
{
    char buf[129];
    time_t tim;
    struct tm *timtm;

    setenv("TZ", tzname, 1);
    tim = time(NULL);
    timtm = localtime(&tim);
    if (timtm == NULL)
        return smprintf("");

    if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
        fprintf(stderr, "strftime == 0\n");
        return smprintf("");
    }

    return smprintf("%s", buf);
}

void setstatus(const char *str)
{
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

char* readfile(const char *base, const char *file)
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

char* getbattery(const char *base)
{
    char *co;
    int descap = -1;
    int remcap = -1;

    co = readfile(base, "present");
    if (co == NULL) {
        return smprintf("");
    }

    if (co[0] != '1') {
        free(co);
        return smprintf("not present");
    }
    free(co);

    co = readfile(base, "charge_full_design");
    if (co == NULL) {
        co = readfile(base, "energy_full_design");
        if (co == NULL) {
            return smprintf("");
        }
    }
    sscanf(co, "%d", &descap);
    free(co);

    co = readfile(base, "charge_now");
    if (co == NULL) {
        co = readfile(base, "energy_now");
        if (co == NULL) {
            return smprintf("");
        }
    }
    sscanf(co, "%d", &remcap);
    free(co);

    char status[11];
    co = readfile(base, "status");
    if (!strncmp(co, "Discharging", 11)) {
        sprintf(status, " Unplugged");
    } else if(!strncmp(co, "Charging", 8)) {
        sprintf(status, " Charging");
    } else {
        sprintf(status, " Unknown");
    }

    if (remcap < 0 || descap < 0) {
        return smprintf("invalid");
    }

    return smprintf("%.0f%%%s", ((float)remcap / (float)descap) * 100, status);
}

char* gettemperature(const char *base, const char *sensor)
{
    char* co = readfile(base, sensor);
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
        fprintf(stderr, "dwmstatus: cannot open display.\n");
        return 1;
    }

    while (true) {
        char* bat = getbattery("/sys/class/power_supply/BAT1");
        char* tmadr = mktimes("%H:%M:%S", tzmadr);
        char* t0 = gettemperature("/sys/devices/virtual/thermal/thermal_zone0", "temp");
        char* t1 = gettemperature("/sys/devices/virtual/thermal/thermal_zone1", "temp");

        char* status = smprintf(" B:%s T:%s|%s Madrid: %s ", bat, t0, t1, tmadr);
        setstatus(status);

        free(t0);
        free(t1);
        free(bat);
        free(tmadr);
        free(status);

        sleep(10);
    }

    XCloseDisplay(dpy);
    return 0;
}
