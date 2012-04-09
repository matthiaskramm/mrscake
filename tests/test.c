#include "language_interpreter.h"

int main()
{
    language_interpreter_t*lang = javascript_interpreter_new();
    int r = lang->run_script(lang, "3*7");
    printf("%d\n", r);
    lang->destroy(lang);
}
