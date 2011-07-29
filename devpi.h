#ifndef DEVPI_H
#define DEVPI_H

int __init init_module(void);
void __exit cleanup_module(void);


#define PI_MODE_MIN 0
#define PI_MODE_MAX 1
enum pi_mode {
    HEX = 0,
    DECIMAL = 1,
};

const char* pi_mode_map[] = {
    [0] = "hex",
    [1] = "decimal",
};

struct pi_status {
    size_t bytes_read;
    enum pi_mode mode;
};

#define PI_MAJOR 235
#define PI_CLASSNAME "pi"
#define PI_DEVNAME "pi"

#endif /* DEVPI_H */
