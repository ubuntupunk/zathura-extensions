#include "zathura/zathura/plugin-api.h"
#include <stdio.h>

static bool test_init(zathura_t* zathura) {
    printf("Test init called\n");
    return true;
}

ZATHURA_UTILITY_PLUGIN_REGISTER(
  "test-plugin",
  0, 1, 0,
  test_init
)

int main() {
    printf("Test compiled successfully\n");
    return 0;
}