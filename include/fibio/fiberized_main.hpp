//
//  fiberized_main.hpp
//  fibio
//
//  Created by Chen Xu on 14/12/12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fiberized_main_hpp
#define fibio_fiberized_main_hpp

#if defined(_WIN32)
#   if defined(_CONSOLE)
// Win32/64 console app
namespace fibio {
    int _tmain(int argc, TCHAR *argv[]);
}
int _tmain(int argc, TCHAR *argv[]) {
    return fibio::fiberize(fibio::_tmain, argc, argv);
}
#   else    // defined(_CONSOLE)
// Win32/64 GUI app
namespace fibio {
    int WinMain(_In_ HINSTANCE hInstance,
                _In_ HINSTANCE hPrevInstance,
                _In_ LPSTR lpCmdLine,
                _In_ int nCmdShow);
}
int CALLBACK WinMain(_In_ HINSTANCE hInstance,
                     _In_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine,
                     _In_ int nCmdShow)
{
    return fibio::fiberize(fibio::WinMain, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
#   endif   // !defined(_CONSOLE)
#else    // defined(_WIN32)
// POSIX app
namespace fibio {
    int main(int argc, char *argv[]);
}
int main(int argc, char *argv[]) {
    return fibio::fiberize(fibio::main, argc, argv);
}
#endif   // !defined(_WIN32)

#endif
