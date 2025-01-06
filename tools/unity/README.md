Generates the following files:

- `amast_config.h`
- `amast.h`
- `amast.c`
- `amast_cooperative.c`
- `amast_preemptive.c`
- `amast_freertos.c`
- `amast_posix.c`
- `amast_test.h`
- `amast_test.c`
- `Makefile`

All the files except `amast_config.h` and `Makefile` are generated using
the files listed in `files.txt.in` using the followings rules:

1. `amast.h` combines all `.h` files.
2. `amast_cooperative.c` combines all `libs/ao/cooperative/*.c` files.
3. `amast_preemptive.c` combines all `libs/ao/preemptive/*.c` files.
4. `amast_freertos.c` combines all `libs/pal/freertos/*.c` files.
5. `amast_posix.c` combines all `libs/pal/posix/*.c` files.
6. `amast_test.h` combines all test `.h` files.
7. `amast_test.c` combines all test `.c` files.
8. `amast.c` combines all remaining `.c` files.
9. Each file pasted into the generated files is prepended with a comment
   specifying the full path name of the file in the amast repository.
10. All generated `.c` files include `amast_config.h` and `amast.h`.
    All other user includes of the form `#include "<file name>"` are removed from them.
