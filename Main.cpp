#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

#include "Application.h"


int main()
{
    Application::request().startup();
    return 0;
}
