/* Convert a binary file to an include file */
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <file in> <file out>\n", *argv);
    }
    else
    {
        FILE * fpin;
        FILE * fpout;

        if ((fpin = fopen(argv[1], "rb")) == NULL)
        {
            perror("cannot open input file");
        }
        else if ((fpout = fopen(argv[2], "w")) == NULL)
        {
            perror("cannot open output file");
            fclose(fpin);
        }
        else
        {
            fprintf(fpout, "#include \"pico.h\"\n");

            char* dot = strrchr(argv[2], '.');
            if (dot)
            {
                *dot = 0;
            }
            fprintf(fpout, "const unsigned char __in_flash() %s[] = {", argv[2]);

            int c;
            int line_count = 0;
            while ((c = fgetc(fpin)) != EOF)
            {
                if (!line_count)
                {
                    fputs("\n   ", fpout);
                }
                fprintf(fpout, " 0x%02x,", (c & 0xff));
                line_count = (line_count + 1) % 16;
            }
            fputs("\n};\n", fpout);

            fclose(fpin);
            fclose(fpout);

            return 0;
        }
    }
    return -1;
}
