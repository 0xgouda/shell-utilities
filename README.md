# Shell Utilites
- a collection of implementations of popular Linux command-line utilities, built from scratch. The tools are simplified versions of cat, zip, unzip, and grep.
- gpzip & gpunzip are multi-threaded versions of zip & unzip

> gpzip&gzip use RLE encoding e.g aaa = 3a  

# Compile and Run
```bash
gcc -o ggrep ggrep.c
./ggrep string file.txt
gcc -o gcat gcat.c
./gcat file.txt
...
```