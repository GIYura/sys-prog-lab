/*
 * UDP sender
 * */

#define _DEFAULT_SOURCE

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_DUMP         (0)

#define BUFF_LEN            (1024)
#define LOCAL_PORT          (40000)
#define DESTINATION_PORT    (50000)
#define DGRAM_COUNT         (10)

typedef struct
{
    int fd_sock;
    uint32_t buff[BUFF_LEN];
    uint32_t seq_counter;
} ctx_t;

static void PrintBuffer(uint32_t* const buffer, size_t size)
{
    assert(buffer != NULL);

    printf("Sender buffer:\n");

#if BUFFER_DUMP
    for (size_t i = 0; i < size; i++)
    {
        printf("%u ", buffer[i]);
    }
    printf("\n");
#else
    printf("UDP dgram counter=%u\n", buffer[0]);
#endif
}

static void FillBuffer(uint32_t* const buffer, size_t size)
{
    assert(buffer != NULL);

    for (size_t i = 0; i < size; i++)
    {
        buffer[i] = i + 1;
    }
}

static void Init(ctx_t* const ctx)
{
    assert(ctx != NULL);

    ctx->fd_sock = -1;
    ctx->seq_counter = 0;
}

int main (int argc, char* argv[])
{
    ctx_t ctx;
    struct sockaddr_in local_addr = {0};
    struct sockaddr_in dest_addr = {0};
    int ret = 0;
    ssize_t size;

    Init(&ctx);

    FillBuffer(ctx.buff, BUFF_LEN);

    ctx.fd_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (ctx.fd_sock < 0)
    {
        perror("socket");
        return -1;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DESTINATION_PORT);
    ret = inet_pton(AF_INET, "127.0.0.1", &dest_addr.sin_addr);
    if (ret != 1)
    {
        printf("Invalid format\n");
        close(ctx.fd_sock);
        return -2;
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(LOCAL_PORT);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(ctx.fd_sock, (struct sockaddr*)&local_addr, sizeof(local_addr));
    if (ret < 0)
    {
        perror("bind");
        close(ctx.fd_sock);
        return -2;
    }

    for (uint32_t i = 0; i < DGRAM_COUNT; i++)
    {
        ctx.buff[0] = ctx.seq_counter;
        size = sendto(ctx.fd_sock, ctx.buff, sizeof(ctx.buff), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if (size < 0)
        {
            perror("sendto");
            close(ctx.fd_sock);
            return -3;
        }

        usleep(100000);
        ++ctx.seq_counter;
        PrintBuffer(ctx.buff, BUFF_LEN);
    }

    close(ctx.fd_sock);

    return ret;
}
