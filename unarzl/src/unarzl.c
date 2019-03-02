#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int arzl_decompress(unsigned char *buffer, unsigned int buflen, const unsigned char *input, const unsigned char **endptr)
{
#define LOBYTE(x) (*(uint8_t *)&x)
  unsigned char *v5; // r5@1
  unsigned char v6; // r11@1
  int v7; // r4@1
  unsigned char *v8; // r1@2
  signed int v9; // r2@2
  unsigned int v10; // r3@2
  signed int v11; // r0@2
  unsigned int v12; // r7@3
  unsigned char *v13; // r8@3
  unsigned char *v14; // r7@4
  unsigned int v15; // r6@4
  int v16; // lr@5
  int v17; // r6@5
  unsigned char *i; // r8@9
  unsigned char *v19; // r7@10
  unsigned char *v20; // r12@10
  unsigned int v21; // r6@10
  int v22; // r10@11
  int v23; // r6@11
  int v25; // r10@22
  int v26; // lr@23
  unsigned int v27; // r3@24
  signed int v28; // r12@24
  const unsigned char *v29; // r9@24
  unsigned char v30; // zf@25
  unsigned int v31; // lr@25
  int v32; // r2@27
  int v33; // r6@29
  unsigned char *v34; // r6@30
  int v35; // r2@31
  unsigned char *v36; // r0@32
  unsigned int v37; // r2@32
  int v38; // lr@33
  int v39; // r2@33
  uint8_t v40; // cf@33
  signed int v41; // r10@37
  int v42; // r2@42
  unsigned int v43; // r9@43
  int v44; // r7@43
  int v45; // lr@51
  signed int v46; // r7@52
  unsigned int v47; // r10@56
  int v48; // r7@57
  int v49; // r7@58
  int v50; // r10@58
  unsigned char v51; // cf@58
  unsigned int v52; // r10@65
  int v53; // r7@66
  int v54; // r7@67
  int v55; // r10@67
  unsigned int v56; // r10@71
  int v57; // r7@72
  int v58; // r7@73
  int v59; // r10@73
  unsigned char v60; // cf@73
  signed int v61; // r7@83
  unsigned char *v62; // r9@84
  unsigned int v63; // r10@85
  int v64; // r2@85
  int v65; // lr@85
  signed int v66; // r0@87
  signed int v67; // r0@92
  int v68; // r6@96
  unsigned char *v69; // r12@96
  unsigned int v70; // r0@96
  int v71; // lr@97
  int v72; // r0@97
  unsigned char v73; // cf@97
  signed int v74; // r10@101
  int v75; // r0@101
  int v76; // r0@106
  unsigned int v77; // r9@107
  int v78; // r7@107
  int v79; // lr@115
  signed int v80; // r7@116
  unsigned int v81; // r10@120
  int v82; // r7@121
  int v83; // r7@122
  int v84; // r10@122
  unsigned char v85; // cf@122
  unsigned int v86; // r10@129
  int v87; // r7@130
  int v88; // r7@131
  int v89; // r10@131
  unsigned int v90; // r7@135
  int v91; // r6@136
  int v92; // r6@137
  int v93; // r7@137
  unsigned char v94; // cf@137
  unsigned char *v95; // r12@144
  unsigned char *v96; // r6@145
  int v97; // r0@145
  int v98; // lr@150
  int v99; // lr@151
  int v100; // r10@157
  const unsigned char *input_1; // [sp+4h] [bp-CE4h]@1
  int v104; // [sp+10h] [bp-CD8h]@32
  signed int v105; // [sp+10h] [bp-CD8h]@81
  signed int v106; // [sp+10h] [bp-CD8h]@92
  signed int v107; // [sp+14h] [bp-CD4h]@83
  int v108; // [sp+14h] [bp-CD4h]@96
  unsigned char v109[3240]; // [sp+18h] [bp-CD0h]@2

  v5 = &buffer[buflen];
  input_1 = input;
  v6 = *input & 7;
  v7 = __builtin_bswap32(*(uint32_t *)(input + 1));
  if ( !(*input & 0x80) )
  {
    memset(v109, 16 * (8 - (((const uint8_t)*input >> 3) & 3)) & 0xF0, 3240);
    v8 = buffer;
    v9 = 0;
    v10 = -1;
    for ( v11 = 0; v8 != v5; *v20 = v22 )
    {
      v12 = (v9 & 0xFFFFF8FF) | (((uint8_t)v8 & 7) << 8);
      v9 = 1;
      v13 = &v109[255 * ((v12 >> v6) & 7)];
      do
      {
        v14 = &v13[v9];
        v15 = (uint8_t)v13[v9 - 1];
        v9 *= 2;
        if ( !(v10 >> 24) )
        {
          v10 <<= 8;
          v25 = (input_1++)[5];
          v7 = v25 + (v7 << 8);
        }
        v16 = v15 * (v10 >> 8);
        v17 = v15 - (v15 >> 3);
        v10 -= v16;
        if ( v7 >= (unsigned int)v16 )
        {
          v7 -= v16;
        }
        else
        {
          ++v9;
          LOBYTE(v17) = v17 + 31;
          v10 = v16;
        }
        *(v14 - 1) = v17;
      }
      while ( v9 <= 255 );
      for ( i = v8; ; i = &v8[~(intptr_t)i] + (intptr_t)v95 )
      {
        v19 = &v109[v11];
        *i = v9;
        v8 = i + 1;
        v20 = &v109[v11 + 2920];
        v21 = (uint8_t)*v20;
        if ( !(v10 >> 24) )
        {
          v10 <<= 8;
          v26 = (input_1++)[5];
          v7 = v26 + (v7 << 8);
        }
        v22 = v21 - (v21 >> 4);
        v23 = v21 * (v10 >> 8);
        if ( v7 >= (unsigned int)v23 )
          break;
        v27 = v23;
        *v20 = v22 + 15;
        v28 = -1;
        v29 = input_1;
        while ( 1 )
        {
          v30 = v27 >> 24 == 0;
          v31 = (uint8_t)v19[2928];
          if ( !(v27 >> 24) )
          {
            v27 <<= 8;
            v23 = (v29++)[5];
          }
          v19 += 8;
          v32 = v31 * (v27 >> 8);
          if ( v30 )
            v7 = v23 + (v7 << 8);
          v33 = v31 - (v31 >> 4);
          if ( v7 >= (unsigned int)v32 )
            break;
          ++v28;
          v23 = v33 + 15;
          v27 = v31 * (v27 >> 8);
          v19[2920] = v23;
          if ( v28 == 6 )
          {
            input_1 = v29;
            v34 = &v109[6];
            v10 = v32;
            goto LABEL_32;
          }
        }
        v10 = v27 - v32;
        v7 -= v32;
        input_1 = v29;
        v19[2920] = v33;
        v34 = &v109[v28];
        if ( v28 == -1 )
        {
          v35 = 1;
          goto LABEL_82;
        }
LABEL_32:
        v36 = &v109[32 * v28 | 8 * (((uint32_t)v8 << v28) & 3) | (v11 & 7)];
        v104 = v28 - 3;
        v37 = (uint8_t)v36[2984];
        if ( !(v10 >> 24) )
        {
          v10 <<= 8;
          v100 = (input_1++)[5];
          v7 = v100 + (v7 << 8);
        }
        v38 = v37 - (v37 >> 4);
        v39 = v37 * (v10 >> 8);
        v40 = v7 >= (unsigned int)v39;
        if ( v7 >= (unsigned int)v39 )
        {
          v10 -= v39;
          v7 -= v39;
        }
        else
        {
          LOBYTE(v38) = v38 + 15;
          v10 = v39;
        }
        if ( v40 )
        {
          v41 = 4;
          v35 = 2;
        }
        else
        {
          v41 = 6;
          v35 = 3;
        }
        v36[2984] = v38;
        if ( v104 >= 0 )
        {
          if ( v28 != 3 )
          {
            if ( !(v10 >> 24) )
            {
              v10 <<= 8;
              v42 = (input_1++)[5];
              v7 = v42 + (v7 << 8);
            }
            v43 = (uint8_t)v38 - ((unsigned int)(uint8_t)v38 >> 4);
            v44 = (uint8_t)v38 * (v10 >> 8);
            if ( v7 >= (unsigned int)v44 )
              v35 = v41;
            else
              v35 = v41 + 1;
            if ( v7 >= (unsigned int)v44 )
            {
              v10 -= v44;
              v7 -= v44;
            }
            else
            {
              LOBYTE(v43) = v43 + 15;
              v10 = (uint8_t)v38 * (v10 >> 8);
            }
            v36[2984] = v43;
            if ( v104 != 1 )
            {
              if ( !(v10 >> 24) )
              {
                v10 <<= 8;
                v45 = (input_1++)[5];
                v7 = v45 + (v7 << 8);
              }
              v46 = v28;
              do
              {
                v10 >>= 1;
                --v46;
                v35 = (v7 < v10) + 2 * v35;
                if ( v7 >= v10 )
                  v7 -= v10;
              }
              while ( v46 != 4 );
            }
          }
          v47 = (uint8_t)v36[3008];
          if ( !(v10 >> 24) )
          {
            v10 <<= 8;
            v48 = (input_1++)[5];
            v7 = v48 + (v7 << 8);
          }
          v49 = v47 - (v47 >> 4);
          v35 *= 2;
          v50 = v47 * (v10 >> 8);
          v51 = v7 >= (unsigned int)v50;
          if ( v7 >= (unsigned int)v50 )
          {
            v10 -= v50;
            v7 -= v50;
          }
          else
          {
            ++v35;
            v10 = v50;
          }
          if ( !v51 )
            LOBYTE(v49) = v49 + 15;
          v36[3008] = v49;
        }
        if ( v28 )
        {
          v52 = (uint8_t)v36[2992];
          if ( !(v10 >> 24) )
          {
            v10 <<= 8;
            v53 = (input_1++)[5];
            v7 = v53 + (v7 << 8);
          }
          v54 = v52 - (v52 >> 4);
          v35 *= 2;
          v55 = v52 * (v10 >> 8);
          if ( v7 >= (unsigned int)v55 )
          {
            v10 -= v55;
            v7 -= v55;
          }
          else
          {
            LOBYTE(v54) = v54 + 15;
            ++v35;
            v10 = v55;
          }
          v36[2992] = v54;
          if ( v28 != 1 )
          {
            v56 = (uint8_t)v36[3000];
            if ( !(v10 >> 24) )
            {
              v10 <<= 8;
              v57 = (input_1++)[5];
              v7 = v57 + (v7 << 8);
            }
            v58 = v56 - (v56 >> 4);
            v35 *= 2;
            v59 = v56 * (v10 >> 8);
            v60 = v7 >= (unsigned int)v59;
            if ( v7 >= (unsigned int)v59 )
            {
              v10 -= v59;
              v7 -= v59;
            }
            else
            {
              ++v35;
              v10 = v59;
            }
            if ( !v60 )
              LOBYTE(v58) = v58 + 15;
            v36[3000] = v58;
          }
        }
        if ( v35 == 255 )
        {
          if ( endptr )
            *endptr = input_1 + 5;
          return v8 - buffer;
        }
        if ( v35 != 2 )
        {
          v105 = 256;
          goto LABEL_83;
        }
LABEL_82:
        v34 += 248;
        v105 = 64;
LABEL_83:
        v61 = 8;
        v107 = v35;
        do
        {
          v62 = &v34[v61];
          v61 *= 2;
          if ( !(v10 >> 24) )
          {
            v10 <<= 8;
            v98 = (input_1++)[5];
            v7 = v98 + (v7 << 8);
          }
          v63 = (uint8_t)v62[2033];
          v64 = v63 * (v10 >> 8);
          v65 = v63 - (v63 >> 3);
          v10 -= v64;
          if ( v7 < (unsigned int)v64 )
          {
            v61 += 8;
            LOBYTE(v65) = v65 + 31;
          }
          v66 = v61 - v105;
          if ( v7 >= (unsigned int)v64 )
            v7 -= v64;
          else
            v10 = v64;
          v62[2033] = v65;
        }
        while ( v66 < 0 );
        v9 = v107;
        if ( v61 != v105 )
        {
          v67 = v66 >> 3;
          v106 = v67;
          if ( v107 <= 2 )
            v67 = 0;
          if ( v107 > 2 )
            v67 = 1;
          v68 = v106 - 1;
          v108 = v106 - 4;
          v69 = &v109[8 * ((v67 << (v106 - 1)) & 3) | 32 * (v106 - 1) | (v28 & 7)];
          v70 = (uint8_t)v69[2344];
          if ( !(v10 >> 24) )
          {
            v10 <<= 8;
            v99 = (input_1++)[5];
            v7 = v99 + (v7 << 8);
          }
          v71 = v70 - (v70 >> 4);
          v72 = v70 * (v10 >> 8);
          v73 = v7 >= (unsigned int)v72;
          if ( v7 >= (unsigned int)v72 )
          {
            v10 -= v72;
            v7 -= v72;
          }
          else
          {
            LOBYTE(v71) = v71 + 15;
            v10 = v72;
          }
          if ( v73 )
          {
            v74 = 4;
            v75 = 2;
          }
          else
          {
            v74 = 6;
            v75 = 3;
          }
          v69[2344] = v71;
          if ( v108 >= 0 )
          {
            if ( v106 != 4 )
            {
              if ( !(v10 >> 24) )
              {
                v10 <<= 8;
                v76 = (input_1++)[5];
                v7 = v76 + (v7 << 8);
              }
              v77 = (uint8_t)v71 - ((unsigned int)(uint8_t)v71 >> 4);
              v78 = (uint8_t)v71 * (v10 >> 8);
              if ( v7 >= (unsigned int)v78 )
                v75 = v74;
              else
                v75 = v74 + 1;
              if ( v7 >= (unsigned int)v78 )
              {
                v10 -= v78;
                v7 -= v78;
              }
              else
              {
                LOBYTE(v77) = v77 + 15;
                v10 = (uint8_t)v71 * (v10 >> 8);
              }
              v69[2344] = v77;
              if ( v108 != 1 )
              {
                if ( !(v10 >> 24) )
                {
                  v10 <<= 8;
                  v79 = (input_1++)[5];
                  v7 = v79 + (v7 << 8);
                }
                v80 = v106;
                do
                {
                  v10 >>= 1;
                  --v80;
                  v75 = (v7 < v10) + 2 * v75;
                  if ( v7 >= v10 )
                    v7 -= v10;
                }
                while ( v80 != 5 );
              }
            }
            v81 = (uint8_t)v69[2368];
            if ( !(v10 >> 24) )
            {
              v10 <<= 8;
              v82 = (input_1++)[5];
              v7 = v82 + (v7 << 8);
            }
            v83 = v81 - (v81 >> 4);
            v75 *= 2;
            v84 = v81 * (v10 >> 8);
            v85 = v7 >= (unsigned int)v84;
            if ( v7 >= (unsigned int)v84 )
            {
              v10 -= v84;
              v7 -= v84;
            }
            else
            {
              ++v75;
              v10 = v84;
            }
            if ( !v85 )
              LOBYTE(v83) = v83 + 15;
            v69[2368] = v83;
          }
          if ( v68 > 0 )
          {
            v86 = (uint8_t)v69[2352];
            if ( !(v10 >> 24) )
            {
              v10 <<= 8;
              v87 = (input_1++)[5];
              v7 = v87 + (v7 << 8);
            }
            v88 = v86 - (v86 >> 4);
            v75 *= 2;
            v89 = v86 * (v10 >> 8);
            if ( v7 >= (unsigned int)v89 )
            {
              v10 -= v89;
              v7 -= v89;
            }
            else
            {
              LOBYTE(v88) = v88 + 15;
              ++v75;
              v10 = v89;
            }
            v69[2352] = v88;
            if ( v68 != 1 )
            {
              v90 = (uint8_t)v69[2360];
              if ( !(v10 >> 24) )
              {
                v10 <<= 8;
                v91 = (input_1++)[5];
                v7 = v91 + (v7 << 8);
              }
              v92 = v90 - (v90 >> 4);
              v75 *= 2;
              v93 = v90 * (v10 >> 8);
              v94 = v7 >= (unsigned int)v93;
              if ( v7 >= (unsigned int)v93 )
              {
                v10 -= v93;
                v7 -= v93;
              }
              else
              {
                ++v75;
                v10 = v93;
              }
              if ( !v94 )
                LOBYTE(v92) = v92 + 15;
              v69[2360] = v92;
            }
          }
          v66 = v75 - 1;
        }
        v95 = i + 1;
        if ( v8 - buffer <= (unsigned int)v66 )
          return 0x80560200;
        v96 = &v8[v9];
        v97 = ~v66;
        v40 = v5 >= &v8[v9];
        v30 = v5 == &v8[v9];
        LOBYTE(v9) = v8[v97];
        if ( !(!v30 & v40) )
          return 0x80560201;
        do
        {
          *(uint8_t *)v95++ = v9;
          v9 = *(uint8_t *)(v95 + v97);
        }
        while ( v95 != v96 );
        v11 = 7;
      }
      v10 -= v23;
      if ( v11 )
        --v11;
      else
        v11 = 0;
      v7 -= v23;
    }
    return 0x80560201;
  }
  if ( buflen < v7 )
    return 0x80560201;
  memcpy(buffer, input + 5, __builtin_bswap32(*(uint32_t *)(input + 1)));
  if ( endptr )
    *endptr = &input[v7 + 5];
  return v7;
}

