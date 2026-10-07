#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H

#define MAX_BLOCKED_DOMAINS 100
#define MAX_DOMAIN_LENGTH 256
int load_blocked_domains(const char *filename);
int is_domain_blocked(const char *domain);

#endif
