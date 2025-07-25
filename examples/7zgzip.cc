/* 7z Lib */
#include "../7z/CpuArch.c"
#include "../7z/DeflateEncoder.cc"
#include "../7z/LzFind.c"
#include "../7z/OutBuffer.cc"
#include "../7z/HuffEnc.c"
#include "../7z/Sort.c"

/* 7zDeflate Lib*/
#include "../7zDeflate.cc"

/* crc */
#include "crc.c"

#include <time.h>

int printusage(char* prog) {
    fprintf(stderr,
    "Usage: %s [-{1-9}[,{fb_num}[,{pass_num}]]] <infile >outfile\n"
    "Example:\n"
    "  %s -9,258 <data.txt >data.txt.gz\n"
    "  compress data.txt to data.txt.gz with max level 9 and fastbytes 258\n"
    , prog, prog
    );
    return -1;
}

int doencode(int level, int fb, int passes) {
    Deflate7zEncoder* encoder = Deflate7zCreateEncoder(Deflate7zGetParamsEx(DEFLATE_7Z_GZIP, level, fb, passes), crc32);
    return Deflate7zFileStream(encoder, (int)time(0), stdin, stdout);
}

int main(int argc, char** argv) {
    int level = 9;
    int fb = -1;
    int passes = -1;
    if (argc > 2) {
        return printusage(argv[0]);
    }
    else if (argc == 2) {
        char* arg = argv[1];
        if (arg[0] == '-' && (arg[2] == '\0' || arg[2] == ',') && arg[1] >= '1' && arg[1] <= '9') {
            level = arg[1] - '0';
        } else {
            return printusage(argv[0]);
        }
        if (arg[2] == ',')
            fb = atoi(&arg[3]);
        for (arg = &arg[3]; *arg != ',' && *arg != '\0'; ++arg);
        if (*arg == ',')
            passes = atoi(&arg[1]);
    }

    return doencode(level, fb, passes);
}