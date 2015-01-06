#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <getopt.h>

enum STYPE {
    EHLO,
    AUTH,
    USER,
    PWD,
    MAIL,
    RCPT,
    DATA,
    EOM,
    QUIT
};


void usage(char *prog)
{
    printf("%s -h<host> -P<port> -b[bind ip] -a[need auth login] -u[sasl_user] -p[sasl_pass] -f[from] -t<rcpt to> -s<subject>\n", prog);
}

int main(int argc, char **argv)
{
    char host[512] = {0};
    char port[512] = {0};
    char bip[20] = {0};
    int is_auth = 0;
	char sasl_user[512] = {0};
	char sasl_pass[512] = {0};
	char from[512] = {0};
	char to[512] = {0};
	char subject[1024] = {0};

    int ch;
    const char *args = "h:P:b:u:p:t:s:f:a";
    while ((ch = getopt(argc, argv, args)) != -1) {
        switch (ch) {
            case 'h':
                snprintf(host, sizeof(host), "%s", optarg);
                break;
            case 'P':
                snprintf(port, sizeof(port), "%s", optarg);
                break;
            case 'b':
                snprintf(bip, sizeof(bip), "%s", optarg);
                break;
            case 'a':
                is_auth = 1;
                break;
            case 'u':
                snprintf(sasl_user, sizeof(sasl_user), "%s", optarg);
                break;
            case 'p':
                snprintf(sasl_pass, sizeof(sasl_pass), "%s", optarg);
                break;
            case 'f':
                snprintf(from, sizeof(from), "%s", optarg);
                break;
            case 't':
                snprintf(to, sizeof(to), "%s", optarg);
                break;
            case 's':
                snprintf(subject, sizeof(subject), "%s", optarg);
                break;
            default:
                usage(argv[0]);
                exit(0);
        }
    }

    if (strlen(host) <= 0 || strlen(port) <= 0) {
        usage(argv[0]);
        exit(0);
    }

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));

    struct sockaddr_in sin_bind;
    memset(&sin_bind, 0, sizeof(sin_bind));

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr(host);
    srv_addr.sin_port = htons(atoi(port));

    int sock;
    char buf[1024 * 4] = {0};
    char sbuf[1024] = {0};
    char rcode[5] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("create socket fail\r\n");
        return 1;
    }

    if (strlen(bip) > 0) {
        sin_bind.sin_family = AF_INET;
        sin_bind.sin_addr.s_addr = inet_addr(bip);
        sin_bind.sin_port = 0;

        if ( bind(sock, (struct sockaddr *)&sin_bind, sizeof(sin_bind)) < 0) {
            goto FAIL;
        }
    }

    if (connect(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        printf("connect fail:%s\r\n", strerror(errno));
        return 1;
    }

    enum STYPE cur_step = EHLO;
    while (read(sock, buf, sizeof(buf)) >= 0) {
        printf("< %s", buf);

        int i = strlen(buf) - 2;
        while ((buf[i] != '\n') && (i > 0)) {
            i--;
        }

        if (i == 0) {
            memcpy(rcode, buf, 4);
        } else {
            memcpy(rcode, &buf[i+1], 4);
        }
        rcode[4] = '\0';


        switch (cur_step) {
            case EHLO:
                if (strcasecmp(rcode, "220 ") != 0) {
                    goto FAIL;
                    break;
                }

                if (is_auth) {
                	snprintf(sbuf, sizeof(sbuf), "EHLO mmm\r\n");
                    cur_step = AUTH;
                } else {
                	snprintf(sbuf, sizeof(sbuf), "HELO mmm\r\n");
                    cur_step = MAIL;
                }
            break;

            case AUTH:
                if (strcasecmp(rcode, "250 ") != 0) {
                    goto FAIL;
                    break;
                }

                snprintf(sbuf, sizeof(sbuf), "AUTH LOGIN\r\n");
                cur_step = USER;
            break;

            case USER:
                if (strcasecmp(rcode, "334 ") != 0) {
                    goto FAIL;
                    break;
                }

                snprintf(sbuf, sizeof(sbuf), "%s\r\n", sasl_user);
                cur_step = PWD;
            break;

            case PWD:
                if (strcasecmp(rcode, "334 ") != 0) {
                    goto FAIL;
                    break;
                }

                snprintf(sbuf, sizeof(sbuf), "%s\r\n", sasl_pass);
                cur_step = MAIL;
            break;

            case MAIL:
            	if (is_auth) {
            		if (strcasecmp(rcode, "235 ") != 0) {
            			goto FAIL;
            			break;
            		}
            	} else {
            		if (strcasecmp(rcode, "250 ") != 0) {
            			goto FAIL;
            			break;
            		}
            	}
            	
            	if (*from != '\0') {
            		snprintf(sbuf, sizeof(sbuf), "MAIL FROM:<%s>\r\n", from);
            	} else {
            		snprintf(sbuf, sizeof(sbuf), "MAIL FROM:<%s>\r\n", sasl_user);
            	}
                cur_step = RCPT;
            break;

            case RCPT:
                if (strcasecmp(rcode, "250 ") != 0) {
                    goto FAIL;
                    break;
                }

                snprintf(sbuf, sizeof(sbuf), "RCPT TO:<%s>\r\n", to);
                cur_step = DATA;
            break;

            case DATA:
                if (strcasecmp(rcode, "250 ") != 0) {
                    goto FAIL;
                    break;
                }

                snprintf(sbuf, sizeof(sbuf), "DATA\r\n");
                cur_step = EOM;
            break;

            case EOM:
                if (strcasecmp(rcode, "354 ") != 0) {
                    goto FAIL;
                    break;
                }

                snprintf(sbuf, sizeof(sbuf), "SUBJECT: %s\r\n\r\n.\r\n", subject);
                cur_step = QUIT;
            break;

            case QUIT:
                if (strcasecmp(rcode, "250 ") != 0) {
                    goto FAIL;
                    break;
                }

                snprintf(sbuf, sizeof(sbuf), "QUIT\r\n");
                cur_step = QUIT;
            break;

        }

        write(sock, sbuf, strlen(sbuf));
        printf("> %s", sbuf);

        memset(buf, 0, sizeof(buf));
        memset(sbuf, 0, sizeof(sbuf));
    }


FAIL:
    close(sock);


    return 0;
}
