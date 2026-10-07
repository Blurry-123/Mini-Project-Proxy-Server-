#include <stdio.h>
#include <string.h>

#include "access_control.h"

char blocked_domains[MAX_BLOCKED_DOMAINS][MAX_DOMAIN_LENGTH];

int blocked_domain_count = 0;


/*
 * Load blocked domains from a file.
 */
int load_blocked_domains(const char *filename)
{
    FILE *file;

    file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("Warning: Could not open %s\n", filename);
        return -1;
    }

    while (
        blocked_domain_count < MAX_BLOCKED_DOMAINS &&
        fgets(
            blocked_domains[blocked_domain_count],
            MAX_DOMAIN_LENGTH,
            file
        ) != NULL
    )
    {
        /*
         * Remove newline.
         */
        blocked_domains[blocked_domain_count]
            [strcspn(
                blocked_domains[blocked_domain_count],
                "\r\n"
            )] = '\0';

        /*
         * Ignore empty lines and comments.
         */
        if (
            blocked_domains[blocked_domain_count][0] == '\0' ||
            blocked_domains[blocked_domain_count][0] == '#'
        )
        {
            continue;
        }

        blocked_domain_count++;
    }

    fclose(file);

    printf(
        "Loaded %d blocked domain(s)\n",
        blocked_domain_count
    );

    return 0;
}


/*
 * Check whether a domain is blocked.
 */
int is_domain_blocked(const char *domain)
{
    int i;

    for (i = 0; i < blocked_domain_count; i++)
    {
        if (strcasecmp(
                domain,
                blocked_domains[i]) == 0)
        {
            return 1;
        }
    }

    return 0;
}