int arzl_deobfuscate(unsigned char *buffer, int len, int version)
{
  unsigned char *buf; // r3@1
  unsigned char *bufend; // r12@1
  unsigned int data; // t1@3 MAPDST
  unsigned int change_stride; // r5@3
  unsigned char *buf_1; // r3@8
  unsigned int v10; // r4@8
  unsigned int v11; // r5@8
  unsigned int v12; // t1@12
  unsigned int v13; // r4@13
  unsigned int v14; // r4@14
  intptr_t offset;

  buf = buffer;
  bufend = &buffer[len];
  if ( len )
  {
    if ( version == 2 )
    {
      do
      {
        data = *(uint32_t *)buf;
        buf += 4;
        change_stride = (data & 0xF800F800) >> 27;
        offset = (buf - buffer);
        if ( (data & 0xF800F800) == 0xF800F000 )
        {
          v14 = (((data >> 16) & 0xFFC007FF) | ((data & 0x7FF) << 11)) - (offset >> 1);
          v14 = ((((v14 & 0x7FF) << 16) | 0xF800F000) & 0xFFFFF800) | ((v14 >> 11) & 0x7FF);
          *((uint32_t *)buf - 1) = v14;
        }
        else if ( (data & 0x8000FBF0) == 0x0000F2C0 )
        {
          v14 = (data & 0xF0FFFFF0) | ((data & 0xF) << 24) | ((data >> 24) & 0xF);
          *((uint32_t *)buf - 1) = v14;
        }
        else if ( change_stride == 30 )
        {
          buf -= 2;
        }
      }
      while ( bufend > buf );
    }
    else if ( version == 1 )
    {
      buf_1 = buffer + 4;
      v10 = *(uint32_t *)buffer;
      v11 = (*(uint32_t *)buffer & 0xF800F800) >> 27;
      if ( (*(uint32_t *)buffer & 0xF800F800) == 0xF800F000 )
        goto LABEL_13;
LABEL_9:
      if ( v11 == 30 )
        buf_1 -= 2;
      while ( buf_1 < bufend )
      {
        v12 = *(uint32_t *)buf_1;
        buf_1 += 4;
        v10 = v12;
        v11 = (v12 & 0xF800F800) >> 27;
        if ( (v12 & 0xF800F800) != 0xF800F000 )
          goto LABEL_9;
LABEL_13:
        v13 = (((v10 >> 16) & 0xFFC007FF) | ((v10 & 0x7FF) << 11)) + ((buf_1 - buffer) >> 1);
        *((uint32_t *)buf_1 - 1) = ((((v13 & 0x7FF) << 16) | 0xF800F000) & 0xFFFFF800) | ((v13 >> 11) & 0x7FF);
      }
    }
    else
    {
      do
      {
        data = *(uint32_t *)buf;
        buf += 4;
        change_stride = (data & 0xF800F800) >> 27;
        if ( (data & 0xF800F800) == 0xF800F000 )
        {
          v14 = (((data >> 16) & 0xFFC007FF) | ((data & 0x7FF) << 11)) - ((buf - buffer) >> 1);
          *((uint32_t *)buf - 1) = ((((v14 & 0x7FF) << 16) | 0xF800F000) & 0xFFFFF800) | ((v14 >> 11) & 0x7FF);
        }
        else if ( change_stride == 30 )
        {
          buf -= 2;
        }
      }
      while ( bufend > buf );
    }
  }
  return len;
}

