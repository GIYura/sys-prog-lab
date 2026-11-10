/*
 * UDP receiver
 * */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <poll.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>
#include <errno.h>

#include <stdatomic.h>

#include "ring.h"

#define BUFF_LEN            (1024)
#define DESTINATION_PORT    (50000)
#define POLL_TIMEOUT_MS     (500)

typedef struct
{
    int fd_sock;
    ring_t ring;
    uint32_t buff[BUFF_LEN];
    uint32_t pushed_dgram_counter;
    uint32_t popped_dgram_counter;
    uint32_t dropped_dgram_counter;
    bool workerFinished;
    pthread_mutex_t lock;
    pthread_cond_t dataAvailable;
} ctx_t;

static atomic_int stop;

static void handle_sigint(int signo)
{
    (void)signo;

    atomic_store(&stop, 1);
}

static void PrintBuffer(const uint32_t* const buffer, size_t size)
{
    assert(buffer != NULL);
    assert(size > 0);

    printf("Receiver buffer counter=%u\n", buffer[0]);
}

static bool Init(ctx_t* const ctx)
{
    assert(ctx != NULL);

    ctx->fd_sock = -1;
    ctx->pushed_dgram_counter = 0;
    ctx->popped_dgram_counter = 0;
    ctx->dropped_dgram_counter = 0;
    ctx->workerFinished = false;

    atomic_init(&stop, 0);

    pthread_mutex_init(&ctx->lock, NULL);
    pthread_cond_init(&ctx->dataAvailable, NULL);

    return RingInit(&ctx->ring);
}

static void* udp_thread(void* arg)
{
    ctx_t* ctx = (ctx_t*)arg;
    ssize_t size;
    int ret;
    struct pollfd pfds = {.fd = ctx->fd_sock, .events = POLLIN};

    while (!atomic_load(&stop))
    {
        ret = poll(&pfds, 1, POLL_TIMEOUT_MS);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("poll");
            break;
        }

        if (ret == 0)
        {
            continue;
        }

        if (pfds.revents & (POLLERR | POLLNVAL | POLLHUP))
        {
            fprintf(stderr, "poll: socket error, revents=0x%x\n", pfds.revents);
            break;
        }

        if (!(pfds.revents & POLLIN))
            continue;

        if (pfds.revents & POLLIN)
        {
            size = recvfrom(ctx->fd_sock, ctx->buff, sizeof(ctx->buff), 0, NULL, NULL);
            if (size == (ssize_t)sizeof(ctx->buff))
            {
                printf("Receiver get %zd bytes\n", size);
                pthread_mutex_lock(&ctx->lock);
                if (RingPush(&ctx->ring, ctx->buff))
                {
                    ctx->pushed_dgram_counter++;
                    pthread_cond_signal(&ctx->dataAvailable);
                }
                else
                {
                    ctx->dropped_dgram_counter++;
                }
                pthread_mutex_unlock(&ctx->lock);
            }
            else if (size == 0)
            {
                printf("Received empty UDP\n");
            }
            else if (size < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                perror("recvfrom");
                break;
            }
        }
    }

    pthread_mutex_lock(&ctx->lock);

    ctx->workerFinished = true;
    pthread_cond_signal(&ctx->dataAvailable);

    pthread_mutex_unlock(&ctx->lock);

    return NULL;
}

int main(void)
{
    ctx_t ctx;
    struct sockaddr_in addr = {0};
    int ret;
    pthread_t th;
    uint32_t datagram[BUFF_LEN];
    struct sigaction sa = {0};

    if (!Init(&ctx))
    {
        fprintf(stderr, "Failed to init\n");
        return 1;
    }

    if (!atomic_is_lock_free(&stop))
    {
        fprintf(stderr, "stop atomic is not lock-free\n");
        return 2;
    }

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) < 0)
    {
        perror("sigaction");
        return 3;
    }

    ctx.fd_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx.fd_sock < 0)
    {
        perror("socket");
        return 4;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(DESTINATION_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(ctx.fd_sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        perror("bind");
        close(ctx.fd_sock);
        return 5;
    }

    ret = pthread_create(&th, NULL, udp_thread, &ctx);
    if (ret != 0)
    {
        fprintf(stderr, "pthread_create: %s\n", strerror(ret));
        close(ctx.fd_sock);
        return 6;
    }

    printf("UDP receiver ready!\n");

    while (1)
    {
        pthread_mutex_lock(&ctx.lock);

        while (RingCount(&ctx.ring) == 0 && !ctx.workerFinished)
        {
            pthread_cond_wait(&ctx.dataAvailable, &ctx.lock);
        }

        if (RingCount(&ctx.ring) == 0 && ctx.workerFinished)
        {
            pthread_mutex_unlock(&ctx.lock);
            break;
        }

        pthread_mutex_unlock(&ctx.lock);

        if (RingPop(&ctx.ring, datagram))
        {
            PrintBuffer(datagram, BUFF_LEN);
            ctx.popped_dgram_counter++;
        }
    }

    ret = pthread_join(th, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "pthread_join: %s\n", strerror(ret));
    }

    close(ctx.fd_sock);

    pthread_mutex_destroy(&ctx.lock);
    pthread_cond_destroy(&ctx.dataAvailable);
    RingFree(&ctx.ring);

    return ret;
}
