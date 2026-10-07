#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H

#define MAX_BLOCKED_DOMAINS 100
#define MAX_DOMAIN_LENGTH 256

/*
 * Load blocked domains from file.
 */
int load_blocked_domains(const char *filename);

/*
 * Check whether a domain is blocked.
 */
int is_domain_blocked(const char *domain);

#endif