#include "application.h"



int main(void)
{
    Application app;
    if (app.init())
    {
        app.run();
    }
    return 0;
}