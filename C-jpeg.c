////////////////////////////////////////////////////////////////////////////////
//
//      Custom JPEG encoder
//          for FER PPP course
//
//      - Assumptions:
//          - maximum image size known
//          - image received as a RGB value list via stdin
//          - image width and height multiples of 8
//
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#pragma pack(1)

#define MAX 4096

const float RGB_to_YUV_factors[ 3 ][ 4 ] = {
    { 0.257, 0.504, 0.098, 16 },
    { -0.148, -0.291, 0.439, 128 },
    { 0.439, -0.368, -0.071, 128 }
};
const float DCT_coefficients[ 8 ][ 8 ] = {
    { .3536, .3536, .3536, .3536, .3536, .3536, .3536, .3536 },
    { .4904, .4157, .2778, .0975,-.0975,-.2778,-.4157,-.4094 },
    { .4619, .1913,-.1913,-.4619,-.4619,-.1913, .1913, .4619 },
    { .4157,-.0975,-.4904,-.2778, .2778, .4904, .0975,-.4157 },
    { .3536,-.3536,-.3536, .3536, .3536,-.3536,-.3536, .3536 },
    { .2778,-.4904, .0975, .4157,-.4157,-.0975, .4904,-.2778 },
    { .1913,-.4619, .4619,-.1913,-.1913, .4619,-.4619, .1913 },
    { .0975,-.2778, .4157,-.4904, .4904,-.4157, .2778,-.0975 }
};
const float DCT_coefficients_transp[ 8 ][ 8 ] = {
    { .3536, .4904, .4619, .4157, .3536, .2778, .1913, .0975 },
    { .3536, .4157, .1913,-.0975,-.3536,-.4904,-.4619,-.2778 },
    { .3536, .2778,-.1913,-.4904,-.3536, .0975, .4619, .4157 },
    { .3536, .0975,-.4619,-.2778, .3536, .4157,-.1913,-.4904 },
    { .3536,-.0975,-.4619, .2778, .3536,-.4157,-.1913, .4904 },
    { .3536,-.2778,-.1913, .4904,-.3536,-.0975, .4619,-.4157 },
    { .3536,-.4157, .1913, .0975,-.3536, .4904,-.4619, .2778 },
    { .3536,-.4094, .4619,-.4157, .3536,-.2778, .1913,-.0975 }
};
const short quantization_table_luminance[8][8] = {
	{ 16, 11, 10, 16, 24, 40, 51, 61 },
	{ 12, 12, 14, 19, 26, 58, 60, 55 },
	{ 14, 13, 16, 24, 40, 57, 69, 56 },
	{ 14, 17, 22, 29, 51, 87, 80, 62 },
	{ 18, 22, 37, 56, 68, 109, 103, 77 },
	{ 24, 35, 55, 64, 81, 104, 113, 92 },
	{ 49, 64, 78, 87, 103, 121, 120, 101 },
	{ 72, 92, 95, 98, 112, 100, 103, 99 }
};

const short quantization_table_chrominance[8][8] = {
	{ 17, 18, 24, 47, 99, 99, 99, 99 },
	{ 18, 21, 26, 66, 99, 99, 99, 99 },
	{ 24, 26, 56, 99, 99, 99, 99, 99 },
	{ 47, 66, 99, 99, 99, 99, 99, 99 },
	{ 99, 99, 99, 99, 99, 99, 99, 99 },
	{ 99, 99, 99, 99, 99, 99, 99, 99 },
	{ 99, 99, 99, 99, 99, 99, 99, 99 },
	{ 99, 99, 99, 99, 99, 99, 99, 99 }
};

short RGB_image[ MAX ][ MAX ][ 3 ];
short YUV_image[ MAX ][ MAX ][ 3 ];
short DCT_image[ MAX ][ MAX ][ 3 ];
short ZigZag_U[64];
short ZigZag_V[64]
int N, M;

void generate_YUV_image();
void quantize( int, int );
void perform_DCT( int, int );
void read_input();
void shift_values( int, int );

int main( void ){
	ZigZag_U = { 0 };
	ZigZag_V = { 0 };
    read_input();

    printf( "%d %d\n", N, M );

    puts( "RGB:" );
    for( int k = 0; k < 3; ++k ){
        for( int i = 0; i < N; ++i ){
            for( int j = 0; j < M; ++j )
                printf( "%6d", RGB_image[ i ][ j ][ k ] );
            puts( "" );
        }
        puts( "" );
    }

    generate_YUV_image();

    puts( "YUV:" );
    for( int k = 0; k < 3; ++k ){
        for( int i = 0; i < N; ++i ){
            for( int j = 0; j < M; ++j )
                printf( "%6d", YUV_image[ i ][ j ][ k ] );
            puts( "" );
        }
        puts( "" );
    }

    for( int i = 0; i < N; i += 8 )
        for( int j = 0; j < M; j += 8 ){
            printf( "Block (%d, %d):\n", i/8, j/8  );
            shift_values( i, j );

            for( int a = 0; a < 3; ++a ){
                for( int b = 0; b < 8; ++b ){
                    for( int c = 0; c < 8; ++c )
                        printf( "%6d", YUV_image[ i+b ][ j+c ][ a ] );
                    puts( "" );
                }
                puts( "" ); 
            }
            
            perform_DCT( i, j );
            quantize( i, j );
			zig_zag(i, j, ZigZag_U, 1);
			zig_zag(i, j, ZigZag_V, 2);
        }

    return 0;
}

