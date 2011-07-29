#define DEVICE_NAME "pi"

int init_module(void);
void cleanup_module(void);

struct pi_status {
    size_t bytes_read;
};
