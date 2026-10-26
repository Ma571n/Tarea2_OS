#include <stdio.h>
#include <string.h>
#include <time.h>
#include "config.h"

void config_defaults(Config *cfg) {
    cfg->delay_min_ms = 50;
    cfg->delay_max_ms = 300;
    cfg->seed = (unsigned int)time(NULL);
}

int config_load(Config *cfg, const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen config");
        return -1;
    }

    char line[128];
    while (fgets(line, sizeof(line), f) != NULL) {
        int val;
        if (sscanf(line, "delay_min=%d", &val) == 1)      cfg->delay_min_ms = val;
        else if (sscanf(line, "delay_max=%d", &val) == 1) cfg->delay_max_ms = val;
        else if (sscanf(line, "seed=%d", &val) == 1)      cfg->seed = (unsigned int)val;
    }

    fclose(f);
    return 0;
}