void generate_YUV_image(){
    float temp;
    for( int i = 0; i < N; ++i )
        for( int j = 0; j < M; ++j )
            for( int k = 0; k < 3; ++k ){
                temp = RGB_to_YUV_factors[ k ][ 3 ];

                for( int l = 0; l < 3; ++l )
                    temp += RGB_image[ i ][ j ][ l ] * RGB_to_YUV_factors[ k ][ l ];

                YUV_image[ i ][ j ][ k ] = temp;
            }
}

void perform_DCT( int x, int y ){
    float temp[ 8 ][ 8 ];
    for( int l = 0; l < 3; ++l ){
        memset( temp, 0, sizeof( temp ) );
        for( int i = 0; i < 8; ++i )
            for( int j = 0; j < 8; ++j )
                for( int k = 0; k < 8; ++k )
                    temp[ i ][ j ] += DCT_coefficients[ i ][ k ] * YUV_image[ x+k ][ y+j ][ l ];
        for( int i = 0; i < 8; ++i )
            for( int j = 0; j < 8; ++j )
                for( int k = 0; k < 8; ++k )
                    DCT_image[ x+i ][ y+j ][ l ] += temp[ i ][ k ] * DCT_coefficients_transp[ k ][ j ];
    }
}

void read_input(){
    scanf( "%d%d", &N, &M );
    for( int i = 0; i < N; ++i )
        for( int j = 0; j < M; ++j )
            for( int k = 0; k < 3; ++k )
                scanf( "%d", &RGB_image[ i ][ j ][ k ] );
}

struct BitMap
{
	unsigned short int Type;
	unsigned int Size;
	unsigned short int Reserved1, Reserved2;
	unsigned int Offset;
} Header;

struct BitMapInfo
{
	unsigned int Size;
	int Width, Height;
	unsigned short int Planes;
	unsigned short int Bits;
	unsigned int Compression;
	unsigned int ImageSize;
	int xResolution, yResolution;
	unsigned int Colors;
	unsigned int ImportantColors;
} InfoHeader;

struct Pixels
{
	unsigned char Blue, Green, Red;
};

/*void read_input(void) {

	int i = 0, j = 0;
	int size_spix;
	int padding = 0;
	char temp[4];
	struct Pixels **pixel_arrayp;

	FILE *BMP_in = fopen("test.bmp", "rb");
	if (BMP_in == NULL) {

		printf("Soap test.bmp ne moze se otvoriti");
		exit(1);
	}


	fread(&Header.Type, sizeof(Header.Type), 1, BMP_in);
	fread(&Header.Size, sizeof(Header.Size), 1, BMP_in);
	fread(&Header.Reserved1, sizeof(Header.Reserved1), 1, BMP_in);
	fread(&Header.Reserved2, sizeof(Header.Reserved2), 1, BMP_in);
	fread(&Header.Offset, sizeof(Header.Offset), 1, BMP_in);
	fread(&InfoHeader, sizeof(InfoHeader), 1, BMP_in);


	padding = InfoHeader.Width % 4;
	if (padding != 0) {
		padding = 4 - padding;
	}

	size_spix = sizeof(struct Pixels);


	pixel_arrayp = (struct Pixels **)calloc(InfoHeader.Height, sizeof(struct Pixel*));


	for (i = 0; i<InfoHeader.Height; i++) {
		pixel_arrayp[i] = (struct Pixels *)calloc(InfoHeader.Width, size_spix);
	}

	for (i = 0; i < InfoHeader.Height; i++) {
		for (j = 0; j < InfoHeader.Width; j++) {


			fread(&pixel_arrayp[i][j], 3, 1, BMP_in);

		}
		if (padding != 0) {
			fread(&temp, padding, 1, BMP_in);
		}
	}

	fclose(BMP_in);
}*/

void shift_values( int x, int y ){
    for( int i = 0; i < 8; ++i )
        for( int j = 0; j < 8; ++j )
            for( int k = 0; k < 3; ++k )
                YUV_image[ x+i ][ y+j ][ k ] -= 128;
}

void quantize(int x, int y) {
	int i, j, k;
	for (k = 0; k < 3; k++)
		for (i = 0; i < 8; ++i)
			for (j = 0; j < 8; ++j)
				DCT_image[x + i][y + j][k] /= (k == 0 ? quantization_table_luminance[i][j] :
					quantization_table_chrominance[i][j]);
}

void zig_zag(int x, int y, int ZZ[64], int yuv) {
	int i = x, j = y, k = 0, d = 0;
	// do dijagonale
	while (k<36){
		ZZ[k++] = DCT_image[i][j][yuv];
		if ((i == 0) && (j % 2 == 0)) {
			j++;
			d = 1;
		}
		else if ((j == 0) && (i % 2 == 1)) {
			i++;
			d = 0;
		}
		else if (d == 0) {
			i--;
			j++;
		}
		else {
			i++;
			j--;
		}
	}
	// poslije dijagonale
	i = 7 + x;
	j = 1 + y;
	d = 0;
	while (k<64) {
		ZZ[k++] = DCT_image[i][j][yuv];
		if ((i == 7) && (j % 2 == 0)) {
			j++;
			d = 0;
		}
		else if ((j == 7) && (i % 2 == 1)) {
			i++;
			d = 1;
		}
		else if (d == 0) {
			i--;
			j++;
		}
		else {
			i++;
			j--;
		}
	}
}