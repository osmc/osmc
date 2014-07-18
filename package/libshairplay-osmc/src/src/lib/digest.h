#ifndef DIGEST_H
#define DIGEST_H

void digest_generate_nonce(char *result, int resultlen);
int digest_is_valid(const char *our_realm, const char *password,
                    const char *our_nonce, const char *method,
                    const char *our_uri, const char *authorization);

#endif
