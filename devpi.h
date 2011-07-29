#ifndef DEVPI_H
#define DEVPI_H

int __init init_module(void);
void __exit cleanup_module(void);


#define PI_MODE_MIN 0
#define PI_MODE_MAX 2
enum pi_mode {
    HEX = 0,
    DECIMAL = 1,
    STRING = 2,
};

const char* pi_mode_map[] = {
    [0] = "hex",
    [1] = "decimal",
    [2] = "string",
};

struct pi_status {
    size_t bytes_read;
    enum pi_mode mode;
};

#define NUM_PIES 4
const char* pies[NUM_PIES] = {
    "apple",
    "pumpkin",
    "cherry",
    "banana cream",
};

size_t pie_sizes[NUM_PIES] = {
    5,
    7,
    6,
    12,
};

#define PI_MAJOR 235
#define PI_CLASSNAME "pi"
#define PI_DEVNAME "pi"

#endif /* DEVPI_H */
