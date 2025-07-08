#include <misc/print.h>

void print_logo_and_tagline() {
    printk(COLOR_BLUE, "   _|_|              _|  _|                    _|                \n");
    printk(COLOR_CYAN, " _|    _|  _|  _|_|      _|  _|      _|_|    _|_|_|_|    _|_|    \n");
    printk(COLOR_GREEN, " _|_|_|_|  _|_|      _|  _|_|      _|    _|    _|      _|    _|  \n");
    printk(COLOR_MAGENTA, " _|    _|  _|        _|  _|  _|    _|    _|    _|      _|    _|  \n");
    printk(COLOR_YELLOW, " _|    _|  _|        _|  _|    _|    _|_|        _|_|    _|_|   \n");
    printk(COLOR_WHITE, "Check out arikoto.nerdnextdoor.net!\n");
}