int main(int argc, const char *argv[])
{
  FILE *input;
  FILE *output;
  long int size;
  long int count;
  char *inbuf;
  char *outbuf;
  int outlen;
  int len;
  int ret;

  input = output = NULL;
  inbuf = outbuf = NULL;
  ret = 1;
  if (argc < 2)
  {
    fprintf(stderr, "usage: %s input [output]\n", argv[0]);
    return 1;
  }

  if ((input = fopen(argv[1], "rb")) == NULL)
  {
    perror("input");
    goto error;
  }

  if (argc > 2) {
	if ((output = fopen(argv[2], "wb")) == NULL) {
		perror("output");
		goto error;
	}
  } else if ((output = fopen("uncompressed_data.bin", "wb")) == NULL) {
	  	perror("output");
		goto error;
  }

  fseek(input, 0, SEEK_END);
  if ((size = ftell(input)) < 0)
  {
    perror("input");
    goto error;
  }
  fseek(input, 0, SEEK_SET);

  if ((inbuf = malloc(size)) == NULL)
  {
    perror("memory");
    goto error;
  }
  outlen = (int)size;

  count = 0;
  while ((count = fread(inbuf+count, sizeof(char), size, input)) < size)
  {
    if (count < 0)
    {
      perror("read");
      goto error;
    }
    size -= count;
  }

  do
  {
    outlen *= 2; // resize buffer
    outbuf = realloc(outbuf, outlen);
    len = arzl_decompress((uint8_t *)outbuf, outlen, (uint8_t *)inbuf+4, NULL);
  } while (len == 0x80560201); // out of space

  if (len < 0)
  {
    fprintf(stderr, "Error decompressing: 0x%08X\n", len);
    goto error;
  }
  arzl_deobfuscate((uint8_t *)outbuf, len, 2);

  count = 0;
  while ((count = fwrite(outbuf+count, sizeof(char), len, output)) < len)
  {
    if (count < 0)
    {
      perror("write");
      goto error;
    }
    len -= count;
  }

  ret = 0;
error:
  free(outbuf);
  free(inbuf);
  fclose(output);
  fclose(input);
  return ret;
}
