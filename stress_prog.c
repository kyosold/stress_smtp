#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>

#define NTHREADS	65500

void *thread_function(void *);

//pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

//int counter = 0;

void usage(char *prog) 
{
	printf("%s -t<thread num>\n", prog);
}

int main(int argc, char **argv)
{
	int thread_num = 0;
	char host[512] = {0};
    char port[512] = {0};
    int is_auth = 0;
	char sasl_user[512] = {0};
	char sasl_pass[512] = {0};
	int ch;
	const char *args = "h:t:h:P:u:p:a";
	while ((ch = getopt(argc, argv, args)) != -1) {
		switch (ch) {
			case 't':
				thread_num = atoi(optarg);
				break;
			case 'h':
                snprintf(host, sizeof(host), "%s", optarg);
                break;
            case 'P':
                snprintf(port, sizeof(port), "%s", optarg);
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
			case 'h':
			default:
				usage(argv[0]);
				exit(0);
		}
	}

	pthread_t thread_id[thread_num];
	int i, j;

	for (i=0; i<thread_num; i++) {
		pthread_create(&thread_id[i], NULL, thread_function, NULL);
	}

	for (j=0; j<thread_num; j++) {
		pthread_join(thread_id[j], NULL);
	}

	//printf("Final counter value: %d\n", counter);

	return 0;
}

void *thread_function(void *dummyPtr)
{
/*
	printf("Thread number %ld\n", pthread_self());
	pthread_mutex_lock( &mutex1 );
	counter++;
	pthread_mutex_unlock( &mutex1 );
*/


	char to_name[512] = {0};
	char subject[512] = {0};

	char cmd[1024] = {0};
	int i = 1;
	for (i = 1; i<=200; i++) {
		snprintf(to_name, sizeof(to_name), "smtptest%03d@xxxx.com", i);
		snprintf(subject, sizeof(subject), "-s'Thread[%0X]: smtp test index[%d] for:%s'", pthread_self(), i, to_name);
		
		if (is_auth == 1) {
			snprintf(cmd, sizeof(cmd), "./smtp -h'%s' -P'%s' -a -u'%s' -p'%s' -f'smail@test.com' -t'%s' %s", host, port, sasl_user, sasl_pass, to_name, subject);
		} else {
			snprintf(cmd, sizeof(cmd), "./smtp -h'%s' -P'%s' -f'smail@test.com' -t'%s' %s", host, port, to_name, subject);
		}
		
		//printf("Thread[%0X] system:%s\n",  pthread_self(), cmd);

		int ret = 0;
		ret = system(cmd);
		if (ret == -1) {
			printf("Thread[%0X] system failed:%s [%s]\n", pthread_self(), strerror(errno), cmd);
		}
	
	}
	
}

