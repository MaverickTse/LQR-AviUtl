# LQR-AviUtl
Liquid Rescale Plugin for AviUtl. Directly use libLqR, does not use ImageMagick

# Building Note
If building without Intel's compiler, please change mathimf.h to math.h
You will also need GTK+ Win32 development files (glib) in order to build.
I used Cilk for multithreading. If your compiler does not support Cilk,
redefine _Cilk_for to a standard "for" and remove any other _Cilk keywords
 and includes.
