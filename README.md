# FICRGB
<-------------------- Project ------------------------->
<--- Compile --->
Link: http://docs.opencv.org/2.4/doc/tutorials/introduction/linux_gcc_cmake/linux_gcc_cmake.html

<--- Clone ---> 
git clone [http://ownrepo]

<-------------------- Laborator ------------------------->
Informatii laborator FIC:

Site: g.iovanalex.ro

Tool: Mobaxtrem Personal 9.3

Ip's:
172.16.254.246, 89, 42, 198, 53, 59, 117, 206, 199, 159, 88, 63.

Username: cloud
Password: cloud123

1.
Install medit: sudo apt-get install mc

2.
Compilare sa vedem codul in assembler: gcc -S nume.c -> nume.s

3.
mcedit nume.s

4.
Compilare fara sa leg binarul de executabil: gcc -c -o 1.o 1.c

5.
Deasamblare ( decompilare ): objdump -d 1.o ( -d disassamble )

6.
(Optimize first level - easy )
gcc -O1 -S 1.c
