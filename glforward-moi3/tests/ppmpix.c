/* ppmpix <arquivo.ppm> <x> <y>: imprime "R G B" do pixel (x, y), com y contado de cima.
 * Usado pelo run_tests.sh para conferir o quadro que o glhost grava na troca de buffers. */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE *f;
    int w = 0, h = 0, max = 0, x, y;
    unsigned char p[3];
    if (argc < 4) { fprintf(stderr, "uso: ppmpix arquivo.ppm x y\n"); return 2; }
    f = fopen(argv[1], "rb");
    if (!f) { printf("sem arquivo %s\n", argv[1]); return 1; }
    if (fscanf(f, "P6 %d %d %d", &w, &h, &max) != 3) { printf("cabecalho PPM invalido\n"); return 1; }
    fgetc(f);
    x = atoi(argv[2]); y = atoi(argv[3]);
    if (x < 0 || y < 0 || x >= w || y >= h) { printf("fora da imagem %dx%d\n", w, h); return 1; }
    if (fseek(f, (long)(y * w + x) * 3, SEEK_CUR) || fread(p, 1, 3, f) != 3) { printf("arquivo curto\n"); return 1; }
    printf("%d %d %d\n", p[0], p[1], p[2]);
    return 0;
}
