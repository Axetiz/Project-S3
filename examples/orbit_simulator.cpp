// Apsilume: Main product entry point; all CLI/GUI startup logic is shared in app_launcher.cpp.
#include "app_launcher.hpp"
int main(int argc, char **argv) {
    return orbitsimlite::run_product_app(argc, argv, 0);
}
