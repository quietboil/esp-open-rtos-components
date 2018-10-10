#include <FreeRTOS.h>
#include <espressif/esp_system.h>
#include <lwip/apps/tftp_server.h>

#ifdef VFS_DEBUG
#define LOG(fmt,...) printf("VFS>" fmt,## __VA_ARGS__)
#else
#define LOG(fmt,...)
#endif

/**
 * \bried Failure code for TFTP `read` and `write` functions
 */
#define ERR -1

static struct tftp_context const ** vfs_list;
static struct tftp_context const *  vfs;

static void * tftp_open(const char * fname, const char * mode, uint8_t write)
{
    LOG("%s %s\n", (write ? "write" : "read"), fname);
    void * handle = NULL;
    if (!vfs) {
        struct tftp_context const ** ctx_lst = vfs_list;
        struct tftp_context const * ctx;
        while ((ctx = *ctx_lst) != NULL) {
            handle = ctx->open(fname, mode, write);
            if (handle) {
                vfs = ctx;
                break;
            }
            ++ctx_lst;
        }
    }
    LOG("%s VFS that can handle this request\n", handle ? "found" : "there is no");
    return handle;
}

static void tftp_close(void * handle)
{
    if (vfs) {
        vfs->close(handle);
        vfs = NULL;
    }
}

static int tftp_read(void * handle, void * buf, int bytes)
{
    if (vfs) {
        return vfs->read(handle, buf, bytes);
    } else {
        LOG("read error - no VFS\n");
        return ERR;
    }
}

static int tftp_write(void * handle, struct pbuf * p)
{
    if (vfs) {
        return vfs->write(handle, p);
    } else {
        LOG("write error - no VFS\n");
        return ERR;
    }
}

static struct tftp_context const ctx = {
    .open  = tftp_open,
    .close = tftp_close,
    .read  = tftp_read,
    .write = tftp_write
};

void tftp_vfs_init(struct tftp_context const * vfs_contexts[])
{
    vfs_list = vfs_contexts;

    err_t err = tftp_init(&ctx);
    if (err) {
        LOG("failed to start TFTP: %d\n", err);
    } else {
        LOG("TFTP started\n");
    }
}