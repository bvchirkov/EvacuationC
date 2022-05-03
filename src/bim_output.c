#include "bim_output.h"

void bim_output_head  (const bim_t *const bim, FILE *fp)
{
    fprintf(fp, "t,");
    for (size_t i = 0; i < bim->zones->length; i++)
    {
        bim_zone_t *zone = bim->zones->data[i];
        fprintf(fp, "%s,", zone->name);
    }
    for (size_t i = 0; i < bim->transits->length; i++)
    {
        bim_transit_t *transit = bim->transits->data[i];
        fprintf(fp, "%s,", transit->name);
    }
    fprintf(fp, "\n");
    fflush(fp);
}

void bim_output_body  (const bim_t *const bim, float time, FILE *fp)
{
    fprintf(fp, "%.2f,", time);
    for (size_t i = 0; i < bim->zones->length; i++)
    {
        bim_zone_t *zone = bim->zones->data[i];
        fprintf(fp, "%.2f,", zone->numofpeople);
    }

    for (size_t i = 0; i < bim->transits->length; i++)
    {
        bim_transit_t *transit = bim->transits->data[i];
        fprintf(fp, "%.2f,", transit->nop_proceeding);
    }
    fprintf(fp, "\n");
    fflush(fp);
}

char* bim_basename    (char *path_to_file)
{
    char *fn;
    char *suffix;

    (fn = strrchr(path_to_file, '/')) ? ++fn : (fn = path_to_file);
    (suffix = strrchr(fn, '.')) ? suffix : (suffix = fn);

    char *s = (char*)calloc(strlen(path_to_file), sizeof (char));
    if (!s)
    {
        return NULL;
    }
    strncpy(s, fn, suffix - fn);

    return s;
}

char *bim_create_file_name  (const char* bfn, const char* middle_name, const char* suffix)
{
    char *fn = (char*)calloc((strlen(bfn) + strlen(middle_name) + strlen(suffix)), sizeof (char));
    if (!fn)
    {
        return NULL;
    }

    strcat(fn, bfn);
    strcat(fn, middle_name);
    strcat(fn, suffix);

    return fn;
}
