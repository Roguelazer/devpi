#define DEVICE_NAME "pi"

int init_module(void);
void cleanup_module(void);

struct pi_status {
    int bytes_read;
};
